/**
 * @file    hal_spi_stm32.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL SPI backend implementation with blocking and interrupt-driven support
 *
 * @details
 * This module implements the SPI HAL abstraction layer for STM32 platforms.
 * It provides both blocking and interrupt-driven SPI operations through the
 * drv_spi_hal_ops_t interface.
 *
 * Features:
 * - Blocking APIs: transmit, receive, full-duplex transfer
 * - Interrupt-driven APIs: transmit_it, receive_it, transfer_it
 * - Direct IRQ-driven completion handling (no HAL weak callbacks)
 * - Board-level GPIO and NVIC ownership (no MSP dependency)
 *
 * -----------------------------------------------------------------------------
 * @section spi_flow SPI Execution Flow
 *
 * Blocking mode:
 * @code
 * App → drv_spi_* → HAL_SPI_* → Peripheral
 * @endcode
 *
 * Interrupt mode:
 * @code
 * App → drv_spi_*_it → HAL_SPI_*_IT
 *     → SPI IRQ
 *         → hal_spi_stm32_irq_handler()
 *             → HAL_SPI_IRQHandler()
 *             → State comparison (before/after)
 *             → drv_irq_dispatch_from_isr()
 * @endcode
 *
 * -----------------------------------------------------------------------------
 * @section irq_design IRQ Design
 *
 * - No HAL weak callbacks are used
 * - Completion inferred via HAL state machine:
 *     BUSY_TX    → TX_DONE
 *     BUSY_RX    → RX_DONE
 *     BUSY_TX_RX → TXRX_DONE
 * - Errors take precedence over completion events
 *
 * -----------------------------------------------------------------------------
 * @dependencies
 * board/mcu_config.h, board/board_config.h,
 * drivers/com/drv_spi.h,
 * drivers/com/hal/stm32/hal_spi_stm32.h,
 * drivers/drv_irq.h
 *
 * @note
 * Compiled only when CONFIG_DEVICE_VARIANT == MCU_VAR_STM
 * and HAL_SPI_MODULE_ENABLED is defined.
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * @license
 * GNU General Public License v3 or later.
 */

#include <board/mcu_config.h>
#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM) && defined(HAL_SPI_MODULE_ENABLED)

#include <drivers/drv_spi.h>
#include <drivers/hal/stm32/hal_spi_stm32.h>
#include <drivers/drv_irq.h>

/* ────────────────────────────────────────────────────────────────────────── */
/* Internal Helpers                                                          */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Convert HAL status to OS error code
 */
__SECTION_OS __USED
static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* HAL Operations: Initialization                                            */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize SPI hardware
 *
 * @param h SPI handle
 * @return OS_ERR_NONE on success, OS_ERR_OP on failure
 *
 * @details
 * - Configures GPIO pins (SCK, MISO, MOSI, optional NSS)
 * - Enables IRQ in NVIC
 * - Calls HAL_SPI_Init()
 *
 * GPIO/NVIC ownership is handled here (not MSP).
 */
