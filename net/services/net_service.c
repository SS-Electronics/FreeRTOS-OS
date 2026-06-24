/**
 * @file        net_service.c
 * @brief       Network bring-up service: lwIP, static IPv4 netif, RX poll loop
 * @ingroup     net
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Network
 * @info        Brings the TCP/IP stack online for the NUCLEO-H723ZG ping demo.
 *              Runs tcpip_init(), attaches the STM32H7 Ethernet netif at a
 *              fixed address and starts a thread that blocks on the ETH RX
 *              interrupt and drains received frames into the stack. With
 *              LWIP_ICMP enabled the board answers ping out of the box.
 * @dependency  lwIP (tcpip, netif), net/ethernetif.h, os/kernel.h
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

#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

#include <net/net_service.h>
#include <net/ethernetif.h>
#include <drivers/drv_eth.h>
#include <os/kernel.h>
#include <os/kernel_syscall.h>

/* Static network identity (IP / netmask / gateway / MAC) lives in lwipopts.h
 * so the whole network configuration sits in one file. */

/* RX thread: priority just below tcpip_thread so frames drain promptly. */
#define NET_RX_THREAD_STACK     512     /* words */
#define NET_RX_THREAD_PRIO      9
#define NET_RX_WAKE_TIMEOUT_MS  1000    /* periodic wake to re-check link */

static struct netif g_netif;

/**
 * @brief Interrupt-driven RX: block on the ETH RX signal, then drain frames.
 * @note  drv_eth_rx_wait() sleeps until HAL_ETH_RxCpltCallback fires (or the
 *        timeout lapses); ethernetif_poll() then pulls every queued frame.
 *        The timeout is a safety net, not a poll — it costs one wake per second
 *        when idle and lets the loop survive a missed interrupt.
 */
static void net_rx_thread(void *arg)
{
	struct netif *nif = (struct netif *)arg;

	printk("[net] up: ip=%d.%d.%d.%d link=%ld\n",
	       NET_IP_ADDR0, NET_IP_ADDR1, NET_IP_ADDR2, NET_IP_ADDR3,
	       (long)drv_eth_link_is_up());

	for (;;)
	{
		(void)drv_eth_rx_wait(NET_RX_WAKE_TIMEOUT_MS);
		(void)ethernetif_poll(nif);
	}
}

int32_t net_service_start(void)
{
	ip4_addr_t ipaddr;
	ip4_addr_t netmask;
	ip4_addr_t gateway;

	IP4_ADDR(&ipaddr,  NET_IP_ADDR0,  NET_IP_ADDR1,  NET_IP_ADDR2,  NET_IP_ADDR3);
	IP4_ADDR(&netmask, NET_NETMASK0,  NET_NETMASK1,  NET_NETMASK2,  NET_NETMASK3);
	IP4_ADDR(&gateway, NET_GW_ADDR0,  NET_GW_ADDR1,  NET_GW_ADDR2,  NET_GW_ADDR3);

	/* Spawns tcpip_thread (queued until the scheduler runs). */
	tcpip_init(NULL, NULL);

	/* netif input is tcpip_input: RX frames are handed to the tcpip thread. */
	if (netif_add(&g_netif, &ipaddr, &netmask, &gateway, NULL,
	              ethernetif_init, tcpip_input) == NULL)
	{
		return OS_ERR_OP;
	}

	netif_set_default(&g_netif);
	netif_set_up(&g_netif);

	os_thread_create(net_rx_thread, "net_rx",
	                 NET_RX_THREAD_STACK, NET_RX_THREAD_PRIO, &g_netif);

	return OS_ERR_NONE;
}
