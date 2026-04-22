#!/usr/bin/env python3
"""
gen_irq_table.py — IRQ table and NVIC init code generator

Reads app/board/irq_table.xml and generates:

  app/board/irq_hw_conf.h
      Capacity constants, base offsets, irq_id_t typedef, and ID macros.
      Included by irq_notify.h (and everything that uses IRQ IDs).
      Replaces the hand-written section that lived in irq_notify.h.

  FreeRTOS-OS/irq/irq_table.c
      Static IRQ name table indexed by software irq_id_t.

  app/board/irq_hw_init_generated.c  +  irq_hw_init_generated.h
      irq_hw_init_all() — binds irq_chip_nvic to each software IRQ ID,
      sets NVIC priorities, and enables hardware interrupt lines.
      Call once at boot before any request_irq().

  app/board/irq_periph_handlers_generated.h
      Weak forward declarations for every peripheral IRQ handler.
      Include ONLY from the startup compilation unit (Default_Handler must
      be visible at the include site).

  app/board/irq_periph_vectors_generated.inc
      Raw C comma-separated entries for the peripheral section of
      g_pfnVectors[].  #include'd inside the array in startup_*.c.

Usage (from project root):
    python3 FreeRTOS-OS/scripts/gen_irq_table.py app/board/irq_table.xml
    — or —
    make irq_gen

Usage (from FreeRTOS-OS/):
    python3 scripts/gen_irq_table.py ../app/board/irq_table.xml
    — or —
    make irq_gen APP_DIR=../app
"""

import sys
import os
import re
import xml.etree.ElementTree as ET
from datetime import datetime, timezone

# ── Default capacity constants (overridden by <config> in XML) ───────────────

IRQ_MAX_UART       = 4
IRQ_MAX_I2C        = 4
IRQ_MAX_SPI        = 4
IRQ_MAX_EXTI_LINES = 16
IRQ_NOTIFY_MAX_SUBS = 4

IRQ_UART_EVENTS = 3   # RX, TX_DONE, ERROR
IRQ_I2C_EVENTS  = 3   # TX_DONE, RX_DONE, ERROR
IRQ_SPI_EVENTS  = 4   # TX_DONE, RX_DONE, TXRX_DONE, ERROR

IRQ_ID_UART_BASE = 0
IRQ_ID_I2C_BASE  = IRQ_ID_UART_BASE + IRQ_MAX_UART * IRQ_UART_EVENTS
IRQ_ID_SPI_BASE  = IRQ_ID_I2C_BASE  + IRQ_MAX_I2C  * IRQ_I2C_EVENTS
IRQ_ID_EXTI_BASE = IRQ_ID_SPI_BASE  + IRQ_MAX_SPI  * IRQ_SPI_EVENTS
IRQ_ID_TOTAL     = IRQ_ID_EXTI_BASE + IRQ_MAX_EXTI_LINES

# ── Derived ID helpers (use module globals so _update_globals() takes effect) ─

def uart_rx(n):        return IRQ_ID_UART_BASE + n * IRQ_UART_EVENTS + 0
def uart_tx_done(n):   return IRQ_ID_UART_BASE + n * IRQ_UART_EVENTS + 1
def uart_error(n):     return IRQ_ID_UART_BASE + n * IRQ_UART_EVENTS + 2

def i2c_tx_done(n):    return IRQ_ID_I2C_BASE  + n * IRQ_I2C_EVENTS  + 0
def i2c_rx_done(n):    return IRQ_ID_I2C_BASE  + n * IRQ_I2C_EVENTS  + 1
def i2c_error(n):      return IRQ_ID_I2C_BASE  + n * IRQ_I2C_EVENTS  + 2

def spi_tx_done(n):    return IRQ_ID_SPI_BASE  + n * IRQ_SPI_EVENTS  + 0
def spi_rx_done(n):    return IRQ_ID_SPI_BASE  + n * IRQ_SPI_EVENTS  + 1
def spi_txrx_done(n):  return IRQ_ID_SPI_BASE  + n * IRQ_SPI_EVENTS  + 2
def spi_error(n):      return IRQ_ID_SPI_BASE  + n * IRQ_SPI_EVENTS  + 3

# Macro-string forms used in generated C source
def uart_rx_macro(n):       return f"IRQ_ID_UART_RX({n})"
def uart_tx_done_macro(n):  return f"IRQ_ID_UART_TX_DONE({n})"
def uart_error_macro(n):    return f"IRQ_ID_UART_ERROR({n})"

def i2c_tx_done_macro(n):   return f"IRQ_ID_I2C_TX_DONE({n})"
def i2c_rx_done_macro(n):   return f"IRQ_ID_I2C_RX_DONE({n})"
def i2c_error_macro(n):     return f"IRQ_ID_I2C_ERROR({n})"

def spi_tx_done_macro(n):   return f"IRQ_ID_SPI_TX_DONE({n})"
def spi_rx_done_macro(n):   return f"IRQ_ID_SPI_RX_DONE({n})"
def spi_txrx_done_macro(n): return f"IRQ_ID_SPI_TXRX_DONE({n})"
def spi_error_macro(n):     return f"IRQ_ID_SPI_ERROR({n})"

def exti_macro(n):          return f"IRQ_ID_EXTI({n})"

# ── Config / globals update ───────────────────────────────────────────────────

def parse_config_elem(root):
    """Read <config> element and return a capacity dict (empty if absent)."""
    cfg = root.find('config')
    if cfg is None:
        return {}
    return {k: int(v) for k, v in cfg.attrib.items()}

