/*
 * hal_uart_stm32.c — STM32 HAL UART ops implementation
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * Implements the drv_uart_hal_ops_t vtable for the STM32 HAL backend.
 * Compiled only when CONFIG_DEVICE_VARIANT is MCU_VAR_STM.
 *
 * IRQ handling — hal_uart_stm32_irq_handler():
 *   Reads USART SR/CR1/CR3 registers directly and dispatches events without
 *   going through the STM32 HAL callback chain.  On each RXNE interrupt the
 *   received byte is read from DR and forwarded via two paths:
 *     1. drv_irq_dispatch_from_isr(IRQ_ID_UART_RX(n)) — management tasks
 *        subscribe to this via drv_irq_register().
 *     2. drv_uart_rx_isr_dispatch(n, byte) — puts the byte into the IPC ring
 *        buffer and calls any board-level on_rx_byte hook.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_handle.h>
#include <drivers/com/hal/stm32/hal_uart_stm32.h>
#include <drivers/drv_irq.h>

/* Forward declaration — defined in drv_uart.c */
extern void drv_uart_rx_isr_dispatch(uint8_t dev_id, uint8_t rx_byte);

/* ── Internal helpers ─────────────────────────────────────────────────────── */

static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

/* ── HAL ops implementations ──────────────────────────────────────────────── */

