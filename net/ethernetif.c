/**
 * @file        ethernetif.c
 * @brief       lwIP netif <-> drv_eth bridge for the STM32H7 MAC
 * @ingroup     net
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Network
 * @info        Implements the lwIP link-layer netif on top of the frame-level
 *              drv_eth.h API. TX flattens the pbuf chain into the driver's D2
 *              bounce buffer; RX copies each received frame into a pool pbuf and
 *              feeds tcpip_input. RX is interrupt-driven: the net service thread
 *              wakes on the ETH RX semaphore and calls ethernetif_poll().
 * @dependency  lwIP (netif, etharp, pbuf), drivers/drv_eth.h
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

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/snmp.h"
#include "netif/ethernet.h"

#include <net/ethernetif.h>
#include <drivers/drv_eth.h>
#include <string.h>

/* Two-character interface name lwIP uses for diagnostics. */
#define IFNAME0 's'
#define IFNAME1 't'

/* Board MAC address — configured in lwipopts.h (locally administered). */
static const uint8_t s_mac[DRV_ETH_MAC_LEN] = {
	NET_MAC_ADDR0, NET_MAC_ADDR1, NET_MAC_ADDR2,
	NET_MAC_ADDR3, NET_MAC_ADDR4, NET_MAC_ADDR5
};

/* Scratch buffer used to linearise an outgoing pbuf chain before handing it to
 * the driver. lwIP core locking serialises low_level_output, so one shared
 * buffer is safe. */
static uint8_t tx_assembly[DRV_ETH_MAX_FRAME_LEN];

/* ── Link bring-up ──────────────────────────────────────────────────────── */

/**
 * @brief Program the netif hardware address and start the MAC.
 * @return ERR_OK on success, ERR_IF on a driver failure.
 */
static err_t low_level_init(struct netif *netif)
{
	netif->hwaddr_len = ETH_HWADDR_LEN;
	memcpy(netif->hwaddr, s_mac, DRV_ETH_MAC_LEN);

	netif->mtu = 1500;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	if (drv_eth_init(s_mac) != OS_ERR_NONE)
	{
		return ERR_IF;
	}

	if (drv_eth_start() != OS_ERR_NONE)
	{
		return ERR_IF;
	}

	return ERR_OK;
}

/* ── TX ─────────────────────────────────────────────────────────────────── */

/**
 * @brief Transmit a pbuf chain.
 * @note  Flattens the chain into tx_assembly, then issues a blocking driver
 *        transmit. Called by etharp_output / the stack via netif->linkoutput.
 * @return ERR_OK on success, ERR_IF on a driver error, ERR_MEM if oversized.
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	(void)netif;

	if (p->tot_len > sizeof(tx_assembly))
	{
		return ERR_MEM;
	}

	/* pbuf_copy_partial walks the chain and returns the byte count copied. */
	u16_t len = pbuf_copy_partial(p, tx_assembly, p->tot_len, 0);

	if (len != p->tot_len)
	{
		return ERR_BUF;
	}

	if (drv_eth_transmit(tx_assembly, len) != OS_ERR_NONE)
	{
		return ERR_IF;
	}

	MIB2_STATS_NETIF_ADD(netif, ifoutoctets, len);

	return ERR_OK;
}

/* ── RX ─────────────────────────────────────────────────────────────────── */

/**
 * @brief Pull one frame from the driver and wrap it in a pool pbuf.
 * @return The new pbuf, or NULL when no frame is ready / allocation failed.
 *         The driver buffer is always released before returning.
 */
static struct pbuf *low_level_input(struct netif *netif)
{
	uint8_t  *frame = NULL;
	uint16_t  len   = 0;

	if (drv_eth_receive(&frame, &len) != OS_ERR_NONE)
	{
		return NULL;
	}

	struct pbuf *p = NULL;

	if ((len > 0U) && (len <= DRV_ETH_MAX_FRAME_LEN))
	{
		p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

		if (p != NULL)
		{
			/* Single-buffer frames: a straight copy into the pbuf chain. */
			if (pbuf_take(p, frame, len) != ERR_OK)
			{
				pbuf_free(p);
				p = NULL;
			}
			else
			{
				MIB2_STATS_NETIF_ADD(netif, ifinoctets, len);
			}
		}
	}

	/* The driver buffer is fully consumed (copied) — hand it back at once. */
	drv_eth_rx_release(frame);

	return p;
}

uint32_t ethernetif_poll(struct netif *netif)
{
	struct pbuf *p;
	uint32_t     drained = 0;

	/* Drain every frame the DMA has queued since the last poll. */
	while ((p = low_level_input(netif)) != NULL)
	{
		drained++;
		if (netif->input(p, netif) != ERR_OK)
		{
			pbuf_free(p);
		}
	}

	return drained;
}

/* ── netif init callback ────────────────────────────────────────────────── */

err_t ethernetif_init(struct netif *netif)
{
	LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
	netif->hostname = "freertos-os";
#endif

	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;

	/* Outgoing path: ARP resolves then calls our linkoutput. */
	netif->output     = etharp_output;
	netif->linkoutput = low_level_output;

	return low_level_init(netif);
}