def _update_globals(cfg):
    """Overwrite module-level capacity constants from the parsed config dict."""
    global IRQ_MAX_UART, IRQ_MAX_I2C, IRQ_MAX_SPI, IRQ_MAX_EXTI_LINES
    global IRQ_UART_EVENTS, IRQ_I2C_EVENTS, IRQ_SPI_EVENTS, IRQ_NOTIFY_MAX_SUBS
    global IRQ_ID_UART_BASE, IRQ_ID_I2C_BASE, IRQ_ID_SPI_BASE
    global IRQ_ID_EXTI_BASE, IRQ_ID_TOTAL

    if 'max_uart'        in cfg: IRQ_MAX_UART        = cfg['max_uart']
    if 'uart_events'     in cfg: IRQ_UART_EVENTS     = cfg['uart_events']
    if 'max_i2c'         in cfg: IRQ_MAX_I2C         = cfg['max_i2c']
    if 'i2c_events'      in cfg: IRQ_I2C_EVENTS      = cfg['i2c_events']
    if 'max_spi'         in cfg: IRQ_MAX_SPI         = cfg['max_spi']
    if 'spi_events'      in cfg: IRQ_SPI_EVENTS      = cfg['spi_events']
    if 'max_exti'        in cfg: IRQ_MAX_EXTI_LINES  = cfg['max_exti']
    if 'notify_max_subs' in cfg: IRQ_NOTIFY_MAX_SUBS = cfg['notify_max_subs']

    IRQ_ID_UART_BASE = 0
    IRQ_ID_I2C_BASE  = IRQ_ID_UART_BASE + IRQ_MAX_UART * IRQ_UART_EVENTS
    IRQ_ID_SPI_BASE  = IRQ_ID_I2C_BASE  + IRQ_MAX_I2C  * IRQ_I2C_EVENTS
    IRQ_ID_EXTI_BASE = IRQ_ID_SPI_BASE  + IRQ_MAX_SPI  * IRQ_SPI_EVENTS
    IRQ_ID_TOTAL     = IRQ_ID_EXTI_BASE + IRQ_MAX_EXTI_LINES

# ── Default name table ────────────────────────────────────────────────────────

def build_default_names():
    """Pre-fill with generic names for every slot; overridden by XML labels."""
    names = {}
    for n in range(IRQ_MAX_UART):
        names[uart_rx(n)]      = f"UART{n}_RX"
        names[uart_tx_done(n)] = f"UART{n}_TX_DONE"
        names[uart_error(n)]   = f"UART{n}_ERROR"
    for n in range(IRQ_MAX_I2C):
        names[i2c_tx_done(n)]  = f"I2C{n}_TX_DONE"
        names[i2c_rx_done(n)]  = f"I2C{n}_RX_DONE"
        names[i2c_error(n)]    = f"I2C{n}_ERROR"
    for n in range(IRQ_MAX_SPI):
        names[spi_tx_done(n)]  = f"SPI{n}_TX_DONE"
        names[spi_rx_done(n)]  = f"SPI{n}_RX_DONE"
        names[spi_txrx_done(n)]= f"SPI{n}_TXRX_DONE"
        names[spi_error(n)]    = f"SPI{n}_ERROR"
    for n in range(IRQ_MAX_EXTI_LINES):
        names[IRQ_ID_EXTI_BASE + n] = f"EXTI{n}"
    return names

# ── XML parse (peripheral bindings + sys IRQs) ────────────────────────────────

