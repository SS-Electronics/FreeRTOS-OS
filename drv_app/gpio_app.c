/**
 * @file        gpio_app.c
 * @brief       gpio_app.c — Application-level GPIO sync / async operations
 * @ingroup     drv_app
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Driver App
 * @info        Application-facing helpers that wrap driver init for board-listed peripherals.
 * @dependency  Driver layer, board BSP
 *
 * @details
 * gpio_app.c — Application-level GPIO sync / async operations
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

#include <drv_app/gpio_app.h>
#include <drivers/drv_gpio.h>

/* ── Immediate (direct driver) ────────────────────────────────────────── */

uint8_t gpio_read(uint8_t gpio_id)
{
    drv_gpio_handle_t *h = drv_gpio_get_handle(gpio_id);
    if (h == NULL || h->ops == NULL || !h->initialized)
        return 0xFFU;
    return h->ops->read(h);
}

/* ── Asynchronous via management queue ───────────────────────────────── */

int32_t gpio_async_set(uint8_t gpio_id)
{
    return gpio_mgmt_post(gpio_id, GPIO_MGMT_CMD_SET, 0, 0);
}

int32_t gpio_async_clear(uint8_t gpio_id)
{
    return gpio_mgmt_post(gpio_id, GPIO_MGMT_CMD_CLEAR, 0, 0);
}

int32_t gpio_async_toggle(uint8_t gpio_id)
{
    return gpio_mgmt_post(gpio_id, GPIO_MGMT_CMD_TOGGLE, 0, 0);
}

int32_t gpio_async_write(uint8_t gpio_id, uint8_t state)
{
    return gpio_mgmt_post(gpio_id, GPIO_MGMT_CMD_WRITE, state, 0);
}

int32_t gpio_async_delayed(uint8_t gpio_id, gpio_mgmt_cmd_t cmd,
                            uint8_t state, uint32_t delay_ms)
{
    return gpio_mgmt_post(gpio_id, cmd, state, delay_ms);
}
