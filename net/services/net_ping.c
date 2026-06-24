/**
 * @file        net_ping.c
 * @brief       Raw-ICMP ping client for the shell 'ping' command
 * @ingroup     net
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Network
 * @info        Implements net_ping.h on the lwIP raw API. An echo request is
 *              built by hand (ICMP header + payload + software checksum) and
 *              handed to raw_sendto under the TCP/IP core lock; the matching
 *              echo reply is caught by a raw recv callback running in the
 *              tcpip thread, which signals a semaphore the caller blocks on.
 *              Round-trip time is measured with sys_now().
 * @dependency  lwIP (raw, icmp, inet_chksum), os/kernel.h
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
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip_addr.h"
#include "lwip/ip.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"

#include <net/net_ping.h>
#include <os/kernel.h>
#include <perf/cycle_count.h>
#include <string.h>

/* Core clock (Hz) for cycle->microsecond conversion of the RTT. */
extern uint32_t SystemCoreClock;

/* Identifier stamped into every echo request so replies can be matched back. */
#define PING_ID         0xAFAFU

/* ── Session state (single concurrent session — the shell is one thread) ───── */

static struct raw_pcb   *ping_pcb;
static os_sem_t          ping_sem;
static ip_addr_t         ping_target;
static volatile uint16_t ping_expect_seq;

/* ── Echo-reply handler (runs in the tcpip thread, core lock held) ────────── */

/**
 * @brief raw recv callback: eat the echo reply that matches our id/seq.
 * @return 1 if the pbuf was consumed (ours), 0 to pass it on.
 */
static u8_t _ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p,
                       const ip_addr_t *addr)
{
    (void)arg;
    (void)pcb;
    (void)addr;

    /* The raw pbuf still starts at the IP header; step over it to the ICMP. */
    if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) &&
        (pbuf_remove_header(p, PBUF_IP_HLEN) == 0))
    {
        const struct icmp_echo_hdr *iecho =
            (const struct icmp_echo_hdr *)p->payload;

        if ((iecho->id == PING_ID) &&
            (iecho->seqno == lwip_htons(ping_expect_seq)) &&
            (ICMPH_TYPE(iecho) == ICMP_ER))
        {
            pbuf_free(p);
            (void)os_sem_give(ping_sem);
            return 1;
        }

        /* Not ours: restore the header and let the stack handle it. */
        pbuf_add_header(p, PBUF_IP_HLEN);
    }

    return 0;
}

/* ── Public API ───────────────────────────────────────────────────────────── */

int32_t net_ping_open(const char *ip_str)
{
    if (ip_str == NULL)
    {
        return OS_ERR_NULL_PTR;
    }

    if (ipaddr_aton(ip_str, &ping_target) == 0)
    {
        return OS_ERR_INVALID_ARG;
    }

    if (ping_pcb != NULL)
    {
        net_ping_close();
    }

    if (ping_sem == NULL)
    {
        ping_sem = os_sem_create(1, 0);
        if (ping_sem == NULL)
        {
            return OS_ERR_OP;
        }
    }

    /* Free-running DWT cycle counter drives the sub-ms RTT measurement. */
    cycle_count_enable();

    LOCK_TCPIP_CORE();
    ping_pcb = raw_new(IP_PROTO_ICMP);
    if (ping_pcb != NULL)
    {
        raw_recv(ping_pcb, _ping_recv, NULL);
        (void)raw_bind(ping_pcb, IP_ADDR_ANY);

        /* Pre-warm the ARP cache so the first echo request is not stalled
         * waiting on address resolution (which would time out as a loss). */
        if (netif_default != NULL)
        {
            (void)etharp_request(netif_default, ip_2_ip4(&ping_target));
        }
    }
    UNLOCK_TCPIP_CORE();

    return (ping_pcb != NULL) ? OS_ERR_NONE : OS_ERR_OP;
}

int32_t net_ping_once(uint16_t seq, uint32_t timeout_ms, uint32_t *rtt_us)
{
    if (ping_pcb == NULL)
    {
        return OS_ERR_OP;
    }

    const size_t icmp_len = sizeof(struct icmp_echo_hdr) + NET_PING_PAYLOAD_LEN;

    struct pbuf *p = pbuf_alloc(PBUF_IP, (u16_t)icmp_len, PBUF_RAM);
    if (p == NULL)
    {
        return OS_ERR_OP;
    }

    struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)p->payload;
    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = PING_ID;
    iecho->seqno  = lwip_htons(seq);

    /* Deterministic payload (0,1,2,...) like the classic ping pattern. */
    for (size_t i = 0; i < NET_PING_PAYLOAD_LEN; i++)
    {
        ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

    /* Software ICMP checksum: raw_sendto does not insert it for us. */
    iecho->chksum = inet_chksum(iecho, (u16_t)icmp_len);

    ping_expect_seq = seq;

    /* Drop any stale token from a late reply to a previous sequence. */
    (void)os_sem_take(ping_sem, 0);

    const uint32_t c0 = cycle_count_get();

    LOCK_TCPIP_CORE();
    err_t er = raw_sendto(ping_pcb, p, &ping_target);
    UNLOCK_TCPIP_CORE();

    pbuf_free(p);

    if (er != ERR_OK)
    {
        return OS_ERR_OP;
    }

    if (os_sem_take(ping_sem, timeout_ms) != OS_ERR_NONE)
    {
        return OS_ERR_TIMEOUT;
    }

    if (rtt_us != NULL)
    {
        /* Wrap-safe modulo-2^32 difference; idle never WFIs so CYCCNT is
         * a true free-running wall clock here. */
        uint32_t cycles = cycle_count_get() - c0;
        *rtt_us = cycle_count_to_us(cycles, SystemCoreClock);
    }

    return OS_ERR_NONE;
}

void net_ping_close(void)
{
    if (ping_pcb != NULL)
    {
        LOCK_TCPIP_CORE();
        raw_remove(ping_pcb);
        UNLOCK_TCPIP_CORE();
        ping_pcb = NULL;
    }
}