def parse_xml_elements(root):
    """Parse uart/i2c/spi/exti/sys/sys_clk/gpio_clk elements.
    Returns (names, hw_irqs, sys_irqs, clk_info) where clk_info is a dict:
      clk_info['sys']   = list of inst strings (SYSCFG, PWR, …)
      clk_info['uart']  = list of hal_inst strings (USART1, USART2, …)
      clk_info['i2c']   = list of hal_inst strings (I2C1, …)
      clk_info['spi']   = list of hal_inst strings (SPI1, …)
      clk_info['gpio']  = list of port letters (A, B, …) — unique, sorted
    """
    names    = build_default_names()
    hw_irqs  = []
    sys_irqs = []
    clk_info = {'sys': [], 'uart': [], 'i2c': [], 'spi': [], 'gpio': set()}

    for elem in root:
        tag = elem.tag

        if tag == "uart":
            n    = int(elem.get("dev_id"))
            lbl  = elem.get("label", f"UART{n}")
            irqn = elem.get("irqn")
            prio = int(elem.get("priority", "5"))
            hal_inst = elem.get("hal_inst", "")

            if n >= IRQ_MAX_UART:
                _die(f"uart dev_id={n} exceeds IRQ_MAX_UART={IRQ_MAX_UART}")

            names[uart_rx(n)]      = f"{lbl}_RX"
            names[uart_tx_done(n)] = f"{lbl}_TX_DONE"
            names[uart_error(n)]   = f"{lbl}_ERROR"

            if irqn:
                hw_irqs.append((uart_rx_macro(n),      irqn, prio, lbl))
                hw_irqs.append((uart_tx_done_macro(n), irqn, prio, None))
                hw_irqs.append((uart_error_macro(n),   irqn, prio, None))

            if hal_inst and hal_inst not in clk_info['uart']:
                clk_info['uart'].append(hal_inst)

        elif tag == "i2c":
            n       = int(elem.get("dev_id"))
            lbl     = elem.get("label", f"I2C{n}")
            irqn_ev = elem.get("irqn_ev")
            prio_ev = int(elem.get("priority_ev", "5"))
            irqn_er = elem.get("irqn_er")
            prio_er = int(elem.get("priority_er", "5"))
            hal_inst = elem.get("hal_inst", "")

            if n >= IRQ_MAX_I2C:
                _die(f"i2c dev_id={n} exceeds IRQ_MAX_I2C={IRQ_MAX_I2C}")

            names[i2c_tx_done(n)] = f"{lbl}_TX_DONE"
            names[i2c_rx_done(n)] = f"{lbl}_RX_DONE"
            names[i2c_error(n)]   = f"{lbl}_ERROR"

            if irqn_ev:
                hw_irqs.append((i2c_tx_done_macro(n), irqn_ev, prio_ev, lbl))
                hw_irqs.append((i2c_rx_done_macro(n), irqn_ev, prio_ev, None))
            if irqn_er:
                hw_irqs.append((i2c_error_macro(n),   irqn_er, prio_er, None))

            if hal_inst and hal_inst not in clk_info['i2c']:
                clk_info['i2c'].append(hal_inst)

        elif tag == "spi":
            n    = int(elem.get("dev_id"))
            lbl  = elem.get("label", f"SPI{n}")
            irqn = elem.get("irqn")
            prio = int(elem.get("priority", "5"))
            hal_inst = elem.get("hal_inst", "")

            if n >= IRQ_MAX_SPI:
                _die(f"spi dev_id={n} exceeds IRQ_MAX_SPI={IRQ_MAX_SPI}")

            names[spi_tx_done(n)]   = f"{lbl}_TX_DONE"
            names[spi_rx_done(n)]   = f"{lbl}_RX_DONE"
            names[spi_txrx_done(n)] = f"{lbl}_TXRX_DONE"
            names[spi_error(n)]     = f"{lbl}_ERROR"

            if irqn:
                hw_irqs.append((spi_tx_done_macro(n),   irqn, prio, lbl))
                hw_irqs.append((spi_rx_done_macro(n),   irqn, prio, None))
                hw_irqs.append((spi_txrx_done_macro(n), irqn, prio, None))
                hw_irqs.append((spi_error_macro(n),     irqn, prio, None))

            if hal_inst and hal_inst not in clk_info['spi']:
                clk_info['spi'].append(hal_inst)

        elif tag == "exti":
            n    = int(elem.get("line"))
            lbl  = elem.get("label", f"EXTI{n}")
            irqn = elem.get("irqn")
            prio = int(elem.get("priority", "6"))

            if n >= IRQ_MAX_EXTI_LINES:
                _die(f"exti line={n} exceeds IRQ_MAX_EXTI_LINES={IRQ_MAX_EXTI_LINES}")

            names[IRQ_ID_EXTI_BASE + n] = lbl

            if irqn:
                hw_irqs.append((exti_macro(n), irqn, prio, lbl))

        elif tag == "sys":
            sys_irqs.append({
                "name":     elem.get("name"),
                "irqn":     elem.get("irqn"),
                "priority": int(elem.get("priority", "15")),
                "action":   elem.get("action", "set_priority"),
                "irq_id":   elem.get("irq_id"),          # optional software IRQ ID name
                "flow":     elem.get("flow", "simple"),  # "simple" | "edge"
            })

        elif tag == "sys_clk":
            inst = elem.get("inst", "").strip()
            if inst and inst not in clk_info['sys']:
                clk_info['sys'].append(inst)

        elif tag == "gpio_clk":
            port = elem.get("port", "").strip().upper()
            if port:
                clk_info['gpio'].add(port)

    clk_info['gpio'] = sorted(clk_info['gpio'])
    return names, hw_irqs, sys_irqs, clk_info

def _irqn_to_handler(irqn):
    """Convert an IRQn symbol to its C handler name: USART1_IRQn → USART1_IRQHandler."""
    return irqn.replace('_IRQn', '_IRQHandler')

def parse_vector_table_elem(root):
    """Parse <vector_table> entries; return {irqn: (handler, comment)}."""
    vt = root.find('vector_table')
    if vt is None:
        return {}
    result = {}
    for e in vt.findall('entry'):
        irqn    = int(e.get('irqn'))
        handler = e.get('handler')
        comment = e.get('comment', '')
        result[irqn] = (handler, comment)
    return result

# ── Dispatch data parser ──────────────────────────────────────────────────────

