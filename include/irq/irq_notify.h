/*
 * irq_notify.h — Generic vendor-agnostic IRQ notification framework
 *
 * This file is part of FreeRTOS-OS Project.
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
 */

#ifndef INCLUDE_IRQ_IRQ_NOTIFY_H_
#define INCLUDE_IRQ_IRQ_NOTIFY_H_

#include <def_std.h>
#include <FreeRTOS.h>

/* ── Capacity constants ──────────────────────────────────────────────────── */

#define IRQ_MAX_UART            4   /* max UART bus instances     */
#define IRQ_MAX_I2C             4   /* max I2C bus instances      */
#define IRQ_MAX_SPI             4   /* max SPI bus instances      */
#define IRQ_MAX_EXTI_LINES      16  /* GPIO EXTI lines 0–15       */

#define IRQ_UART_EVENTS         3   /* RX, TX_DONE, ERROR         */
#define IRQ_I2C_EVENTS          3   /* TX_DONE, RX_DONE, ERROR    */
#define IRQ_SPI_EVENTS          4   /* TX_DONE, RX_DONE, TXRX, ERR */

#define IRQ_NOTIFY_MAX_SUBS     4   /* subscribers per IRQ ID     */

/* ── IRQ ID base offsets ─────────────────────────────────────────────────── */

#define IRQ_ID_UART_BASE   0
#define IRQ_ID_I2C_BASE    (IRQ_ID_UART_BASE + IRQ_MAX_UART * IRQ_UART_EVENTS)
#define IRQ_ID_SPI_BASE    (IRQ_ID_I2C_BASE  + IRQ_MAX_I2C  * IRQ_I2C_EVENTS)
#define IRQ_ID_EXTI_BASE   (IRQ_ID_SPI_BASE  + IRQ_MAX_SPI  * IRQ_SPI_EVENTS)
#define IRQ_ID_TOTAL       (IRQ_ID_EXTI_BASE + IRQ_MAX_EXTI_LINES)

typedef uint32_t irq_id_t;

/* ── ID generator macros ─────────────────────────────────────────────────── */

#define IRQ_ID_UART_RX(n)        ((irq_id_t)(IRQ_ID_UART_BASE + (n)*IRQ_UART_EVENTS + 0))
#define IRQ_ID_UART_TX_DONE(n)   ((irq_id_t)(IRQ_ID_UART_BASE + (n)*IRQ_UART_EVENTS + 1))
#define IRQ_ID_UART_ERROR(n)     ((irq_id_t)(IRQ_ID_UART_BASE + (n)*IRQ_UART_EVENTS + 2))

#define IRQ_ID_I2C_TX_DONE(n)    ((irq_id_t)(IRQ_ID_I2C_BASE  + (n)*IRQ_I2C_EVENTS  + 0))
#define IRQ_ID_I2C_RX_DONE(n)    ((irq_id_t)(IRQ_ID_I2C_BASE  + (n)*IRQ_I2C_EVENTS  + 1))
#define IRQ_ID_I2C_ERROR(n)      ((irq_id_t)(IRQ_ID_I2C_BASE  + (n)*IRQ_I2C_EVENTS  + 2))

#define IRQ_ID_SPI_TX_DONE(n)    ((irq_id_t)(IRQ_ID_SPI_BASE  + (n)*IRQ_SPI_EVENTS  + 0))
#define IRQ_ID_SPI_RX_DONE(n)    ((irq_id_t)(IRQ_ID_SPI_BASE  + (n)*IRQ_SPI_EVENTS  + 1))
#define IRQ_ID_SPI_TXRX_DONE(n)  ((irq_id_t)(IRQ_ID_SPI_BASE  + (n)*IRQ_SPI_EVENTS  + 2))
#define IRQ_ID_SPI_ERROR(n)      ((irq_id_t)(IRQ_ID_SPI_BASE  + (n)*IRQ_SPI_EVENTS  + 3))

/** IRQ_ID_EXTI(pin) — GPIO EXTI line interrupt, pin in [0, 15]. */
#define IRQ_ID_EXTI(pin)         ((irq_id_t)(IRQ_ID_EXTI_BASE + (pin)))

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
