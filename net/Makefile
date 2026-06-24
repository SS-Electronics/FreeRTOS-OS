# File:        net/Makefile
# Author:      Subhajit Roy
#              subhajitroy005@gmail.com
#
# Module:      net
# Info:        lwIP TCP/IP stack + FreeRTOS-OS port + Ethernet glue.
#              Built only when CONFIG_NET=y (the STM32H7 ping bring-up).
#
# This file is part of FreeRTOS-OS Project.
#
# FreeRTOS-OS is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, either version
# 3 of the License, or (at your option) any later version.

include autoconf.mk

ifeq ($(CONFIG_NET),1)

LWIPDIR := net/lwip/src

# ── Include paths ──────────────────────────────────────────────────────────
# lwipopts.h is supplied by the application board dir (already on INCLUDES via
# -I$(APP_DIR)/board). cc.h / sys_arch.h come from the port; netif glue headers
# from include/net (already on INCLUDES via include/Makefile -Iinclude).
INCLUDES += -I$(CURDIR)/$(LWIPDIR)/include
INCLUDES += -I$(CURDIR)/net/port/include

# ── lwIP core ──────────────────────────────────────────────────────────────
obj-y += $(LWIPDIR)/core/init.o
obj-y += $(LWIPDIR)/core/def.o
obj-y += $(LWIPDIR)/core/dns.o
obj-y += $(LWIPDIR)/core/inet_chksum.o
obj-y += $(LWIPDIR)/core/ip.o
obj-y += $(LWIPDIR)/core/mem.o
obj-y += $(LWIPDIR)/core/memp.o
obj-y += $(LWIPDIR)/core/netif.o
obj-y += $(LWIPDIR)/core/pbuf.o
obj-y += $(LWIPDIR)/core/raw.o
obj-y += $(LWIPDIR)/core/stats.o
obj-y += $(LWIPDIR)/core/sys.o
obj-y += $(LWIPDIR)/core/altcp.o
obj-y += $(LWIPDIR)/core/altcp_alloc.o
obj-y += $(LWIPDIR)/core/altcp_tcp.o
obj-y += $(LWIPDIR)/core/tcp.o
obj-y += $(LWIPDIR)/core/tcp_in.o
obj-y += $(LWIPDIR)/core/tcp_out.o
obj-y += $(LWIPDIR)/core/timeouts.o
obj-y += $(LWIPDIR)/core/udp.o

# ── lwIP IPv4 ──────────────────────────────────────────────────────────────
obj-y += $(LWIPDIR)/core/ipv4/acd.o
obj-y += $(LWIPDIR)/core/ipv4/autoip.o
obj-y += $(LWIPDIR)/core/ipv4/dhcp.o
obj-y += $(LWIPDIR)/core/ipv4/etharp.o
obj-y += $(LWIPDIR)/core/ipv4/icmp.o
obj-y += $(LWIPDIR)/core/ipv4/igmp.o
obj-y += $(LWIPDIR)/core/ipv4/ip4_frag.o
obj-y += $(LWIPDIR)/core/ipv4/ip4.o
obj-y += $(LWIPDIR)/core/ipv4/ip4_addr.o

# ── lwIP netif + tcpip thread ──────────────────────────────────────────────
obj-y += $(LWIPDIR)/netif/ethernet.o
obj-y += $(LWIPDIR)/api/tcpip.o
obj-y += $(LWIPDIR)/api/err.o

# ── FreeRTOS-OS port + Ethernet glue + service thread ──────────────────────
obj-y += net/port/sys_arch.o
obj-y += net/ethernetif.o
obj-y += net/services/net_service.o
obj-y += net/services/net_ping.o

endif