def parse_dispatch_data(root):
    """
    Build a handler_map {handler_name → info_dict} and extra_includes set
    from all XML elements that describe ISR dispatch bodies.

    info_dict keys:
      kind         : 'uart' | 'i2c_ev' | 'i2c_er' | 'spi' | 'sys_dispatch'
      hal_inst     : HAL peripheral pointer (uart/i2c/spi)
      guard        : optional preprocessor guard string
      call         : dispatch expression (sys_dispatch only)
      label        : human-readable comment label
    """
    handler_map     = {}
    extra_includes  = set()

    for elem in root:
        tag = elem.tag

        if tag == "uart":
            irqn     = elem.get("irqn")
            hal_inst = elem.get("hal_inst")
            label    = elem.get("label", "")
            if irqn and hal_inst:
                handler_map[_irqn_to_handler(irqn)] = {
                    'kind': 'uart', 'hal_inst': hal_inst,
                    'guard': '', 'label': label,
                }
                extra_includes.add('<drivers/com/hal/stm32/hal_uart_stm32.h>')

        elif tag == "i2c":
            irqn_ev  = elem.get("irqn_ev")
            irqn_er  = elem.get("irqn_er")
            hal_inst = elem.get("hal_inst")
            label    = elem.get("label", "")
            if hal_inst:
                if irqn_ev:
                    handler_map[_irqn_to_handler(irqn_ev)] = {
                        'kind': 'i2c_ev', 'hal_inst': hal_inst,
                        'guard': 'HAL_I2C_MODULE_ENABLED', 'label': label,
                    }
                if irqn_er:
                    handler_map[_irqn_to_handler(irqn_er)] = {
                        'kind': 'i2c_er', 'hal_inst': hal_inst,
                        'guard': 'HAL_I2C_MODULE_ENABLED', 'label': None,
                    }
                if irqn_ev or irqn_er:
                    extra_includes.add('<drivers/com/hal/stm32/hal_iic_stm32.h>')

        elif tag == "spi":
            irqn     = elem.get("irqn")
            hal_inst = elem.get("hal_inst")
            label    = elem.get("label", "")
            if irqn and hal_inst:
                handler_map[_irqn_to_handler(irqn)] = {
                    'kind': 'spi', 'hal_inst': hal_inst,
                    'guard': 'HAL_SPI_MODULE_ENABLED', 'label': label,
                }
                extra_includes.add('<drivers/com/hal/stm32/hal_spi_stm32.h>')

        elif tag == "sys":
            dispatch_call    = elem.get("dispatch")
            dispatch_include = elem.get("dispatch_include")
            irqn             = elem.get("irqn")
            name             = elem.get("name", "")
            if dispatch_call and irqn:
                handler_map[_irqn_to_handler(irqn)] = {
                    'kind': 'sys_dispatch', 'call': dispatch_call,
                    'guard': '', 'label': name,
                }
                if dispatch_include:
                    extra_includes.add(dispatch_include)

    return handler_map, extra_includes

# ── Handler section classifier ────────────────────────────────────────────────

def _handler_section(name):
    """Return a section banner string for the given handler name, or None if same section."""
    if name.startswith(('WWDG', 'PVD')):
        return 'Watchdog / Power'
    if name.startswith(('TAMP', 'RTC')):
        return 'RTC / Tamper'
    if name.startswith('FLASH'):
        return 'Flash'
    if name.startswith('RCC'):
        return 'RCC'
    if name.startswith('EXTI') and not name.startswith(('EXTI9', 'EXTI15')):
        return 'EXTI — individual lines'
    if name.startswith('DMA1'):
        return 'DMA1 streams'
    if name.startswith('ADC'):
        return 'ADC'
    if name.startswith(('EXTI9', 'EXTI15')):
        return 'EXTI — shared groups'
    if name.startswith('TIM'):
        return 'Timers'
    if name.startswith('I2C'):
        return 'I2C'
    if name.startswith('SPI'):
        return 'SPI'
    if name.startswith('USART'):
        return 'UART'
    if name.startswith('SDIO'):
        return 'SDIO'
    if name.startswith('OTG'):
        return 'USB OTG'
    if name.startswith('DMA2'):
        return 'DMA2 streams'
    if name.startswith('FPU'):
        return 'FPU'
    return 'Miscellaneous'

# ── Generators ────────────────────────────────────────────────────────────────

_BANNER = """\
/*
 * AUTO-GENERATED — DO NOT EDIT
 * Generated by FreeRTOS-OS/scripts/gen_irq_table.py from app/board/irq_table.xml
 * {ts}
 *
 * This file is part of FreeRTOS-OS Project.
 */
"""

def _ts():
    return datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")


