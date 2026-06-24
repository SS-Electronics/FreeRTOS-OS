/**
 * @file        net_service.h
 * @brief       Network bring-up service (lwIP + static IP + RX poll thread)
 * @ingroup     net
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Network
 * @info        Single entry point the application calls to start the TCP/IP
 *              stack. Initialises lwIP, attaches the STM32H7 netif with a
 *              static IPv4 address and spawns the RX polling thread that
 *              services incoming frames (e.g. ICMP echo / ping).
 * @dependency  lwIP (tcpip), net/ethernetif.h, os/kernel.h
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

#ifndef NET_NET_SERVICE_H_
#define NET_NET_SERVICE_H_

#include <def_std.h>
#include <def_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the network stack: lwIP init, netif up, RX poll thread.
 * @note  Call from app_main() before the scheduler starts. The MAC PHY
 *        auto-negotiation inside this call can block for up to ~5 s if the
 *        cable is unplugged; a failure is non-fatal to the rest of the system.
 * @return OS_ERR_NONE on success, OS_ERR_OP if the netif failed to attach.
 */
int32_t net_service_start(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_NET_SERVICE_H_ */