static int32_t stm32_uart_hw_init(drv_uart_handle_t *h)
{
    if (h == NULL || h->ops == NULL)
        return OS_ERR_OP;

    UART_HandleTypeDef *huart = &h->hw.huart;

    huart->Init.BaudRate = h->baudrate;

    HAL_StatusTypeDef ret = HAL_UART_Init(huart);
    if (ret == HAL_OK)
    {
        h->initialized = 1;
        /* Enable RXNE interrupt directly — no HAL state machine involved */
        SET_BIT(huart->Instance->CR1, USART_CR1_RXNEIE);
    }

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_uart_hw_deinit(drv_uart_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

    CLEAR_BIT(h->hw.huart.Instance->CR1, USART_CR1_RXNEIE);
    HAL_StatusTypeDef ret = HAL_UART_DeInit(&h->hw.huart);
    h->initialized = 0;
    h->last_err    = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_uart_transmit(drv_uart_handle_t *h,
                                   const uint8_t     *data,
                                   uint16_t           len,
                                   uint32_t           timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_UART_Transmit(&h->hw.huart,
                                              (uint8_t *)data,
                                              len,
                                              timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_uart_receive(drv_uart_handle_t *h,
                                  uint8_t           *data,
                                  uint16_t           len,
                                  uint32_t           timeout_ms)
{
    if (h == NULL || !h->initialized || data == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = HAL_UART_Receive(&h->hw.huart, data, len, timeout_ms);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_uart_start_rx_it(drv_uart_handle_t *h, uint8_t *rx_byte)
{
    (void)rx_byte; /* byte is read directly from DR in the IRQ handler */
    if (h == NULL || !h->initialized)
        return OS_ERR_OP;

    SET_BIT(h->hw.huart.Instance->CR1, USART_CR1_RXNEIE);
    return OS_ERR_NONE;
}

static void stm32_uart_rx_isr_cb(drv_uart_handle_t *h,
                                  uint8_t            rx_byte,
                                  void              *rb)
{
    /* Push byte into ring buffer if provided */
    if (rb != NULL)
    {
        extern void ringbuffer_putchar(void *rb, uint8_t ch);
        ringbuffer_putchar(rb, rx_byte);
    }
    (void)h;
}

/* ── Static ops table ─────────────────────────────────────────────────────── */

static const drv_uart_hal_ops_t _stm32_uart_ops = {
    .hw_init     = stm32_uart_hw_init,
    .hw_deinit   = stm32_uart_hw_deinit,
    .transmit    = stm32_uart_transmit,
    .receive     = stm32_uart_receive,
    .start_rx_it = stm32_uart_start_rx_it,
    .rx_isr_cb   = stm32_uart_rx_isr_cb,
};

/* ── Public API ───────────────────────────────────────────────────────────── */

const drv_uart_hal_ops_t *hal_uart_stm32_get_ops(void)
{
    return &_stm32_uart_ops;
}

void hal_uart_stm32_set_config(drv_uart_handle_t *h,
                               USART_TypeDef     *instance,
                               uint32_t           word_len,
                               uint32_t           stop_bits,
                               uint32_t           parity,
                               uint32_t           mode)
{
    if (h == NULL || instance == NULL)
        return;

    UART_HandleTypeDef *huart = &h->hw.huart;

    huart->Instance          = instance;
    huart->Init.WordLength   = word_len;
    huart->Init.StopBits     = stop_bits;
    huart->Init.Parity       = parity;
    huart->Init.Mode         = mode;
    huart->Init.HwFlowCtl   = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
}

/* ── IRQ dispatch ─────────────────────────────────────────────────────────── */

/*
 * hal_uart_stm32_irq_handler — direct USART register-level IRQ handler.
 *
 * Reads SR, CR1, CR3 and dispatches events to the drv_irq framework without
 * going through the STM32 HAL callback chain (no HAL_UART_IRQHandler call).
 *
 * RX path:
 *   RXNE set → read DR (clears RXNE) → drv_irq_dispatch_from_isr(RX)
 *                                    → drv_uart_rx_isr_dispatch() (ring buf)
 * Error path:
 *   Read DR to clear SR error flags, dispatch RX byte if RXNE was also set,
 *   then dispatch ERROR event.
 * TX path:
 *   TXE/TC → clear interrupt enable bit → dispatch TX_DONE event.
 */
void hal_uart_stm32_irq_handler(USART_TypeDef *instance)
{
    for (uint8_t id = 0; id < NO_OF_UART; id++)
    {
        drv_uart_handle_t *h = drv_uart_get_handle(id);
        if (h == NULL || !h->initialized || h->hw.huart.Instance != instance)
            continue;

        uint32_t sr         = READ_REG(instance->SR);
        uint32_t cr1        = READ_REG(instance->CR1);
        uint32_t cr3        = READ_REG(instance->CR3);
        uint32_t errorflags = sr & (USART_SR_PE | USART_SR_FE |
                                    USART_SR_ORE | USART_SR_NE);
        BaseType_t hpt = pdFALSE;

        /* ── No errors: handle RX ─────────────────────────────────────────── */
        if (errorflags == 0U)
        {
            if ((sr & USART_SR_RXNE) && (cr1 & USART_CR1_RXNEIE))
            {
                uint8_t byte = (uint8_t)(READ_REG(instance->DR) & 0xFFU);
                drv_irq_dispatch_from_isr(IRQ_ID_UART_RX(id), &byte, &hpt);
                drv_uart_rx_isr_dispatch(id, byte);
                portYIELD_FROM_ISR(hpt);
                return;
            }
        }

        /* ── Error path ───────────────────────────────────────────────────── */
        if (errorflags && ((cr3 & USART_CR3_EIE) ||
                           (cr1 & (USART_CR1_RXNEIE | USART_CR1_PEIE))))
        {
            /* SR read above + DR read clears error flags on STM32F4 */
            uint8_t byte = (uint8_t)(READ_REG(instance->DR) & 0xFFU);

            if (sr & USART_SR_RXNE)
            {
                drv_irq_dispatch_from_isr(IRQ_ID_UART_RX(id), &byte, &hpt);
                drv_uart_rx_isr_dispatch(id, byte);
            }

            drv_irq_dispatch_from_isr(IRQ_ID_UART_ERROR(id), &errorflags, &hpt);
            portYIELD_FROM_ISR(hpt);
            return;
        }

        /* ── TX empty ─────────────────────────────────────────────────────── */
        if ((sr & USART_SR_TXE) && (cr1 & USART_CR1_TXEIE))
        {
            CLEAR_BIT(instance->CR1, USART_CR1_TXEIE);
            drv_irq_dispatch_from_isr(IRQ_ID_UART_TX_DONE(id), NULL, &hpt);
            portYIELD_FROM_ISR(hpt);
            return;
        }

        /* ── Transmit complete ────────────────────────────────────────────── */
        if ((sr & USART_SR_TC) && (cr1 & USART_CR1_TCIE))
        {
            CLEAR_BIT(instance->CR1, USART_CR1_TCIE);
            drv_irq_dispatch_from_isr(IRQ_ID_UART_TX_DONE(id), NULL, &hpt);
            portYIELD_FROM_ISR(hpt);
            return;
        }

        return;
    }
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