def gen_irq_hw_conf_h(cfg, sys_irq_ids, out_path):
    """Generate app/board/irq_hw_conf.h — capacity constants and ID macros."""
    guard = 'APP_BOARD_IRQ_HW_CONF_H_'

    max_uart        = cfg.get('max_uart',        IRQ_MAX_UART)
    uart_events     = cfg.get('uart_events',     IRQ_UART_EVENTS)
    max_i2c         = cfg.get('max_i2c',         IRQ_MAX_I2C)
    i2c_events      = cfg.get('i2c_events',      IRQ_I2C_EVENTS)
    max_spi         = cfg.get('max_spi',         IRQ_MAX_SPI)
    spi_events      = cfg.get('spi_events',      IRQ_SPI_EVENTS)
    max_exti        = cfg.get('max_exti',        IRQ_MAX_EXTI_LINES)
    notify_max_subs = cfg.get('notify_max_subs', IRQ_NOTIFY_MAX_SUBS)

    # Build the sys IRQ ID section and the final IRQ_ID_TOTAL definition.
    if sys_irq_ids:
        sys_lines  = '\n/* ── System IRQ IDs (allocated after EXTI block) ────────────────────── */\n\n'
        sys_lines += '#define IRQ_ID_SYS_BASE    (IRQ_ID_EXTI_BASE + IRQ_MAX_EXTI_LINES)\n'
        for i, (id_name, _irqn, _prio, _flow) in enumerate(sys_irq_ids):
            sys_lines += f'#define IRQ_ID_{id_name:<16} ((irq_id_t)(IRQ_ID_SYS_BASE + {i}))\n'
        n_sys = len(sys_irq_ids)
        sys_lines += f'\n#define IRQ_ID_TOTAL       (IRQ_ID_SYS_BASE + {n_sys})\n'
    else:
        sys_lines  = '\n#define IRQ_ID_TOTAL       (IRQ_ID_EXTI_BASE + IRQ_MAX_EXTI_LINES)\n'

    content = f"""{_BANNER.format(ts=_ts())}
#ifndef {guard}
#define {guard}

#include <stdint.h>

/* ── Capacity constants ──────────────────────────────────────────────────── */

#define IRQ_MAX_UART            {max_uart:<4} /* max UART bus instances     */
#define IRQ_MAX_I2C             {max_i2c:<4} /* max I2C bus instances      */
#define IRQ_MAX_SPI             {max_spi:<4} /* max SPI bus instances      */
#define IRQ_MAX_EXTI_LINES      {max_exti:<4} /* GPIO EXTI lines 0-{max_exti - 1:<2}     */

#define IRQ_UART_EVENTS         {uart_events:<4} /* RX, TX_DONE, ERROR         */
#define IRQ_I2C_EVENTS          {i2c_events:<4} /* TX_DONE, RX_DONE, ERROR    */
#define IRQ_SPI_EVENTS          {spi_events:<4} /* TX_DONE, RX_DONE, TXRX, ERR */

#define IRQ_NOTIFY_MAX_SUBS     {notify_max_subs:<4} /* subscribers per IRQ ID     */

/* ── IRQ ID base offsets ─────────────────────────────────────────────────── */

#define IRQ_ID_UART_BASE   0
#define IRQ_ID_I2C_BASE    (IRQ_ID_UART_BASE + IRQ_MAX_UART * IRQ_UART_EVENTS)
#define IRQ_ID_SPI_BASE    (IRQ_ID_I2C_BASE  + IRQ_MAX_I2C  * IRQ_I2C_EVENTS)
#define IRQ_ID_EXTI_BASE   (IRQ_ID_SPI_BASE  + IRQ_MAX_SPI  * IRQ_SPI_EVENTS)
{sys_lines}
typedef uint32_t irq_id_t;

/* ── ID generator macros ─────────────────────────────────────────────────── */

#define IRQ_ID_UART_RX(n)        ((irq_id_t)(IRQ_ID_UART_BASE + (n)*IRQ_UART_EVENTS + 0))
#define IRQ_ID_UART_TX_DONE(n)   ((irq_id_t)(IRQ_ID_UART_BASE + (n)*IRQ_UART_EVENTS + 1))
#define IRQ_ID_UART_ERROR(n)     ((irq_id_t)(IRQ_ID_UART_BASE + (n)*IRQ_UART_EVENTS + 2))

#define IRQ_ID_I2C_TX_DONE(n)    ((irq_id_t)(IRQ_ID_I2C_BASE  + (n)*IRQ_I2C_EVENTS  + 0))
#define IRQ_ID_I2C_RX_DONE(n)    ((irq_id_t)(IRQ_ID_I2C_BASE  + (n)*IRQ_I2C_EVENTS  + 1))
#define IRQ_ID_I2C_ERROR(n)      ((irq_id_t)(IRQ_ID_I2C_BASE  + (n)*IRQ_I2C_EVENTS  + 2))

#define IRQ_ID_SPI_TX_DONE(n)    ((irq_id_t)(IRQ_ID_SPI_BASE  + (n)*IRQ_SPI_EVENTS  + 0))
#define IRQ_ID_SPI_RX_DONE(n)    ((irq_id_t)(IRQ_ID_SPI_BASE  + (n)*IRQ_SPI_EVENTS  + 1))
#define IRQ_ID_SPI_TXRX_DONE(n)  ((irq_id_t)(IRQ_ID_SPI_BASE  + (n)*IRQ_SPI_EVENTS  + 2))
#define IRQ_ID_SPI_ERROR(n)      ((irq_id_t)(IRQ_ID_SPI_BASE  + (n)*IRQ_SPI_EVENTS  + 3))

/** IRQ_ID_EXTI(pin) — GPIO EXTI line interrupt, pin in [0, {max_exti - 1}]. */
#define IRQ_ID_EXTI(pin)         ((irq_id_t)(IRQ_ID_EXTI_BASE + (pin)))

#endif /* {guard} */
"""
    _write(out_path, content)


def gen_periph_handlers_h(vec_entries, out_path):
    """Generate app/board/irq_periph_handlers_generated.h — weak handler decls."""
    guard = 'APP_BOARD_IRQ_PERIPH_HANDLERS_GENERATED_H_'
    lines = [_BANNER.format(ts=_ts())]
    lines.append(f'#ifndef {guard}\n#define {guard}\n\n')
    lines.append('/*\n')
    lines.append(' * Peripheral IRQ handler weak forward declarations.\n')
    lines.append(' * Include ONLY from the startup compilation unit where\n')
    lines.append(' * Default_Handler is defined in the same translation unit.\n')
    lines.append(' */\n\n')

    seen = set()
    for irqn in sorted(vec_entries.keys()):
        handler, _comment = vec_entries[irqn]
        if handler not in seen:
            seen.add(handler)
            pad = max(1, 42 - len(handler))
            lines.append(
                f'void {handler}(void){" " * pad}'
                f'__attribute__((weak, alias("Default_Handler"), used));\n'
            )

    lines.append(f'\n#endif /* {guard} */\n')
    _write(out_path, ''.join(lines))


