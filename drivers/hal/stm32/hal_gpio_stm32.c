/*
 * hal_gpio_stm32.c — STM32 HAL GPIO ops implementation
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <config/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_handle.h>
#include <drivers/cpu/hal/stm32/hal_gpio_stm32.h>

/* ── HAL ops implementations ──────────────────────────────────────────── */

static int32_t stm32_gpio_hw_init(drv_gpio_handle_t *h)
{
    if (h == NULL || h->hw.port == NULL)
        return OS_ERR_OP;

    /* Enable the GPIO clock via the HAL RCC macro */
    /* NOTE: Applications must enable the peripheral clock before calling
     * hw_init.  The generic GPIO driver cannot know which clock to enable
     * without hard-coding port-to-clock mappings.  A board-specific
     * gpio_clock_enable() hook in stm32f4xx_hal_msp.c is the correct place. */

    GPIO_InitTypeDef cfg = {
        .Pin   = h->hw.pin,
        .Mode  = GPIO_MODE_OUTPUT_PP,
        .Pull  = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };

    HAL_GPIO_Init(h->hw.port, &cfg);
    h->initialized = 1;
    return OS_ERR_NONE;
}

static int32_t stm32_gpio_hw_deinit(drv_gpio_handle_t *h)
{
    if (h == NULL || h->hw.port == NULL)
        return OS_ERR_OP;

    HAL_GPIO_DeInit(h->hw.port, h->hw.pin);
    h->initialized = 0;
    return OS_ERR_NONE;
}

static void stm32_gpio_set(drv_gpio_handle_t *h)
{
    if (h != NULL && h->initialized)
        HAL_GPIO_WritePin(h->hw.port, h->hw.pin, GPIO_PIN_SET);
}

static void stm32_gpio_clear(drv_gpio_handle_t *h)
{
    if (h != NULL && h->initialized)
        HAL_GPIO_WritePin(h->hw.port, h->hw.pin, GPIO_PIN_RESET);
}

static void stm32_gpio_toggle(drv_gpio_handle_t *h)
{
    if (h != NULL && h->initialized)
        HAL_GPIO_TogglePin(h->hw.port, h->hw.pin);
}

static uint8_t stm32_gpio_read(drv_gpio_handle_t *h)
{
    if (h == NULL || !h->initialized)
        return 0;
    return (uint8_t)HAL_GPIO_ReadPin(h->hw.port, h->hw.pin);
}

static void stm32_gpio_write(drv_gpio_handle_t *h, uint8_t state)
{
    if (h == NULL || !h->initialized)
        return;
    HAL_GPIO_WritePin(h->hw.port, h->hw.pin,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ── Static ops table ─────────────────────────────────────────────────── */

static const drv_gpio_hal_ops_t _stm32_gpio_ops = {
    .hw_init   = stm32_gpio_hw_init,
    .hw_deinit = stm32_gpio_hw_deinit,
    .set       = stm32_gpio_set,
    .clear     = stm32_gpio_clear,
    .toggle    = stm32_gpio_toggle,
    .read      = stm32_gpio_read,
    .write     = stm32_gpio_write,
};

/* ── Public API ───────────────────────────────────────────────────────── */

const drv_gpio_hal_ops_t *hal_gpio_stm32_get_ops(void)
{
    return &_stm32_gpio_ops;
}

void hal_gpio_stm32_set_config(drv_gpio_handle_t *h,
                                GPIO_TypeDef      *port,
                                uint16_t           pin,
                                uint32_t           mode,
                                uint32_t           pull,
                                uint32_t           speed,
                                uint8_t            active_state)
{
    if (h == NULL || port == NULL)
        return;

    h->hw.port         = port;
    h->hw.pin          = pin;
    h->hw.active_state = active_state;

    /* Store init params in a local struct; the hw_init call will re-apply them.
     * We keep mode/pull/speed in the hw context by repurposing the GPIO_InitTypeDef
     * that will be rebuilt in hw_init — for simplicity we apply them here directly
     * so the caller gets the correct configuration even before hw_init. */
    GPIO_InitTypeDef cfg = {
        .Pin   = pin,
        .Mode  = mode,
        .Pull  = pull,
        .Speed = speed,
    };
    HAL_GPIO_Init(port, &cfg);
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
