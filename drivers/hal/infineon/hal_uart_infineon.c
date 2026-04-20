/*
 * hal_uart_infineon.c — Infineon XMC series baremetal UART ops (stubs)
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * All functions are stubs.  Replace the bodies with actual Infineon USIC-UART
 * register sequences when porting.  The USIC peripheral is memory-mapped;
 * channel_base points to the USIC channel register block.
 *
 * Infineon XMC reference:
 *   XMC4500 Reference Manual, Chapter 16 – Universal Serial Interface Channel
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)

#include <drivers/drv_handle.h>
#include <drivers/com/hal/infineon/hal_uart_infineon.h>

/* ── Stub helpers ─────────────────────────────────────────────────────── */

/* Cast channel_base to a volatile uint32_t pointer array and access registers
 * by offset.  Real implementation would use the XMC SDK structs or CMSIS-style
 * register definitions from the Infineon device header. */
#define USIC_CH(base)   ((volatile uint32_t *)(base))

/* ── HAL ops stubs ────────────────────────────────────────────────────── */

static int32_t infineon_uart_hw_init(drv_uart_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    /*
     * TODO: Configure USIC channel for UART mode:
     *   1. Enable USIC module clock via SCU_CLK.
     *   2. Set baud rate dividers (BRGO/DCTQ/PDIV) for h->baudrate.
     *   3. Set frame format: 8 data bits, 1 stop bit, no parity.
     *   4. Configure TX/RX FIFO or DMA as needed.
     *   5. Enable the channel.
     */
    (void)h;

    h->initialized = 1;
    h->last_err    = OS_ERR_NONE;
    return OS_ERR_NONE;
}

static int32_t infineon_uart_hw_deinit(drv_uart_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    /* TODO: Disable USIC channel. */
    h->initialized = 0;
    h->last_err    = OS_ERR_NONE;
    return OS_ERR_NONE;
}

static int32_t infineon_uart_transmit(drv_uart_handle_t *h,
                                       const uint8_t     *data,
                                       uint16_t           len,
                                       uint32_t           timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    /*
     * TODO: Write each byte to USIC TBUF (transmit buffer register).
     *   while (len--) {
     *       while (!(USIC_CH(h->hw.channel_base)[TBUF_STAT] & TBUF_READY));
     *       USIC_CH(h->hw.channel_base)[TBUF] = *data++;
     *   }
     */
    (void)data;
    (void)len;
    (void)timeout_ms;

    return OS_ERR_NONE;
}

static int32_t infineon_uart_receive(drv_uart_handle_t *h,
                                      uint8_t           *data,
                                      uint16_t           len,
                                      uint32_t           timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    /*
     * TODO: Read each byte from USIC RBUF (receive buffer register).
     *   while (len--) {
     *       while (!(USIC_CH(h->hw.channel_base)[RBUF_STAT] & RBUF_VALID));
     *       *data++ = (uint8_t)USIC_CH(h->hw.channel_base)[RBUF];
     *   }
     */
    (void)data;
    (void)len;
    (void)timeout_ms;

    return OS_ERR_NONE;
}

static int32_t infineon_uart_start_rx_it(drv_uart_handle_t *h, uint8_t *rx_byte)
{
    if (h == NULL || !h->initialized || rx_byte == NULL)
        return OS_ERR_OP;

    /* TODO: Enable receive interrupt on USIC channel. */
    (void)rx_byte;
    return OS_ERR_NONE;
}

static void infineon_uart_rx_isr_cb(drv_uart_handle_t *h,
                                     uint8_t            rx_byte,
                                     void              *rb)
{
    /* TODO: Read the received byte and push to ring buffer. */
    (void)h;
    (void)rx_byte;
    (void)rb;
}

/* ── Static ops table ─────────────────────────────────────────────────── */

static const drv_uart_hal_ops_t _infineon_uart_ops = {
    .hw_init     = infineon_uart_hw_init,
    .hw_deinit   = infineon_uart_hw_deinit,
    .transmit    = infineon_uart_transmit,
    .receive     = infineon_uart_receive,
    .start_rx_it = infineon_uart_start_rx_it,
    .rx_isr_cb   = infineon_uart_rx_isr_cb,
};

/* ── Public API ───────────────────────────────────────────────────────── */

const drv_uart_hal_ops_t *hal_uart_infineon_get_ops(void)
{
    return &_infineon_uart_ops;
}

void hal_uart_infineon_set_config(drv_uart_handle_t *h,
                                  uint32_t           channel_base)
{
    if (h == NULL)
        return;

    h->hw.channel_base = channel_base;
    h->hw.baudrate     = h->baudrate;
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON */
