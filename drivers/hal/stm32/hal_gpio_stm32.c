/*
 * hal_gpio_stm32.c — STM32 HAL GPIO ops implementation
 *
 * This file is part of FreeRTOS-OS Project.
 */

#include <board/mcu_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_handle.h>
#include <drivers/cpu/hal/stm32/hal_gpio_stm32.h>

/* ── HAL ops implementations ──────────────────────────────────────────── */

static int32_t stm32_gpio_hw_init(drv_gpio_handle_t *h)
{
    if (h == NULL || h->hw.port == NULL)
        return OS_ERR_OP;

    /*
     * mode/pull/speed were stored in the hw context by hal_gpio_stm32_set_config().
     * board_gpio_clk_enable() must be called before this (done by gpio_mgmt).
     */
    GPIO_InitTypeDef cfg = {
        .Pin   = h->hw.pin,
        .Mode  = h->hw.mode  ? h->hw.mode  : GPIO_MODE_OUTPUT_PP,
        .Pull  = h->hw.pull,
        .Speed = h->hw.speed ? h->hw.speed : GPIO_SPEED_FREQ_LOW,
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

    /* Store all parameters in the hw context.
     * hw_init() will call HAL_GPIO_Init() using these values.
     * Caller must invoke board_gpio_clk_enable(port) before drv_gpio_register(). */
    h->hw.port         = port;
    h->hw.pin          = pin;
    h->hw.mode         = mode;
    h->hw.pull         = pull;
    h->hw.speed        = speed;
    h->hw.active_state = active_state;
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */
