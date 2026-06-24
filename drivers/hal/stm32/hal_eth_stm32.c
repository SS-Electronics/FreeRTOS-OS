/**
 * @file        hal_eth_stm32.c
 * @brief       STM32H7 Ethernet MAC backend for the lwIP ping bring-up
 * @ingroup     hal_stm32
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      STM32 HAL
 * @info        Implements drv_eth.h on the STM32H7 ETH peripheral driving the
 *              on-board LAN8742 PHY over RMII. Owns the DMA descriptors and
 *              packet buffers (placed in D2 SRAM, made non-cacheable by the
 *              MPU), the RMII pin mux and the PHY auto-negotiation. RX is
 *              interrupt-driven: HAL_ETH_RxCpltCallback gives a semaphore the
 *              net service thread blocks on (drv_eth_rx_wait).
 * @dependency  stm32h7xx-hal-driver (ETH, GPIO, CORTEX/MPU), drv_eth.h
 *
 * @details
 * Buffer ownership
 * ────────────────
 * HAL_ETH_ReadData() drives three weak callbacks that we override globally
 * (USE_HAL_ETH_REGISTER_CALLBACKS == 0):
 *   HAL_ETH_RxAllocateCallback  hands the DMA a free RX buffer to re-arm a
 *                               descriptor.
 *   HAL_ETH_RxLinkCallback      records the length of a completed frame and
 *                               returns its buffer as the "app buffer".
 *   HAL_ETH_TxFreeCallback      unused: TX is copy-then-blocking-transmit.
 * Frames never exceed one buffer (RxBuffLen == 1536), so each frame maps to a
 * single pool slot. The slot stays owned by the caller of drv_eth_receive()
 * until drv_eth_rx_release() frees it.
 *
 * @copyright
 * This file is part of FreeRTOS-OS Project.
 *
 * FreeRTOS-OS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * FreeRTOS-OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with FreeRTOS-OS. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <device.h>

#if defined(HAL_ETH_MODULE_ENABLED)

#include <drivers/drv_eth.h>
#include <drivers/hal/stm32/hal_eth_stm32.h>
#include <os/kernel.h>
#include <string.h>

/* ── Geometry ───────────────────────────────────────────────────────────── */

#define ETH_RX_BUFFER_SIZE      1536U   /* multiple of 4, holds one frame   */
#define ETH_RX_BUFFER_CNT       8U      /* >= ETH_RX_DESC_CNT + in-flight   */
#define ETH_TX_BUFFER_SIZE      1536U

/* LAN8742 sits at RMII PHY address 0 on the NUCLEO-H723ZG. */
#define LAN8742_PHY_ADDR        0U

/* IEEE 802.3 PHY registers (subset). */
#define PHY_REG_BCR             0x00U   /* Basic Control                    */
#define PHY_REG_BSR             0x01U   /* Basic Status                     */
#define PHY_REG_SSR             0x1FU   /* LAN8742 Special Control/Status   */

#define PHY_BCR_RESET           0x8000U
#define PHY_BCR_AUTONEG_EN      0x1000U
#define PHY_BCR_AUTONEG_RESTART 0x0200U

#define PHY_BSR_LINK_UP         0x0004U
#define PHY_BSR_AUTONEG_DONE    0x0020U

#define PHY_SSR_DUPLEX_FULL     0x0010U /* bit 4: 1 = full duplex           */
#define PHY_SSR_SPEED_MASK      0x000CU /* bits[3:2]: 01 = 10M, 10 = 100M   */
#define PHY_SSR_SPEED_100M      0x0008U

#define PHY_RESET_TIMEOUT_MS    500U
#define PHY_AUTONEG_TIMEOUT_MS  5000U
#define ETH_TX_TIMEOUT_MS       50U

/* ── D2 SRAM resident DMA structures (non-cacheable via MPU) ─────────────── */

/** RX/TX DMA descriptor rings — must live in non-cacheable RAM. */
static ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".eth_d2")));
static ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".eth_d2")));

/** RX buffer pool. Each slot backs at most one in-flight frame. */
static uint8_t  rx_pool[ETH_RX_BUFFER_CNT][ETH_RX_BUFFER_SIZE] __attribute__((section(".eth_d2")));
static uint16_t rx_pool_len[ETH_RX_BUFFER_CNT];
static volatile uint8_t rx_pool_used[ETH_RX_BUFFER_CNT];

