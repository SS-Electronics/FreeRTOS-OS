/**
 * @file        uart_app.c
 * @brief       uart_app.c — Application-level UART sync / async operations
 * @ingroup     drv_app
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Driver App
 * @info        Application-facing helpers that wrap driver init for board-listed peripherals.
 * @dependency  Driver layer, board BSP
 *
 * @details
 * uart_app.c — Application-level UART sync / async operations
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

#include <drv_app/uart_app.h>
#include <services/uart_mgmt.h>
#include <os/kernel.h>

#include "FreeRTOS.h"
#include "task.h"

/* ── Synchronous ──────────────────────────────────────────────────────── */

int32_t uart_sync_transmit(uint8_t dev_id, const uint8_t *data,
                            uint16_t len, uint32_t timeout_ms)
{
    QueueHandle_t q = uart_mgmt_get_queue();
    if (q == NULL || data == NULL || len == 0)
        return OS_ERR_OP;

    int32_t result = OS_ERR_OP;

    uart_mgmt_msg_t msg = {
        .cmd           = UART_MGMT_CMD_TRANSMIT,
        .dev_id        = dev_id,
        .data          = data,
        .len           = len,
        .result_notify = xTaskGetCurrentTaskHandle(),
        .result_code   = &result,
    };

    if (xQueueSend(q, &msg, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)
        return OS_ERR_OP;

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return result;
}

int32_t uart_sync_receive(uint8_t dev_id, uint8_t *buf,
                           uint16_t len, uint32_t timeout_ms)
{
    if (buf == NULL || len == 0)
        return OS_ERR_OP;

    uint32_t elapsed = 0;
    uint16_t received = 0;

    while (received < len)
    {
        if (uart_mgmt_read_byte(dev_id, &buf[received]) == OS_ERR_NONE)
        {
            received++;
        }
        else
        {
            if (elapsed >= timeout_ms)
                return OS_ERR_OP;
            os_thread_delay(1);
            elapsed++;
        }
    }

    return OS_ERR_NONE;
}

/* ── Asynchronous ────────────────────────────────────────────────────── */

int32_t uart_async_transmit(uint8_t dev_id, const uint8_t *data, uint16_t len)
{
    return uart_mgmt_async_transmit(dev_id, data, len);
}

int32_t uart_async_read_byte(uint8_t dev_id, uint8_t *byte)
{
    return uart_mgmt_read_byte(dev_id, byte);
}
