/**
 * @file        drv_eth.h
 * @brief       Ethernet MAC driver interface (vendor-neutral)
 * @ingroup     drivers
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Driver Layer
 * @info        Vendor-agnostic driver vtables; concrete backends live under drivers/hal/<vendor>/.
 * @dependency  HAL backend (selected by CONFIG_DEVICE_VARIANT)
 *
 * @details
 *              STM32H7 backend (drivers/hal/stm32/hal_eth_stm32.c) owns the
 *              ETH peripheral, RMII pins, MPU setup for the D2 buffers, and
 *              the LAN8742 PHY. RX is interrupt-driven (drv_eth_rx_wait).
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

#ifndef DRIVERS_DRV_ETH_H_
#define DRIVERS_DRV_ETH_H_

#include <def_std.h>
#include <def_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum payload a single received frame can carry (one DMA buffer). */
#define DRV_ETH_MAX_FRAME_LEN   1536U

/** Length of the hardware MAC address in bytes. */
#define DRV_ETH_MAC_LEN         6U

/**
 * @brief Initialise the Ethernet MAC, PHY and DMA buffers.
 * @note  Configures the MPU non-cacheable window over D2 SRAM, brings up the
 *        RMII pins and clocks, runs PHY auto-negotiation and programs the MAC.
 *        Call once, from thread context, before the scheduler hands control to
 *        the RX poll loop.
 * @param mac  Six-byte MAC address to program into the MAC filter.
 * @return OS_ERR_NONE on success, OS_ERR_HW_FAULT on a HAL/PHY failure,
 *         OS_ERR_NULL_PTR if @p mac is NULL.
 */
int32_t drv_eth_init(const uint8_t *mac);

/**
 * @brief Start the MAC/DMA so frames begin flowing.
 * @return OS_ERR_NONE on success, OS_ERR_HW_FAULT on failure.
 */
int32_t drv_eth_start(void);

/**
 * @brief Copy a frame into the TX buffer and transmit it (blocking).
 * @param payload  Frame bytes (Ethernet header through payload, no FCS).
 * @param len      Frame length in bytes (<= DRV_ETH_MAX_FRAME_LEN).
 * @return OS_ERR_NONE on success, OS_ERR_HW_FAULT on a DMA error,
 *         OS_ERR_INVALID_ARG on a bad length, OS_ERR_NULL_PTR if NULL.
 */
int32_t drv_eth_transmit(const uint8_t *payload, uint16_t len);

/**
 * @brief Block until the MAC signals a received frame (or the timeout lapses).
 * @note  RX is interrupt-driven: the ETH ISR (HAL_ETH_RxCpltCallback) releases
 *        an internal semaphore. The net RX thread calls this, then drains with
 *        drv_eth_receive(). A timeout lets the caller re-check link state.
 * @param timeout_ms  Max wait in ms, or OS_WAIT_FOREVER to block indefinitely.
 * @return OS_ERR_NONE if signalled, OS_ERR_TIMEOUT on timeout.
 */
int32_t drv_eth_rx_wait(uint32_t timeout_ms);

/**
 * @brief Poll for one received frame.
 * @note  On success @p payload points into a driver-owned D2 buffer that
 *        stays valid until drv_eth_rx_release() is called for it. Copy the
 *        bytes out, then release promptly so the buffer can be re-armed.
 * @param payload  Out: pointer to the received frame buffer.
 * @param len      Out: frame length in bytes.
 * @return OS_ERR_NONE when a frame was dequeued, OS_ERR_OP when none is ready.
 */
int32_t drv_eth_receive(uint8_t **payload, uint16_t *len);

/**
 * @brief Return a buffer obtained from drv_eth_receive() to the free pool.
 * @param payload  The pointer previously handed out by drv_eth_receive().
 */
void drv_eth_rx_release(const uint8_t *payload);

/**
 * @brief Query the PHY link state.
 * @return 1 if the link is up, 0 if down.
 */
int32_t drv_eth_link_is_up(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_DRV_ETH_H_ */
