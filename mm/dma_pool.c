/**
 * @file        dma_pool.c
 * @brief       dma pool
 * @ingroup     mm
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Memory Mgmt
 * @info        Heap helpers, intrusive doubly-linked list (Linux-style), DMA pool allocator.
 * @dependency  FreeRTOS heap_4, list.h
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

#include "dma_pool.h"