/** Single TX bounce buffer: lwIP pbufs are copied here before transmit. */
static uint8_t  tx_buffer[ETH_TX_BUFFER_SIZE] __attribute__((section(".eth_d2")));

/* ── Module state ───────────────────────────────────────────────────────── */

static ETH_HandleTypeDef heth;
static ETH_TxPacketConfigTypeDef tx_config;

/** Binary semaphore: given by the ETH RX-complete ISR, taken by the net thread. */
static os_sem_t rx_sem;

/* ── RX pool helpers ────────────────────────────────────────────────────── */

/** @brief Map a pool buffer pointer back to its slot index, or -1. */
static int32_t _rx_pool_index(const uint8_t *buf)
{
	for (uint32_t i = 0; i < ETH_RX_BUFFER_CNT; i++)
	{
		if (buf == rx_pool[i])
		{
			return (int32_t)i;
		}
	}

	return -1;
}

/* ── HAL ETH weak-callback overrides ────────────────────────────────────── */

/**
 * @brief Hand the DMA a free RX buffer to attach to a descriptor.
 * @note  Called from inside HAL_ETH_ReadData()/HAL_ETH_Start(). Returns NULL
 *        when the pool is exhausted; the DMA then stops re-arming until a
 *        buffer is released, which provides natural back-pressure.
 */
void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
	for (uint32_t i = 0; i < ETH_RX_BUFFER_CNT; i++)
	{
		if (rx_pool_used[i] == 0U)
		{
			rx_pool_used[i] = 1U;
			*buff = rx_pool[i];
			return;
		}
	}

	*buff = NULL;
}

/**
 * @brief Record a completed RX frame and present it as the app buffer.
 * @note  Frames fit in a single buffer, so pStart/pEnd both point at @p buff.
 */
void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
	int32_t idx = _rx_pool_index(buff);

	if (idx >= 0)
	{
		rx_pool_len[idx] = Length;
	}

	if (*pStart == NULL)
	{
		*pStart = buff;
	}

	*pEnd = buff;
}

/**
 * @brief TX completion hook — unused (TX is synchronous copy-and-block).
 */
void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
	(void)buff;
}

/**
 * @brief RX-complete ISR callback: wake the net thread to drain frames.
 * @note  Runs in ETH_IRQn context (NVIC priority 6 >= max-syscall priority),
 *        so the *FromISR give is legal. A context switch is requested via the
 *        woken flag when a higher-priority task is readied.
 */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *eth)
{
	(void)eth;

	int32_t woken = 0;
	(void)os_sem_give_from_isr(rx_sem, &woken);
	portYIELD_FROM_ISR(woken);
}

/**
 * @brief MAC/DMA error ISR callback — no action beyond HAL bookkeeping.
 */
void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *eth)
{
	(void)eth;
}

/**
 * @brief ETH global interrupt entry (routed from the generated ETH_IRQHandler).
 */
void hal_eth_stm32_irq_handler(void)
{
	HAL_ETH_IRQHandler(&heth);
}

/* ── MPU: carve D2 SRAM out of the cache ────────────────────────────────── */

/**
 * @brief Mark the 0x30000000 D2 SRAM window non-cacheable.
 * @note  Runs before HAL_ETH_Init touches any descriptor, so no stale cache
 *        lines can exist for the region. 32 KB covers descriptors + pools.
 */