__SECTION_OS __USED
static int32_t stm32_spi_hw_init(drv_spi_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    SPI_HandleTypeDef      *hspi = &h->hw.hspi;
    const board_spi_desc_t *d    = board_find_spi(hspi->Instance);
    if (d == NULL)
        return OS_ERR_OP;

    GPIO_InitTypeDef gpio = {
        .Mode      = d->sck_pin.mode,
        .Pull      = d->sck_pin.pull,
        .Speed     = d->sck_pin.speed,
        .Alternate = d->sck_pin.alternate,
    };

    gpio.Pin = d->sck_pin.pin;
    HAL_GPIO_Init(d->sck_pin.port, &gpio);

    gpio.Pin       = d->miso_pin.pin;
    gpio.Alternate = d->miso_pin.alternate;
    HAL_GPIO_Init(d->miso_pin.port, &gpio);

    gpio.Pin       = d->mosi_pin.pin;
    gpio.Alternate = d->mosi_pin.alternate;
    HAL_GPIO_Init(d->mosi_pin.port, &gpio);

    if (d->nss_pin.pin != 0)
    {
        gpio.Pin       = d->nss_pin.pin;
        gpio.Alternate = d->nss_pin.alternate;
        HAL_GPIO_Init(d->nss_pin.port, &gpio);
    }

    drv_irq_enable((int32_t)d->irqn, d->irq_priority);

    HAL_StatusTypeDef ret = HAL_SPI_Init(hspi);
    if (ret == HAL_OK)
        h->initialized = 1;

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/**
 * @brief Deinitialize SPI hardware
 */
__SECTION_OS __USED
static int32_t stm32_spi_hw_deinit(drv_spi_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    const board_spi_desc_t *d = board_find_spi(h->hw.hspi.Instance);

    HAL_StatusTypeDef ret = HAL_SPI_DeInit(&h->hw.hspi);
    h->initialized = 0;
    h->last_err    = _hal_to_os_err(ret);

    if (d != NULL)
    {
        drv_irq_disable((int32_t)d->irqn);
        HAL_GPIO_DeInit(d->sck_pin.port,  d->sck_pin.pin);
        HAL_GPIO_DeInit(d->miso_pin.port, d->miso_pin.pin);
        HAL_GPIO_DeInit(d->mosi_pin.port, d->mosi_pin.pin);
        if (d->nss_pin.pin != 0)
            HAL_GPIO_DeInit(d->nss_pin.port, d->nss_pin.pin);
    }

    return h->last_err;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Blocking Operations                                                      */
/* ────────────────────────────────────────────────────────────────────────── */
__SECTION_OS __USED
static int32_t stm32_spi_transmit(drv_spi_handle_t *h,
                                  const uint8_t    *data,
                                  uint16_t          len,
                                  uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret =
        HAL_SPI_Transmit(&h->hw.hspi, (uint8_t *)data, len, timeout_ms);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

__SECTION_OS __USED
static int32_t stm32_spi_receive(drv_spi_handle_t *h,
                                 uint8_t          *data,
                                 uint16_t          len,
                                 uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret =
        HAL_SPI_Receive(&h->hw.hspi, data, len, timeout_ms);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

__SECTION_OS __USED
static int32_t stm32_spi_transfer(drv_spi_handle_t *h,
                                  const uint8_t    *tx,
                                  uint8_t          *rx,
                                  uint16_t          len,
                                  uint32_t          timeout_ms)
{
    if (h == NULL || !h->initialized || tx == NULL || rx == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret =
        HAL_SPI_TransmitReceive(&h->hw.hspi, (uint8_t *)tx, rx, len, timeout_ms);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Interrupt-based Operations                                               */
/* ────────────────────────────────────────────────────────────────────────── */
__SECTION_OS __USED
static int32_t stm32_spi_transmit_it(drv_spi_handle_t *h,
                                     const uint8_t    *data,
                                     uint16_t          len)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret =
        HAL_SPI_Transmit_IT(&h->hw.hspi, (uint8_t *)data, len);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

__SECTION_OS __USED
static int32_t stm32_spi_receive_it(drv_spi_handle_t *h,
                                    uint8_t          *data,
                                    uint16_t          len)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret =
        HAL_SPI_Receive_IT(&h->hw.hspi, data, len);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

__SECTION_OS __USED
static int32_t stm32_spi_transfer_it(drv_spi_handle_t *h,
                                     const uint8_t    *tx,
                                     uint8_t          *rx,
                                     uint16_t          len)
{
    if (h == NULL || !h->initialized || tx == NULL || rx == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret =
        HAL_SPI_TransmitReceive_IT(&h->hw.hspi, (uint8_t *)tx, rx, len);

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* Ops Table                                                                */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief STM32 SPI HAL operations table
 */
static const drv_spi_hal_ops_t _stm32_spi_ops = {
    .hw_init     = stm32_spi_hw_init,
    .hw_deinit   = stm32_spi_hw_deinit,
    .transmit    = stm32_spi_transmit,
    .receive     = stm32_spi_receive,
    .transfer    = stm32_spi_transfer,
    .transmit_it = stm32_spi_transmit_it,
    .receive_it  = stm32_spi_receive_it,
    .transfer_it = stm32_spi_transfer_it,
};

/* ────────────────────────────────────────────────────────────────────────── */
/* Public API                                                               */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Get STM32 SPI HAL operations
 */
__SECTION_OS __USED
const drv_spi_hal_ops_t *hal_spi_stm32_get_ops(void)
{
    return &_stm32_spi_ops;
}

/**
 * @brief Configure SPI peripheral parameters
 */
__SECTION_OS __USED
void hal_spi_stm32_set_config(drv_spi_handle_t *h,
                              SPI_TypeDef      *instance,
                              uint32_t          mode,
                              uint32_t          direction,
                              uint32_t          data_size,
                              uint32_t          clk_polarity,
                              uint32_t          clk_phase,
                              uint32_t          nss,
                              uint32_t          baud_prescaler,
                              uint32_t          bit_order)
{
    if (h == NULL || instance == NULL)
        return;

    SPI_HandleTypeDef *hspi = &h->hw.hspi;

    hspi->Instance               = instance;
    hspi->Init.Mode              = mode;
    hspi->Init.Direction         = direction;
    hspi->Init.DataSize          = data_size;
    hspi->Init.CLKPolarity       = clk_polarity;
    hspi->Init.CLKPhase          = clk_phase;
    hspi->Init.NSS               = nss;
    hspi->Init.BaudRatePrescaler = baud_prescaler;
    hspi->Init.FirstBit          = bit_order;
    hspi->Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial     = 10;
}

/* ────────────────────────────────────────────────────────────────────────── */
/* IRQ Handler                                                              */
/* ────────────────────────────────────────────────────────────────────────── */

/**
 * @brief SPI IRQ handler with completion detection
 *
 * @details
 * Uses HAL state comparison before/after IRQ handling to determine
 * transfer completion and dispatch appropriate events.
 */
__SECTION_OS __USED
void hal_spi_stm32_irq_handler(SPI_TypeDef *instance)
{
    for (uint8_t id = 0; id < NO_OF_SPI; id++)
    {
        drv_spi_handle_t *h = drv_spi_get_handle(id);
        if (h == NULL || !h->initialized || h->hw.hspi.Instance != instance)
            continue;

        HAL_SPI_StateTypeDef prev = HAL_SPI_GetState(&h->hw.hspi);
        HAL_SPI_IRQHandler(&h->hw.hspi);

        if (HAL_SPI_GetState(&h->hw.hspi) != HAL_SPI_STATE_READY)
            return;

        BaseType_t hpt  = pdFALSE;
        uint32_t   err  = HAL_SPI_GetError(&h->hw.hspi);

        if (err != HAL_SPI_ERROR_NONE)
        {
            h->last_err = OS_ERR_OP;
            drv_irq_dispatch_from_isr(IRQ_ID_SPI_ERROR(id), &err, &hpt);
        }
        else
        {
            h->last_err = OS_ERR_NONE;

            if (prev == HAL_SPI_STATE_BUSY_TX)
                drv_irq_dispatch_from_isr(IRQ_ID_SPI_TX_DONE(id), NULL, &hpt);
            else if (prev == HAL_SPI_STATE_BUSY_RX)
                drv_irq_dispatch_from_isr(IRQ_ID_SPI_RX_DONE(id), NULL, &hpt);
            else if (prev == HAL_SPI_STATE_BUSY_TX_RX)
                drv_irq_dispatch_from_isr(IRQ_ID_SPI_TXRX_DONE(id), NULL, &hpt);
        }

        portYIELD_FROM_ISR(hpt);
        return;
    }
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM && HAL_SPI_MODULE_ENABLED */