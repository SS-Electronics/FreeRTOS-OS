/*
 * hal_uart_infineon.h — Infineon XMC series baremetal UART ops declaration
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Stub implementation for Infineon XMC4xxx / XMC1xxx USIC-UART.
 * Fill in hal_uart_infineon.c with actual register operations when porting
 * to an Infineon target.  The ops table interface is identical to the STM32
 * implementation so the generic driver layer requires no changes.
 */

#ifndef DRIVERS_HAL_INFINEON_HAL_UART_INFINEON_H_
#define DRIVERS_HAL_INFINEON_HAL_UART_INFINEON_H_

#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

#include <drivers/drv_uart.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Return pointer to the Infineon UART HAL ops table. */
const drv_uart_hal_ops_t *hal_uart_infineon_get_ops(void);

/**
 * @brief  Set the hardware context for an Infineon UART channel.
 *
 * @param  h             Generic UART handle.
 * @param  channel_base  USIC channel base address (e.g. XMC_UART0_CH0_BASE).
 */
void hal_uart_infineon_set_config(drv_uart_handle_t *h,
                                  uint32_t           channel_base);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON */

#endif /* DRIVERS_HAL_INFINEON_HAL_UART_INFINEON_H_ */
