/**
 * @file        lwipopts.h
 * @brief       lwIP configuration for the NUCLEO-H723ZG ping bring-up
 * @ingroup     board_config
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Board Config
 * @info        Threaded-mode (NO_SYS=0) lwIP tuned for an ICMP echo responder
 *              over the on-board LAN8742 RMII PHY. Sockets/netconn are off:
 *              the only traffic path needed is ARP + IPv4 + ICMP. The stack
 *              runs on the FreeRTOS-OS kernel via net/port/sys_arch.c.
 * @dependency  net/port (sys_arch), STM32H7 ETH driver
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

#ifndef BOARD_LWIPOPTS_H_
#define BOARD_LWIPOPTS_H_

/* ── OS mode ────────────────────────────────────────────────────────────── */
/* Threaded mode: tcpip_thread plus the port in net/port/sys_arch.c. */
#define NO_SYS                          0
#define SYS_LIGHTWEIGHT_PROT            1
#define LWIP_TCPIP_CORE_LOCKING         1

/* Real priority-inheriting mutex from the kernel — not the sem fallback. */
#define LWIP_COMPAT_MUTEX               0

/* Stack size passed to sys_thread_new() is a FreeRTOS-style word count. */
#define LWIP_FREERTOS_THREAD_STACKSIZE_IS_STACKWORDS 1

/* ── Memory ─────────────────────────────────────────────────────────────── */
#define MEM_LIBC_MALLOC                 0
#define MEMP_MEM_MALLOC                 0
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)

#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                4
#define MEMP_NUM_TCP_PCB_LISTEN         2
#define MEMP_NUM_TCP_SEG                16
#define MEMP_NUM_REASSDATA              4
#define MEMP_NUM_ARP_QUEUE              4
#define MEMP_NUM_SYS_TIMEOUT            8

/* RX uses zero-copy custom pbufs (see net/ethernetif.c); the pool here is a
 * small safety reserve for the rare pbuf the core layers allocate. */
#define PBUF_POOL_SIZE                  8
#define PBUF_POOL_BUFSIZE               1536

/* ── Protocol layers ────────────────────────────────────────────────────── */
#define LWIP_IPV4                       1
#define LWIP_IPV6                       0
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_ICMP                       1            /* the ping responder */
#define LWIP_RAW                        1            /* raw ICMP for 'ping' cmd */
#define MEMP_NUM_RAW_PCB                4
#define LWIP_DHCP                       0            /* static IP */
#define LWIP_AUTOIP                     0
#define LWIP_IGMP                       0
#define LWIP_UDP                        1
#define LWIP_TCP                        1

/* No application API surface needed for ping. */
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0

/* ── Checksums (software; MAC hardware offload left disabled) ────────────── */
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1
#define LWIP_CHECKSUM_ON_COPY          0

/* ── netif options ──────────────────────────────────────────────────────── */
#define LWIP_NETIF_HOSTNAME             0
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_SINGLE_NETIF               1
#define LWIP_SUPPORT_CUSTOM_PBUF        1            /* zero-copy RX path */

/* ── tcpip thread / mailbox sizing ──────────────────────────────────────── */
#define TCPIP_THREAD_NAME               "tcpip"
#define TCPIP_THREAD_STACKSIZE          1024         /* words */
#define TCPIP_THREAD_PRIO               10
#define TCPIP_MBOX_SIZE                 16
#define DEFAULT_RAW_RECVMBOX_SIZE       8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_TCP_RECVMBOX_SIZE       8
#define DEFAULT_ACCEPTMBOX_SIZE         8

/* ── Diagnostics (off for size; flip LWIP_DEBUG to chase bring-up issues) ── */
#define LWIP_STATS                      0
#define LWIP_STATS_DISPLAY              0
#define LWIP_DEBUG                      0
#define CHECKSUM_GEN_IP6                0

/* ── Static network identity ────────────────────────────────────────────────
 * Not lwIP core options: these are the board's fixed addresses, consumed by
 * net/services/net_service.c (netif address) and net/ethernetif.c (hwaddr).
 * Kept here so the whole network configuration lives in one file.
 */

/* Board IPv4 address — 192.168.1.50 */
#define NET_IP_ADDR0                    192
#define NET_IP_ADDR1                    168
#define NET_IP_ADDR2                    1
#define NET_IP_ADDR3                    50

/* Subnet mask — 255.255.255.0 (/24) */
#define NET_NETMASK0                    255
#define NET_NETMASK1                    255
#define NET_NETMASK2                    255
#define NET_NETMASK3                    0

/* Default gateway — 192.168.1.1 */
#define NET_GW_ADDR0                    192
#define NET_GW_ADDR1                    168
#define NET_GW_ADDR2                    1
#define NET_GW_ADDR3                    1

/* Locally-administered MAC (first octet bit1 set, bit0 clear) — 02:00:00:00:00:01 */
#define NET_MAC_ADDR0                   0x02
#define NET_MAC_ADDR1                   0x00
#define NET_MAC_ADDR2                   0x00
#define NET_MAC_ADDR3                   0x00
#define NET_MAC_ADDR4                   0x00
#define NET_MAC_ADDR5                   0x01

#endif /* BOARD_LWIPOPTS_H_ */
