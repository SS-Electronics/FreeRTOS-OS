/**
 * @file        net_ping.h
 * @brief       Raw-ICMP ping client (board acts as the ICMP echo requester)
 * @ingroup     net
 *
 * @author      Subhajit Roy <subhajitroy005@gmail.com>
 * @module      Network
 * @info        A small blocking ping client built on the lwIP raw API. The
 *              shell 'ping' command drives it one echo at a time: open a
 *              session to a target, fire N requests, then close. The API takes
 *              no lwIP types so callers (the shell) stay decoupled from lwIP.
 * @dependency  lwIP (raw, icmp), os/kernel.h
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

#ifndef NET_NET_PING_H_
#define NET_NET_PING_H_

#include <def_std.h>
#include <def_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ICMP echo payload length, in bytes (excludes the 8-byte ICMP header). */
#define NET_PING_PAYLOAD_LEN    32U

/**
 * @brief Open a ping session to a dotted-quad IPv4 target.
 * @note  Allocates a raw ICMP PCB and registers the echo-reply handler. Call
 *        net_ping_close() when done. Only one session at a time (the shell is
 *        single-threaded).
 * @param ip_str  Target address as "a.b.c.d".
 * @return OS_ERR_NONE on success, OS_ERR_INVALID_ARG on a malformed address,
 *         OS_ERR_OP if the raw PCB could not be allocated.
 */
int32_t net_ping_open(const char *ip_str);

/**
 * @brief Send one ICMP echo request and block for the reply.
 * @param seq         Sequence number to stamp into the request.
 * @param timeout_ms  Max time to wait for the matching echo reply.
 * @param rtt_us      Out (nullable): round-trip time in microseconds (DWT
 *                    cycle-counter resolution) when a reply lands.
 * @return OS_ERR_NONE on a matching reply, OS_ERR_TIMEOUT if none arrived,
 *         OS_ERR_OP on a send error / no open session.
 */
int32_t net_ping_once(uint16_t seq, uint32_t timeout_ms, uint32_t *rtt_us);

/**
 * @brief Close the ping session and free the raw PCB.
 */
void net_ping_close(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_NET_PING_H_ */
