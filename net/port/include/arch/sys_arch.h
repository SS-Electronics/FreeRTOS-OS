/**
 * @file        sys_arch.h
 * @brief       lwIP OS abstraction types for the FreeRTOS-OS kernel port
 * @ingroup     net_port
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Network Port
 * @info        Declares the sys_sem_t / sys_mutex_t / sys_mbox_t / sys_thread_t
 *              handle types lwIP's threaded mode (NO_SYS=0) builds upon. All
 *              types alias the opaque os_*_t handles from kernel.h, so the
 *              port never includes FreeRTOS headers directly.
 * @dependency  os/kernel.h
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

#ifndef NET_PORT_ARCH_SYS_ARCH_H_
#define NET_PORT_ARCH_SYS_ARCH_H_

#include <os/kernel.h>

/**
 * @brief lwIP handle types mapped onto the kernel IPC primitives.
 * @note  A NULL handle denotes "invalid"; the sys_*_valid()/set_invalid()
 *        helpers in sys_arch.c rely on this.
 */
typedef os_sem_t   sys_sem_t;
typedef os_mutex_t sys_mutex_t;
typedef os_mbox_t  sys_mbox_t;

/**
 * @brief Thread handle: the positive thread_id returned by os_thread_create().
 */
typedef int32_t    sys_thread_t;

/**
 * @brief Critical-section nesting token returned by sys_arch_protect().
 */
typedef uint32_t   sys_prot_t;

/* The handle types above are pointers/integers, so lwIP can null-check them
 * directly. Tell lwIP we provide our own valid/invalid helpers as functions. */
#define LWIP_SYS_ARCH_TIMEOUT  0xffffffffUL

#endif /* NET_PORT_ARCH_SYS_ARCH_H_ */