def gen_periph_vectors_inc(vec_entries, out_path):
    """Generate app/board/irq_periph_vectors_generated.inc — vector table body."""
    if not vec_entries:
        _write(out_path,
               '/* No peripheral vector entries defined in irq_table.xml */\n')
        return

    max_irqn = max(vec_entries.keys())
    handler_w = max((len(h) for h, _ in vec_entries.values()), default=8) + 1
    lines = [_BANNER.format(ts=_ts())]

    for irqn in range(max_irqn + 1):
        if irqn in vec_entries:
            handler, comment = vec_entries[irqn]
            entry = f'    {handler},'
            pad   = max(1, 4 + handler_w + 1 - len(entry))
            lines.append(f'{entry}{" " * pad}/* [{irqn:>3}] {comment} */\n')
        else:
            pad = max(1, 4 + handler_w + 1 - len('    0,'))
            lines.append(f'    0,{" " * pad}/* [{irqn:>3}] Reserved */\n')

    _write(out_path, ''.join(lines))


def gen_periph_dispatch_c(handler_map, extra_includes, vec_entries, out_path):
    """Generate app/board/irq_periph_dispatch_generated.c — ALL ISR bodies.

    Iterates vec_entries in IRQ-number order and emits a C body for every
    handler: full dispatch for uart/i2c/spi/sys_dispatch entries (from
    handler_map), _EXTI_DISPATCH macro calls for EXTI lines 0-4,
    shared-group calls for EXTI9_5 / EXTI15_10, and empty stubs for
    everything else.
    """
    has_exti_direct = any(
        re.match(r'EXTI[0-4]_IRQHandler', vec_entries[p][0])
        for p in vec_entries
    )

    lines = [_BANNER.format(ts=_ts())]
    lines.append('#include <board/mcu_config.h>\n\n')
    lines.append('#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)\n\n')
    lines.append('#include <device.h>\n')
    if has_exti_direct:
        lines.append('#include <drivers/drv_irq.h>\n')
    for inc in sorted(extra_includes):
        lines.append(f'#include {inc}\n')
    lines.append('\n')

    if has_exti_direct:
        lines.append(
            '#define _EXTI_DISPATCH(pin_mask, exti_line_idx)          \\\n'
            '    do {                                                 \\\n'
            '        BaseType_t _hpt = pdFALSE;                       \\\n'
            '        if (__HAL_GPIO_EXTI_GET_IT(pin_mask)) {          \\\n'
            '            __HAL_GPIO_EXTI_CLEAR_IT(pin_mask);          \\\n'
            '            uint16_t _pin = (pin_mask);                  \\\n'
            '            drv_irq_dispatch_from_isr(                   \\\n'
            '                IRQ_ID_EXTI(exti_line_idx), &_pin, &_hpt); \\\n'
            '        }                                                \\\n'
            '        portYIELD_FROM_ISR(_hpt);                        \\\n'
            '    } while (0)\n\n'
        )

    active_guard   = ''
    active_section = None

    for irqn in sorted(vec_entries.keys()):
        handler, _comment = vec_entries[irqn]

        # ── Classify handler ──────────────────────────────────────────
        if handler in handler_map:
            info  = handler_map[handler]
            kind  = info['kind']
            guard = info.get('guard', '')
        else:
            m = re.match(r'EXTI([0-4])_IRQHandler', handler)
            if m:
                kind  = 'exti_direct'
                guard = ''
                info  = {'n': int(m.group(1))}
            elif handler in ('EXTI9_5_IRQHandler', 'EXTI15_10_IRQHandler'):
                kind  = 'exti_group'
                guard = ''
                info  = {}
            else:
                kind  = 'stub'
                guard = ''
                info  = {}

        # ── Section banner ────────────────────────────────────────────
        section = _handler_section(handler)
        if section != active_section:
            if active_guard:
                lines.append(f'#endif /* {active_guard} */\n\n')
                active_guard = ''
            dash = '─' * max(1, 60 - len(section) - 5)
            lines.append(f'/* ── {section} {dash} */\n\n')
            active_section = section

        # ── Guard transition ──────────────────────────────────────────
        if guard != active_guard:
            if active_guard:
                lines.append(f'#endif /* {active_guard} */\n\n')
            if guard:
                lines.append(f'#ifdef {guard}\n\n')
            active_guard = guard

        # ── Emit body ─────────────────────────────────────────────────
        if kind == 'uart':
            label    = info.get('label', '')
            hal_inst = info['hal_inst']
            if label:
                lines.append(f'/* {label} — {hal_inst} */\n')
            lines.append(f'void {handler}(void) {{ hal_uart_stm32_irq_handler({hal_inst}); }}\n\n')

        elif kind == 'i2c_ev':
            label    = info.get('label', '')
            hal_inst = info['hal_inst']
            if label:
                lines.append(f'/* {label} — {hal_inst} */\n')
            lines.append(f'void {handler}(void) {{ hal_iic_stm32_ev_irq_handler({hal_inst}); }}\n')

        elif kind == 'i2c_er':
            hal_inst = info['hal_inst']
            lines.append(f'void {handler}(void) {{ hal_iic_stm32_er_irq_handler({hal_inst}); }}\n\n')

        elif kind == 'spi':
            label    = info.get('label', '')
            hal_inst = info['hal_inst']
            if label:
                lines.append(f'/* {label} — {hal_inst} */\n')
            lines.append(f'void {handler}(void) {{ hal_spi_stm32_irq_handler({hal_inst}); }}\n\n')

        elif kind == 'sys_dispatch':
            label = info.get('label', '')
            call  = info['call']
            if label:
                lines.append(f'/* {label} */\n')
            lines.append(f'void {handler}(void)\n{{\n    {call};\n}}\n\n')

        elif kind == 'exti_direct':
            n = info['n']
            lines.append(f'void {handler}(void) {{ _EXTI_DISPATCH(GPIO_PIN_{n}, {n}); }}\n')

        elif kind == 'exti_group':
            if handler == 'EXTI9_5_IRQHandler':
                pins = range(5, 10)
            else:
                pins = range(10, 16)
            body = '\n'.join(f'    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_{p});' for p in pins)
            lines.append(f'void {handler}(void)\n{{\n{body}\n}}\n\n')

        elif kind == 'stub':
            lines.append(f'void {handler}(void) {{}}\n')

    if active_guard:
        lines.append(f'#endif /* {active_guard} */\n\n')

    lines.append('\n#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */\n')
    _write(out_path, ''.join(lines))


