/*
 * hal_uart_stm32.c — STM32 HAL UART ops implementation
 *
 * This file is part of FreeRTOS-OS Project.
 *
 * This translation unit implements the drv_uart_hal_ops_t vtable for the
 * STM32 HAL backend.  It is compiled only when CONFIG_DEVICE_VARIANT is
 * MCU_VAR_STM.  All STM32 HAL calls are isolated here — the generic driver
 * layer (drv_uart.c) and higher layers never call HAL functions directly.
 */

#include <config/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_handle.h>
#include <drivers/com/hal/stm32/hal_uart_stm32.h>

/* ── Internal helpers ─────────────────────────────────────────────────── */

static int32_t _hal_to_os_err(HAL_StatusTypeDef s)
{
    return (s == HAL_OK) ? OS_ERR_NONE : OS_ERR_OP;
}

/* ── HAL ops implementations ──────────────────────────────────────────── */

static int32_t stm32_uart_hw_init(drv_uart_handle_t *h)
{
    if (h == NULL || h->ops == NULL)
        return OS_ERR_OP;

    UART_HandleTypeDef *huart = &h->hw.huart;

    /* Baudrate is owned by the generic handle; copy it into the HAL struct */
    huart->Init.BaudRate = h->baudrate;

    HAL_StatusTypeDef ret = HAL_UART_Init(huart);
    if (ret == HAL_OK)
        h->initialized = 1;

    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static int32_t stm32_uart_hw_deinit(drv_uart_handle_t *h)
{
    if (h == NULL)
        return OS_ERR_OP;

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
    if (h == NULL || !h->initialized || rx_byte == NULL)
        return OS_ERR_OP;

    HAL_StatusTypeDef ret = UART_Start_Receive_IT(&h->hw.huart, rx_byte, 1);
    h->last_err = _hal_to_os_err(ret);
    return h->last_err;
}

static void stm32_uart_rx_isr_cb(drv_uart_handle_t *h,
                                  uint8_t            rx_byte,
                                  void              *rb)
{
    /* Re-arm interrupt for next byte */
    if (h != NULL && h->initialized)
        HAL_UART_Receive_IT(&h->hw.huart, &rx_byte, 1);

    /* Push byte into ring buffer if provided */
    if (rb != NULL)
    {
        /* Ring-buffer API: ringbuffer_putchar(rb, byte) */
        extern void ringbuffer_putchar(void *rb, uint8_t ch);
        ringbuffer_putchar(rb, rx_byte);
    }
}

/* ── Static ops table ─────────────────────────────────────────────────── */

static const drv_uart_hal_ops_t _stm32_uart_ops = {
    .hw_init     = stm32_uart_hw_init,
    .hw_deinit   = stm32_uart_hw_deinit,
    .transmit    = stm32_uart_transmit,
    .receive     = stm32_uart_receive,
    .start_rx_it = stm32_uart_start_rx_it,
    .rx_isr_cb   = stm32_uart_rx_isr_cb,
};

/* ── Public API ───────────────────────────────────────────────────────── */

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
    /* BaudRate will be set from h->baudrate in hw_init */
}

/* ── IRQ dispatch ─────────────────────────────────────────────────────── */

void hal_uart_stm32_irq_handler(USART_TypeDef *instance)
{
    for (uint8_t id = 0; id < NO_OF_UART; id++)
    {
        drv_uart_handle_t *h = drv_uart_get_handle(id);
        if (h != NULL && h->initialized && h->hw.huart.Instance == instance)
        {
            HAL_UART_IRQHandler(&h->hw.huart);
            return;
        }
    }
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