static void _mpu_config_d2(void)
{
	MPU_Region_InitTypeDef mpu = {0};

	HAL_MPU_Disable();

	mpu.Enable           = MPU_REGION_ENABLE;
	mpu.Number           = MPU_REGION_NUMBER0;
	mpu.BaseAddress      = 0x30000000UL;
	mpu.Size             = MPU_REGION_SIZE_32KB;
	mpu.SubRegionDisable = 0x00;
	mpu.TypeExtField     = MPU_TEX_LEVEL1;
	mpu.AccessPermission = MPU_REGION_FULL_ACCESS;
	mpu.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
	mpu.IsShareable      = MPU_ACCESS_SHAREABLE;
	mpu.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
	mpu.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;

	HAL_MPU_ConfigRegion(&mpu);
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/* ── RMII pin mux + peripheral clocks (called by HAL_ETH_Init) ──────────── */

/**
 * @brief Low-level ETH bring-up: clocks and RMII GPIO mux.
 * @note  HAL_ETH_Init() invokes this automatically.
 *        NUCLEO-H723ZG RMII map (all AF11_ETH):
 *          PA1 REF_CLK   PA2 MDIO    PA7 CRS_DV
 *          PB13 TXD1
 *          PC1 MDC       PC4 RXD0    PC5 RXD1
 *          PG11 TX_EN    PG13 TXD0
 */
void HAL_ETH_MspInit(ETH_HandleTypeDef *eth)
{
	(void)eth;

	GPIO_InitTypeDef gpio = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_ETH1MAC_CLK_ENABLE();
	__HAL_RCC_ETH1TX_CLK_ENABLE();
	__HAL_RCC_ETH1RX_CLK_ENABLE();

	gpio.Mode      = GPIO_MODE_AF_PP;
	gpio.Pull      = GPIO_NOPULL;
	gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
	gpio.Alternate = GPIO_AF11_ETH;

	gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
	HAL_GPIO_Init(GPIOA, &gpio);

	gpio.Pin = GPIO_PIN_13;
	HAL_GPIO_Init(GPIOB, &gpio);

	gpio.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
	HAL_GPIO_Init(GPIOC, &gpio);

	gpio.Pin = GPIO_PIN_11 | GPIO_PIN_13;
	HAL_GPIO_Init(GPIOG, &gpio);
}

/* ── PHY (LAN8742) ──────────────────────────────────────────────────────── */

/**
 * @brief Reset the PHY, run auto-negotiation and program the MAC speed/duplex.
 * @return OS_ERR_NONE on a completed link, OS_ERR_HW_FAULT on MDIO/timeout.
 */
static int32_t _phy_autoneg(void)
{
	uint32_t reg = 0;
	uint32_t start;

	/* Soft-reset the PHY and wait for the reset bit to self-clear. */
	if (HAL_ETH_WritePHYRegister(&heth, LAN8742_PHY_ADDR, PHY_REG_BCR,
	                             PHY_BCR_RESET) != HAL_OK)
	{
		return OS_ERR_HW_FAULT;
	}

	start = HAL_GetTick();
	do
	{
		if ((HAL_GetTick() - start) > PHY_RESET_TIMEOUT_MS)
		{
			return OS_ERR_HW_FAULT;
		}
		if (HAL_ETH_ReadPHYRegister(&heth, LAN8742_PHY_ADDR, PHY_REG_BCR,
		                            &reg) != HAL_OK)
		{
			return OS_ERR_HW_FAULT;
		}
	} while ((reg & PHY_BCR_RESET) != 0U);

	/* Kick off auto-negotiation. */
	if (HAL_ETH_WritePHYRegister(&heth, LAN8742_PHY_ADDR, PHY_REG_BCR,
	                             PHY_BCR_AUTONEG_EN | PHY_BCR_AUTONEG_RESTART) != HAL_OK)
	{
		return OS_ERR_HW_FAULT;
	}

	/* Wait for auto-negotiation to complete with the link up. */
	start = HAL_GetTick();
	do
	{
		if ((HAL_GetTick() - start) > PHY_AUTONEG_TIMEOUT_MS)
		{
			return OS_ERR_HW_FAULT;
		}
		if (HAL_ETH_ReadPHYRegister(&heth, LAN8742_PHY_ADDR, PHY_REG_BSR,
		                            &reg) != HAL_OK)
		{
			return OS_ERR_HW_FAULT;
		}
	} while ((reg & (PHY_BSR_AUTONEG_DONE | PHY_BSR_LINK_UP))
	         != (PHY_BSR_AUTONEG_DONE | PHY_BSR_LINK_UP));

	/* Read the negotiated speed/duplex from the LAN8742 status register. */
	if (HAL_ETH_ReadPHYRegister(&heth, LAN8742_PHY_ADDR, PHY_REG_SSR,
	                            &reg) != HAL_OK)
	{
		return OS_ERR_HW_FAULT;
	}

	ETH_MACConfigTypeDef mac = {0};
	HAL_ETH_GetMACConfig(&heth, &mac);

	mac.DuplexMode = ((reg & PHY_SSR_DUPLEX_FULL) != 0U)
	               ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;
	mac.Speed      = ((reg & PHY_SSR_SPEED_MASK) == PHY_SSR_SPEED_100M)
	               ? ETH_SPEED_100M : ETH_SPEED_10M;

	HAL_ETH_SetMACConfig(&heth, &mac);

	return OS_ERR_NONE;
}

/* ── Public driver API ──────────────────────────────────────────────────── */

int32_t drv_eth_init(const uint8_t *mac)
{
	if (mac == NULL)
	{
		return OS_ERR_NULL_PTR;
	}

	_mpu_config_d2();

	/* Binary semaphore for RX-complete signalling (starts empty). */
	rx_sem = os_sem_create(1, 0);
	if (rx_sem == NULL)
	{
		return OS_ERR_HW_FAULT;
	}

	heth.Instance            = ETH;
	heth.Init.MACAddr        = (uint8_t *)mac;
	heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
	heth.Init.TxDesc         = DMATxDscrTab;
	heth.Init.RxDesc         = DMARxDscrTab;
	heth.Init.RxBuffLen      = ETH_RX_BUFFER_SIZE;

	if (HAL_ETH_Init(&heth) != HAL_OK)
	{
		return OS_ERR_HW_FAULT;
	}

	/* TX config is fixed: hardware appends CRC + padding, checksums are done
	 * in software by lwIP (MAC offload stays off). */
	tx_config.Attributes   = ETH_TX_PACKETS_FEATURES_CRCPAD;
	tx_config.CRCPadCtrl   = ETH_CRC_PAD_INSERT;
	tx_config.ChecksumCtrl = ETH_CHECKSUM_DISABLE;

	if (_phy_autoneg() != OS_ERR_NONE)
	{
		return OS_ERR_HW_FAULT;
	}

	return OS_ERR_NONE;
}

int32_t drv_eth_start(void)
{
	/* Interrupt-driven RX: arms descriptors with IOC and enables ETH DMA IRQs.
	 * HAL_ETH_RxCpltCallback then signals rx_sem on each completed frame. */
	if (HAL_ETH_Start_IT(&heth) != HAL_OK)
	{
		return OS_ERR_HW_FAULT;
	}

	return OS_ERR_NONE;
}

int32_t drv_eth_rx_wait(uint32_t timeout_ms)
{
	return os_sem_take(rx_sem, timeout_ms);
}

int32_t drv_eth_transmit(const uint8_t *payload, uint16_t len)
{
	if (payload == NULL)
	{
		return OS_ERR_NULL_PTR;
	}
	if ((len == 0U) || (len > ETH_TX_BUFFER_SIZE))
	{
		return OS_ERR_INVALID_ARG;
	}

	memcpy(tx_buffer, payload, len);

	ETH_BufferTypeDef tx_buf = {0};
	tx_buf.buffer = tx_buffer;
	tx_buf.len    = len;
	tx_buf.next   = NULL;

	tx_config.Length   = len;
	tx_config.TxBuffer = &tx_buf;

	if (HAL_ETH_Transmit(&heth, &tx_config, ETH_TX_TIMEOUT_MS) != HAL_OK)
	{
		return OS_ERR_HW_FAULT;
	}

	return OS_ERR_NONE;
}

int32_t drv_eth_receive(uint8_t **payload, uint16_t *len)
{
	if ((payload == NULL) || (len == NULL))
	{
		return OS_ERR_NULL_PTR;
	}

	void *app = NULL;

	if ((HAL_ETH_ReadData(&heth, &app) != HAL_OK) || (app == NULL))
	{
		return OS_ERR_OP;
	}

	uint8_t *buf = (uint8_t *)app;
	int32_t  idx = _rx_pool_index(buf);

	if (idx < 0)
	{
		/* Should never happen; drop and let the buffer leak-protect itself. */
		return OS_ERR_OP;
	}

	*payload = buf;
	*len     = rx_pool_len[idx];

	return OS_ERR_NONE;
}

void drv_eth_rx_release(const uint8_t *payload)
{
	int32_t idx = _rx_pool_index(payload);

	if (idx >= 0)
	{
		rx_pool_used[idx] = 0U;
	}
}

int32_t drv_eth_link_is_up(void)
{
	uint32_t reg = 0;

	if (HAL_ETH_ReadPHYRegister(&heth, LAN8742_PHY_ADDR, PHY_REG_BSR,
	                            &reg) != HAL_OK)
	{
		return 0;
	}

	return ((reg & PHY_BSR_LINK_UP) != 0U) ? 1 : 0;
}

#endif /* HAL_ETH_MODULE_ENABLED */
