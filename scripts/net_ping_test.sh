#!/usr/bin/env bash
#
# net_ping_test.sh — host-side ping test for the STM32H723 lwIP bring-up.
#
# The board answers ICMP echo at a fixed address (default 192.168.1.50). This
# script puts a host interface on that subnet, pings the board, and reports a
# structured pass/fail. Run it after flashing build/stm32h723.elf and cabling
# the board's RJ45 to this host (direct cable or shared switch).
#
# Usage:
#   sudo ./scripts/net_ping_test.sh [BOARD_IP] [HOST_IFACE] [HOST_IP]
#
# Defaults: BOARD_IP=192.168.1.50  HOST_IFACE=auto  HOST_IP=192.168.1.10/24
#
# This file is part of FreeRTOS-OS Project (GPLv3).

set -u

BOARD_IP="${1:-192.168.1.50}"
HOST_IFACE="${2:-auto}"
HOST_IP="${3:-192.168.1.10/24}"

PING_COUNT=10
PING_DEADLINE=15      # seconds
PASS_THRESHOLD=8      # replies required to pass (allows a couple of dropped ARPs)

say()  { printf '[net-test] %s\n' "$*"; }
fail() { printf '[net-test] FAIL: %s\n' "$*" >&2; exit 1; }

# ── Pick a wired interface if not told ──────────────────────────────────────
if [ "$HOST_IFACE" = "auto" ]; then
    HOST_IFACE=$(ip -o link show up 2>/dev/null \
                 | awk -F': ' '$2 ~ /^(en|eth)/ {print $2; exit}')
    [ -n "$HOST_IFACE" ] || fail "no wired interface found; pass one explicitly"
fi
say "board=$BOARD_IP  host-iface=$HOST_IFACE  host-ip=$HOST_IP"

# ── Configure the host address (added, not replacing existing config) ───────
if ! ip addr show dev "$HOST_IFACE" | grep -qw "${HOST_IP%%/*}"; then
    say "adding $HOST_IP to $HOST_IFACE (needs root)"
    ip addr add "$HOST_IP" dev "$HOST_IFACE" 2>/dev/null \
        || fail "could not add address — run with sudo"
    ADDED_IP=1
else
    ADDED_IP=0
fi
ip link set "$HOST_IFACE" up 2>/dev/null

cleanup() {
    if [ "${ADDED_IP:-0}" = "1" ]; then
        ip addr del "$HOST_IP" dev "$HOST_IFACE" 2>/dev/null
    fi
}
trap cleanup EXIT

# ── Give the link a moment, then ping ───────────────────────────────────────
sleep 2
say "pinging $BOARD_IP ($PING_COUNT echo requests)..."
OUT=$(ping -c "$PING_COUNT" -w "$PING_DEADLINE" "$BOARD_IP" 2>&1)
printf '%s\n' "$OUT"

RECV=$(printf '%s\n' "$OUT" | sed -n 's/.* \([0-9]\+\) received.*/\1/p')
RECV="${RECV:-0}"

# ── Verdict ─────────────────────────────────────────────────────────────────
say "replies received: $RECV / $PING_COUNT (need >= $PASS_THRESHOLD)"
if [ "$RECV" -ge "$PASS_THRESHOLD" ]; then
    say "PASS — board is answering ICMP echo."
    exit 0
fi

cat >&2 <<EOF
[net-test] FAIL: too few replies. Check, in order:
  1. Cable / link LED on the RJ45.
  2. UART console (UART_APP 115200 8N1): heartbeat ticking? any
     "[net] stack init failed" line means PHY auto-neg did not complete.
  3. Host and board on the same subnet (host $HOST_IP, board $BOARD_IP).
  4. arp -n | grep $BOARD_IP — is the MAC 02:00:00:00:00:01 resolving?
EOF
exit 1
