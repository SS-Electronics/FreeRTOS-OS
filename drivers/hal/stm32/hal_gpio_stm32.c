/**
 * @file    hal_gpio_stm32.c
 * @author  Subhajit Roy (subhajitroy005@gmail.com)
 *
 * @module  drivers
 * @brief   STM32 HAL GPIO operations implementation
 *
 * @details
 * This file implements the GPIO hardware abstraction layer (HAL) backend
 * for STM32 devices. It provides a concrete implementation of the
 * drv_gpio_hal_ops_t interface used by the generic GPIO driver layer.
 *
 * The implementation supports:
 * - GPIO initialization and deinitialization
 * - Pin state control (set, clear, toggle, write)
 * - Pin state reading
 *
 * Configuration parameters are stored in the driver handle and applied
 * during hardware initialization.
 *
 * GPIO clock enable must be handled externally (typically by gpio_mgmt)
 * before invoking drv_gpio_register().
 *
 * @dependencies
 * drv_gpio.h, hal_gpio_stm32.h, mcu_config.h
 *
 * @note
 * This file is part of FreeRTOS-OS Project.
 *
 * @license
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
 * You should have received a copy of the GNU General Public License
 * along with FreeRTOS-OS. If not, see <https://www.gnu.org/licenses/>.
 */

#include <board/board_config.h>

#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)

#include <drivers/drv_gpio.h>
#include <drivers/hal/stm32/hal_gpio_stm32.h>

/* ── HAL ops implementations ──────────────────────────────────────────── */

/**
 * @brief Initialize GPIO hardware
 *
 * @param h Pointer to GPIO handle
 *
 * @retval OS_ERR_NONE on success
 * @retval OS_ERR_OP   on failure
 *
 * @details
 * Configures the GPIO pin using parameters stored in the handle.
 * Default values are applied if mode or speed are not explicitly set.
 *
 * @pre GPIO clock must be enabled before calling this function
 */
__SECTION_OS __USED
static int32_t stm32_gpio_hw_init(drv_gpio_handle_t *h)
{
    if (h == NULL || h->hw.port == NULL)
        return OS_ERR_OP;

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

/**
 * @brief Deinitialize GPIO hardware
 *
 * @param h Pointer to GPIO handle
 *
 * @retval OS_ERR_NONE on success
 * @retval OS_ERR_OP   on failure
 */
__SECTION_OS __USED
static int32_t stm32_gpio_hw_deinit(drv_gpio_handle_t *h)
{
    if (h == NULL || h->hw.port == NULL)
        return OS_ERR_OP;

    HAL_GPIO_DeInit(h->hw.port, h->hw.pin);
    h->initialized = 0;
    return OS_ERR_NONE;
}

/**
 * @brief Set GPIO pin to active state
 *
 * @param h Pointer to GPIO handle
 */
__SECTION_OS __USED
static void stm32_gpio_set(drv_gpio_handle_t *h)
{
    if (h != NULL && h->initialized)
        HAL_GPIO_WritePin(h->hw.port, h->hw.pin, GPIO_PIN_SET);
}

/**
 * @brief Clear GPIO pin to inactive state
 *
 * @param h Pointer to GPIO handle
 */
__SECTION_OS __USED
static void stm32_gpio_clear(drv_gpio_handle_t *h)
{
    if (h != NULL && h->initialized)
        HAL_GPIO_WritePin(h->hw.port, h->hw.pin, GPIO_PIN_RESET);
}

/**
 * @brief Toggle GPIO pin state
 *
 * @param h Pointer to GPIO handle
 */
__SECTION_OS __USED
static void stm32_gpio_toggle(drv_gpio_handle_t *h)
{
    if (h != NULL && h->initialized)
        HAL_GPIO_TogglePin(h->hw.port, h->hw.pin);
}

/**
 * @brief Read GPIO pin state
 *
 * @param h Pointer to GPIO handle
 *
 * @retval 0 Pin is low or invalid handle
 * @retval 1 Pin is high
 */
__SECTION_OS __USED
static uint8_t stm32_gpio_read(drv_gpio_handle_t *h)
{
    if (h == NULL || !h->initialized)
        return 0;

    return (uint8_t)HAL_GPIO_ReadPin(h->hw.port, h->hw.pin);
}

/**
 * @brief Write GPIO pin state
 *
 * @param h     Pointer to GPIO handle
 * @param state Desired pin state (0 = low, non-zero = high)
 */
__SECTION_OS __USED
static void stm32_gpio_write(drv_gpio_handle_t *h, uint8_t state)
{
    if (h == NULL || !h->initialized)
        return;

    HAL_GPIO_WritePin(h->hw.port, h->hw.pin,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ── Static ops table ─────────────────────────────────────────────────── */

/**
 * @brief STM32 GPIO HAL operations table
 *
 * @details
 * Provides the concrete implementation of drv_gpio_hal_ops_t
 * for STM32 platform.
 */
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

/**
 * @brief Get STM32 GPIO HAL operations
 *
 * @return Pointer to GPIO HAL ops structure
 */
__SECTION_OS __USED
const drv_gpio_hal_ops_t *hal_gpio_stm32_get_ops(void)
{
    return &_stm32_gpio_ops;
}

/**
 * @brief Configure GPIO handle parameters
 *
 * @param h             Pointer to GPIO handle
 * @param port          GPIO port base address
 * @param pin           GPIO pin number
 * @param mode          GPIO mode (input/output/alternate)
 * @param pull          Pull-up/pull-down configuration
 * @param speed         Output speed
 * @param active_state  Logical active state (1 = active high, 0 = active low)
 *
 * @details
 * Stores configuration into the handle. Actual hardware initialization
 * is performed later by stm32_gpio_hw_init().
 *
 * @pre board_gpio_clk_enable(port) must be called before registration
 */
__SECTION_OS __USED
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
    h->hw.mode         = mode;
    h->hw.pull         = pull;
    h->hw.speed        = speed;
    h->hw.active_state = active_state;
}

#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */