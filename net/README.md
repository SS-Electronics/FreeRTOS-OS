# net/ — lwIP TCP/IP stack + FreeRTOS-OS port

ICMP echo (ping) bring-up for the **NUCLEO-H723ZG** over the on-board
**LAN8742** PHY (RMII). The stack runs in lwIP threaded mode (`NO_SYS=0`)
on top of the FreeRTOS-OS kernel, with no dependency on FreeRTOS headers
above the port layer. RX is **interrupt-driven** (ETH_IRQn).

## Layout

| Path | Role |
|------|------|
| `lwip/` | Upstream lwIP source (submodule) |
| `port/include/arch/cc.h` | Compiler abstraction: packing, diagnostics, format specifiers, lwIP-memory placement into AXI SRAM |
| `port/include/arch/sys_arch.h` | `sys_sem_t` / `sys_mutex_t` / `sys_mbox_t` / `sys_thread_t` mapped onto the `os_*` kernel handles |
| `port/sys_arch.c` | lwIP OS abstraction implemented on `kernel.h` IPC + `os_uptime_ms()` |
| `ethernetif.c` | netif glue: pbuf TX flatten, polled RX copy, `tcpip_input` feed |
| `services/net_service.c` | `net_service_start()` — lwIP init, static-IP netif, RX poll thread |

Public headers: `include/net/ethernetif.h`, `include/net/net_service.h`.
Board config: `app/board/lwipopts.h`. Driver: `drivers/hal/stm32/hal_eth_stm32.c`
(API in `include/drivers/drv_eth.h`).

## Architecture

```
app_main()  ──► net_service_start()
                  ├─ tcpip_init()           spawns tcpip_thread (prio 10)
                  ├─ netif_add(...)          ethernetif_init ─► drv_eth_init/start_IT
                  │                            ├─ MPU: D2 SRAM non-cacheable
                  │                            ├─ RMII GPIO + ETH clocks
                  │                            └─ LAN8742 auto-negotiation
                  ├─ netif_set_up()
                  └─ os_thread_create(net_rx, prio 9)
                        loop: drv_eth_rx_wait()  ◄─ rx_sem ◄─ ETH_IRQ
                              ethernetif_poll() ─► drv_eth_receive()
                                                   └─ tcpip_input() ─► ICMP echo

ETH_IRQn (61) ─► ETH_IRQHandler ─► hal_eth_stm32_irq_handler
              ─► HAL_ETH_IRQHandler ─► HAL_ETH_RxCpltCallback ─► os_sem_give_from_isr(rx_sem)
```

- **Interrupt-driven RX.** `HAL_ETH_Start_IT` arms the RX DMA interrupt; the
  ETH ISR (NVIC priority 6, ISR-safe) gives `rx_sem`, waking the `net_rx`
  thread which drains every queued frame. ETH_IRQn is routed through the board
  IRQ-table generator via a guarded `<sys>` entry in `irq_table.xml` (empty
  stub when `CONFIG_NET` is off, so non-net builds still link).
- **Cache coherency.** ETH DMA descriptors, the RX buffer pool and the TX
  bounce buffer live in D2 SRAM (`0x30000000`, `.eth_d2` section). The driver
  marks that 32 KB window MPU non-cacheable before `HAL_ETH_Init` touches it.
- **Memory placement.** lwIP's heap (`MEM_SIZE`) and memp pools are redirected
  into AXI SRAM (`.axi_data`, 320 KB) via `LWIP_DECLARE_MEMORY_ALIGNED`, so
  they do not crowd the 80 KB FreeRTOS heap in DTCM.
- **Checksums** are computed in software (`CHECKSUM_*=1`); MAC hardware offload
  stays disabled.
- **Init race:** all of `net_service_start()` runs in `app_main()` *before*
  `vTaskStartScheduler()`, so the tcpip and RX threads are only queued — no
  concurrency during bring-up. The PHY auto-negotiation timeout relies on the
  TIM6-based HAL tick, which runs before the scheduler.

## Network identity (edit in `services/net_service.c`)

| Item | Value |
|------|-------|
| IP | `192.168.1.50` |
| Netmask | `255.255.255.0` |
| Gateway | `192.168.1.1` |
| MAC | `02:00:00:00:00:01` (in `ethernetif.c`) |

## RMII pin map (NUCLEO-H723ZG, all AF11_ETH)

| Signal | Pin | Signal | Pin |
|--------|-----|--------|-----|
| REF_CLK | PA1 | RXD0 | PC4 |
| MDIO | PA2 | RXD1 | PC5 |
| CRS_DV | PA7 | TXD1 | PB13 |
| MDC | PC1 | TX_EN | PG11 |
|  |  | TXD0 | PG13 |

> **PA7 conflict:** the board XML assigns PA7 to SPI1_MOSI. Ethernet claims
> PA7 for `RMII_CRS_DV`; the SPI1 MOSI demo on that pin is superseded while
> `CONFIG_NET=y`.

## Build

```
cp ../app/kconfig.conf .config
make config-outputs
make all APP_DIR=../app TARGET_NAME=stm32h723 CONFIG_BOARD=stm32h723_devboard
```

`CONFIG_NET=y` and `CONFIG_HAL_ETH_MODULE_ENABLED=y` in `kconfig.conf` gate the
whole net stack, the ETH HAL driver and the generated `HAL_ETH_MODULE_ENABLED`.

## Ping test (on hardware)

1. Flash `build/stm32h723.elf` (`make dev-stm32h723-flash`).
2. Connect the board's RJ45 to a host (direct cable or switch).
3. Put the host on the same subnet, e.g. `192.168.1.10/24`.
4. From the host:

   ```
   ping 192.168.1.50
   ```

   Expect replies. The UART console (`UART_APP`, 115200 8N1) shows the
   heartbeat ticking; `[net] stack init failed` there means the PHY link did
   not come up (check the cable and that auto-negotiation completed).

> Status: firmware **builds green** and the stack is wired end to end. The
> on-hardware `ping` round-trip has **not** been executed in this environment
> (no board attached); run the steps above to confirm on real silicon.
