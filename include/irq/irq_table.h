/**
 * @file        irq_table.h
 * @brief       irq_table.h — Static IRQ descriptor table declarations
 * @ingroup     irq
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      IRQ Subsystem
 * @info        Linux-style irq_desc chain, software IRQ table, request_irq/free_irq, NVIC chip driver.
 * @dependency  Generated irq_table.c, NVIC chip
 *
 * @details
 * irq_table.h — Static IRQ descriptor table declarations
 *
 * irq_table.c holds a human-readable name for every IRQ ID defined in
 * irq_notify.h.  The table is the canonical list of all software IRQ IDs
 * in the system and is intended to be auto-generated from a hardware
 * description file when the peripheral set grows.
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

#ifndef INCLUDE_IRQ_IRQ_TABLE_H_
#define INCLUDE_IRQ_IRQ_TABLE_H_

#include <irq/irq_notify.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Return the human-readable name for an IRQ ID.
 * @return Static string (never NULL). Returns "INVALID" for out-of-range IDs
 *         and "UNKNOWN" for IDs with no assigned name.
 */
const char *irq_table_get_name(irq_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_IRQ_IRQ_TABLE_H_ */