def gen_irq_table_c(names, sys_irq_ids, out_path):
    lines = [_BANNER.format(ts=_ts())]
    lines.append('#include <irq/irq_table.h>\n')
    lines.append('static const char * const _irq_names[IRQ_ID_TOTAL] = {\n')

    lines.append('\n    /* ── UART ──────────────────────────────────────────────────────── */\n')
    for n in range(IRQ_MAX_UART):
        lines.append(f'    [{uart_rx_macro(n):<28}] = "{names[uart_rx(n)]}",\n')
        lines.append(f'    [{uart_tx_done_macro(n):<28}] = "{names[uart_tx_done(n)]}",\n')
        lines.append(f'    [{uart_error_macro(n):<28}] = "{names[uart_error(n)]}",\n')
        if n < IRQ_MAX_UART - 1:
            lines.append('\n')

    lines.append('\n    /* ── I2C ──────────────────────────────────────────────────────── */\n')
    for n in range(IRQ_MAX_I2C):
        lines.append(f'    [{i2c_tx_done_macro(n):<28}] = "{names[i2c_tx_done(n)]}",\n')
        lines.append(f'    [{i2c_rx_done_macro(n):<28}] = "{names[i2c_rx_done(n)]}",\n')
        lines.append(f'    [{i2c_error_macro(n):<28}] = "{names[i2c_error(n)]}",\n')
        if n < IRQ_MAX_I2C - 1:
            lines.append('\n')

    lines.append('\n    /* ── SPI ──────────────────────────────────────────────────────── */\n')
    for n in range(IRQ_MAX_SPI):
        lines.append(f'    [{spi_tx_done_macro(n):<28}] = "{names[spi_tx_done(n)]}",\n')
        lines.append(f'    [{spi_rx_done_macro(n):<28}] = "{names[spi_rx_done(n)]}",\n')
        lines.append(f'    [{spi_txrx_done_macro(n):<28}] = "{names[spi_txrx_done(n)]}",\n')
        lines.append(f'    [{spi_error_macro(n):<28}] = "{names[spi_error(n)]}",\n')
        if n < IRQ_MAX_SPI - 1:
            lines.append('\n')

    lines.append('\n    /* ── GPIO EXTI lines ──────────────────────────────────────────── */\n')
    for n in range(IRQ_MAX_EXTI_LINES):
        lines.append(f'    [{exti_macro(n):<28}] = "{names[IRQ_ID_EXTI_BASE + n]}",\n')

    if sys_irq_ids:
        lines.append('\n    /* ── System IRQ IDs ───────────────────────────────────────────── */\n')
        for id_name, _irqn, _prio, _flow in sys_irq_ids:
            macro = f'IRQ_ID_{id_name}'
            lines.append(f'    [{macro:<28}] = "{id_name}",\n')

    lines.append('};\n\n')
    lines.append('const char *irq_table_get_name(irq_id_t id)\n')
    lines.append('{\n')
    lines.append('    if (id >= IRQ_ID_TOTAL)\n')
    lines.append('        return "INVALID";\n')
    lines.append('    return _irq_names[id] ? _irq_names[id] : "UNKNOWN";\n')
    lines.append('}\n')

    _write(out_path, ''.join(lines))


