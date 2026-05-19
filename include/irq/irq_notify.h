/**
 * @file        irq_notify.h
 * @brief       irq_notify.h — Generic vendor-agnostic IRQ notification framework
 * @ingroup     irq
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      IRQ Subsystem
 * @info        Linux-style irq_desc chain, software IRQ table, request_irq/free_irq, NVIC chip driver.
 * @dependency  Generated irq_table.c, NVIC chip
 *
 * @details
 * irq_notify.h — Generic vendor-agnostic IRQ notification framework
 *
 * Design
 * ──────
 * Hardware IRQ events are mapped to generic numeric IRQ IDs defined by this
 * header.  Any driver or service can subscribe to an IRQ ID via irq_register().
 * When the hardware fires, the vendor HAL callback calls irq_notify_from_isr()
 * which dispatches to every registered callback in ISR context.
 *
 * Naming convention
 * ─────────────────
 *   IRQ_ID_UART_RX(n)        — byte received on UART bus n
 *   IRQ_ID_UART_TX_DONE(n)   — transmit complete on UART bus n
 *   IRQ_ID_UART_ERROR(n)     — UART error on bus n
 *   IRQ_ID_I2C_TX_DONE(n)    — I2C master transmit complete on bus n
 *   IRQ_ID_I2C_RX_DONE(n)    — I2C master receive complete on bus n
 *   IRQ_ID_I2C_ERROR(n)      — I2C error on bus n
 *   IRQ_ID_SPI_TX_DONE(n)    — SPI transmit complete on bus n
 *   IRQ_ID_SPI_RX_DONE(n)    — SPI receive complete on bus n
 *   IRQ_ID_SPI_TXRX_DONE(n)  — SPI full-duplex transfer complete on bus n
 *   IRQ_ID_SPI_ERROR(n)      — SPI error on bus n
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

#ifndef INCLUDE_IRQ_IRQ_NOTIFY_H_
#define INCLUDE_IRQ_IRQ_NOTIFY_H_

/*
 * Capacity constants, base offsets, irq_id_t typedef, and ID generator macros
 * live in board/irq_hw_conf.h so they can be tuned per board via irq_table.xml.
 *
 * Application builds:  app/board/irq_hw_conf.h  (auto-generated, shadows stub)
 * Standalone kernel:   include/board/irq_hw_conf.h  (static defaults)
 */
#include <board/irq_hw_conf.h>

#include <FreeRTOS.h>

/* ── Callback type ───────────────────────────────────────────────────────── */

/**
 * irq_notify_cb_t — subscriber callback invoked by irq_notify / irq_notify_from_isr.
 *
 * @param id     The IRQ ID that fired.
 * @param data   Event-specific payload pointer (e.g. received byte ptr, error code ptr).
 * @param arg    User-supplied argument passed to irq_register().
 * @param pxHPT  FreeRTOS higher-priority-task-woken accumulator.
 *               Set via vTaskNotifyGiveFromISR() etc. if a task was unblocked.
 *               NULL when called from task context (irq_notify).
 */
typedef void (*irq_notify_cb_t)(irq_id_t      id,
                                void         *data,
                                void         *arg,
                                BaseType_t   *pxHPT);

/* ── API ─────────────────────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Register a callback for a generic IRQ ID.
 *         Call from task context before interrupts fire.
 *         Duplicate (id, cb, arg) tuples are silently ignored.
 * @return 0 on success, -1 if the subscriber table for @p id is full.
 */
int32_t irq_register(irq_id_t id, irq_notify_cb_t cb, void *arg);

/**
 * @brief  Dispatch notification to all registered callbacks (task context).
 *         pxHPT is passed as NULL — callbacks must not call FromISR APIs.
 */
void irq_notify(irq_id_t id, void *data);

/**
 * @brief  Dispatch notification from ISR context.
 *         Each registered callback receives the same pxHPT pointer so they
 *         can accumulate the yield flag.  Caller must call
 *         portYIELD_FROM_ISR(*pxHPT) after this function returns.
 */
void irq_notify_from_isr(irq_id_t id, void *data, BaseType_t *pxHPT);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_IRQ_IRQ_NOTIFY_H_ */
