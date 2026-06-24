/**
 * @file        ethernetif.h
 * @brief       lwIP netif glue for the STM32H7 Ethernet MAC
 * @ingroup     net
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Network
 * @info        Bridges an lwIP struct netif to the drv_eth.h frame API. The
 *              RX path is pumped by ethernetif_poll(), called from the net
 *              service thread when the ETH RX interrupt wakes it.
 * @dependency  lwIP (netif), drv_eth.h
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

#ifndef NET_ETHERNETIF_H_
#define NET_ETHERNETIF_H_

#include "lwip/err.h"
#include "lwip/netif.h"

/**
 * @brief lwIP netif init callback: configures the netif and brings up the MAC.
 * @note  Pass this as the @c init argument to netif_add().
 * @return ERR_OK on success, ERR_IF if the MAC failed to initialise.
 */
err_t ethernetif_init(struct netif *netif);

/**
 * @brief Drain all pending RX frames into the stack.
 * @note  Called repeatedly from the net service poll loop. Each frame is
 *        copied into a pbuf and handed to @c netif->input (tcpip_input).
 * @return Number of frames drained this call.
 */
uint32_t ethernetif_poll(struct netif *netif);

#endif /* NET_ETHERNETIF_H_ */