def gen_irq_hw_init_c(hw_irqs, sys_irqs, sys_irq_ids, clk_info, out_path):
    """Generate irq_hw_init_all():
      1. RCC clock enables   (sys, peripheral, GPIO port)
      2. irq_desc_init_all()
      3. irq_chip_nvic binding + NVIC priority setup
    """
    lines = [_BANNER.format(ts=_ts())]
    lines.append('#include <board/mcu_config.h>\n\n')
    lines.append('#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)\n\n')
    lines.append('#include <device.h>\n')
    lines.append('#include <irq/irq_desc.h>\n')
    lines.append('#include <drivers/hal/stm32/irq_chip_nvic.h>\n\n')

    lines.append('void irq_hw_init_all(void)\n{\n')
    lines.append('    irq_desc_init_all();\n')

    if hw_irqs:
        lines.append('\n    /* ── Bind irq_chip_nvic to software IRQ IDs ──────────────────────── */\n')
        prev_label = None
        for macro, irqn, prio, label in hw_irqs:
            if label is not None and label != prev_label:
                lines.append(f'\n    /* {label} — {irqn}, priority {prio} */\n')
                prev_label = label
            lines.append(f'    irq_set_chip_and_handler({macro:<28}, irq_chip_nvic_get(), handle_simple_irq);\n')

        lines.append('\n    /* ── Set NVIC priorities for hardware IRQ lines ──────────────────── */\n')
        prev_label = None
        for macro, irqn, prio, label in hw_irqs:
            if label is not None and label != prev_label:
                lines.append(f'\n    /* {label} — {irqn} */\n')
                prev_label = label
            lines.append(f'    irq_chip_nvic_bind_hwirq({macro:<28}, {irqn}, {prio}U);\n')

    if sys_irqs:
        lines.append('\n    /* ── System / core IRQ priorities ────────────────────────────────── */\n')
        for s in sys_irqs:
            name   = s["name"]
            irqn   = s["irqn"]
            prio   = s["priority"]
            action = s["action"]
            lines.append(f'\n    /* {name} */\n')
            lines.append(f'    HAL_NVIC_SetPriority({irqn}, {prio}U, 0U);\n')
            if action == "enable":
                lines.append(f'    HAL_NVIC_EnableIRQ({irqn});\n')

    if sys_irq_ids:
        lines.append('\n    /* ── Bind irq_chip_nvic to system IRQ IDs ────────────────────────── */\n')
        for id_name, irqn, prio, flow in sys_irq_ids:
            macro      = f'IRQ_ID_{id_name}'
            handler_fn = 'handle_edge_irq' if flow == 'edge' else 'handle_simple_irq'
            lines.append(f'\n    /* {id_name} — {irqn}, priority {prio}, {flow} flow */\n')
            lines.append(f'    irq_set_chip_and_handler({macro:<28}, irq_chip_nvic_get(), {handler_fn});\n')
            lines.append(f'    irq_chip_nvic_bind_hwirq({macro:<28}, {irqn}, {prio}U);\n')

    lines.append('}\n\n')
    lines.append('#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */\n')

    _write(out_path, ''.join(lines))


def gen_irq_hw_init_h(out_path):
    guard = 'APP_BOARD_IRQ_HW_INIT_GENERATED_H_'
    content = f"""{_BANNER.format(ts=_ts())}
#ifndef {guard}
#define {guard}

#ifdef __cplusplus
extern "C" {{
#endif

/**
 * irq_hw_init_all — initialise the irq_desc table, bind irq_chip_nvic to
 *                   every hardware-backed software IRQ ID, and configure NVIC
 *                   priorities.
 *
 * Call once at boot after board_clk_enable() and before any request_irq()
 * or drv_irq_register() calls.
 * Generated from app/board/irq_table.xml by scripts/gen_irq_table.py.
 */
void irq_hw_init_all(void);

#ifdef __cplusplus
}}
#endif

#endif /* {guard} */
"""
    _write(out_path, content)

# ── Helpers ───────────────────────────────────────────────────────────────────

def _write(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f:
        f.write(content)
    print(f"  wrote  {path}")

def _die(msg):
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)

# ── Entry point ───────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <path/to/irq_table.xml>", file=sys.stderr)
        sys.exit(1)

    xml_path = sys.argv[1]
    if not os.path.isfile(xml_path):
        _die(f"XML file not found: {xml_path}")

    # Script lives at FreeRTOS-OS/scripts/ — project root is two levels up
    script_dir   = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(os.path.dirname(script_dir))

    out_irq_hw_conf_h         = os.path.join(project_root, "app", "board", "irq_hw_conf.h")
    out_irq_table_c           = os.path.join(project_root, "FreeRTOS-OS", "irq", "irq_table.c")
    out_hw_init_c             = os.path.join(project_root, "app", "board", "irq_hw_init_generated.c")
    out_hw_init_h             = os.path.join(project_root, "app", "board", "irq_hw_init_generated.h")
    out_periph_handlers_h     = os.path.join(project_root, "app", "board", "irq_periph_handlers_generated.h")
    out_periph_vectors_inc    = os.path.join(project_root, "app", "board", "irq_periph_vectors_generated.inc")
    out_periph_dispatch_c     = os.path.join(project_root, "app", "board", "irq_periph_dispatch_generated.c")

    print(f"gen_irq_table: reading {xml_path}")
    tree = ET.parse(xml_path)
    root = tree.getroot()

    cfg = parse_config_elem(root)
    _update_globals(cfg)

    names, hw_irqs, sys_irqs, clk_info  = parse_xml_elements(root)
    vec_entries                         = parse_vector_table_elem(root)
    handler_map, extra_includes         = parse_dispatch_data(root)

    # Collect sys elements that carry a software IRQ ID (irq_id attribute).
    sys_irq_ids = [
        (s["irq_id"], s["irqn"], s["priority"], s["flow"])
        for s in sys_irqs
        if s.get("irq_id")
    ]

    gen_irq_hw_conf_h(cfg, sys_irq_ids, out_irq_hw_conf_h)
    gen_periph_handlers_h(vec_entries, out_periph_handlers_h)
    gen_periph_vectors_inc(vec_entries, out_periph_vectors_inc)
    gen_periph_dispatch_c(handler_map, extra_includes, vec_entries, out_periph_dispatch_c)
    gen_irq_table_c(names, sys_irq_ids, out_irq_table_c)
    gen_irq_hw_init_c(hw_irqs, sys_irqs, sys_irq_ids, clk_info, out_hw_init_c)
    gen_irq_hw_init_h(out_hw_init_h)

    print("gen_irq_table: done.")

if __name__ == "__main__":
    main()
