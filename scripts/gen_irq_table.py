#!/usr/bin/env python3
"""
gen_irq_table.py — IRQ table and NVIC init code generator

Reads app/board/irq_table.xml and generates two C source files:

  FreeRTOS-OS/irq/irq_table.c
      Static IRQ name table indexed by software irq_id_t.
      Replaces the hand-written version.

  app/board/irq_hw_init_generated.c  +  irq_hw_init_generated.h
      irq_hw_init_all() — binds irq_chip_nvic to each software IRQ ID,
      sets NVIC priorities, and enables hardware interrupt lines.
      Call once at boot before any request_irq().

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
import xml.etree.ElementTree as ET
from datetime import datetime, timezone

# ── Constants matching irq_notify.h ──────────────────────────────────────────

IRQ_MAX_UART       = 4
IRQ_MAX_I2C        = 4
IRQ_MAX_SPI        = 4
IRQ_MAX_EXTI_LINES = 16

IRQ_UART_EVENTS = 3   # RX, TX_DONE, ERROR
IRQ_I2C_EVENTS  = 3   # TX_DONE, RX_DONE, ERROR
IRQ_SPI_EVENTS  = 4   # TX_DONE, RX_DONE, TXRX_DONE, ERROR

IRQ_ID_UART_BASE = 0
IRQ_ID_I2C_BASE  = IRQ_ID_UART_BASE + IRQ_MAX_UART * IRQ_UART_EVENTS
IRQ_ID_SPI_BASE  = IRQ_ID_I2C_BASE  + IRQ_MAX_I2C  * IRQ_I2C_EVENTS
IRQ_ID_EXTI_BASE = IRQ_ID_SPI_BASE  + IRQ_MAX_SPI  * IRQ_SPI_EVENTS
IRQ_ID_TOTAL     = IRQ_ID_EXTI_BASE + IRQ_MAX_EXTI_LINES

# ── Derived ID helpers ────────────────────────────────────────────────────────

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

# ── Build default name table ──────────────────────────────────────────────────

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

# ── Parse XML ─────────────────────────────────────────────────────────────────

def parse_xml(xml_path):
    tree = ET.parse(xml_path)
    root = tree.getroot()

    names   = build_default_names()   # irq_id → display name
    hw_irqs = []                      # list of (irq_id_macro, irqn, priority)
    sys_irqs = []                     # list of (name, irqn, priority, action)

    for elem in root:
        tag = elem.tag

        if tag == "uart":
            n    = int(elem.get("dev_id"))
            lbl  = elem.get("label", f"UART{n}")
            irqn = elem.get("irqn")
            prio = int(elem.get("priority", "5"))

            if n >= IRQ_MAX_UART:
                _die(f"uart dev_id={n} exceeds IRQ_MAX_UART={IRQ_MAX_UART}")

            names[uart_rx(n)]      = f"{lbl}_RX"
            names[uart_tx_done(n)] = f"{lbl}_TX_DONE"
            names[uart_error(n)]   = f"{lbl}_ERROR"

            if irqn:
                hw_irqs.append((uart_rx_macro(n),      irqn, prio, lbl))
                hw_irqs.append((uart_tx_done_macro(n), irqn, prio, None))
                hw_irqs.append((uart_error_macro(n),   irqn, prio, None))

        elif tag == "i2c":
            n       = int(elem.get("dev_id"))
            lbl     = elem.get("label", f"I2C{n}")
            irqn_ev = elem.get("irqn_ev")
            prio_ev = int(elem.get("priority_ev", "5"))
            irqn_er = elem.get("irqn_er")
            prio_er = int(elem.get("priority_er", "5"))

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

        elif tag == "spi":
            n    = int(elem.get("dev_id"))
            lbl  = elem.get("label", f"SPI{n}")
            irqn = elem.get("irqn")
            prio = int(elem.get("priority", "5"))

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
            })

    return names, hw_irqs, sys_irqs

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

def gen_irq_table_c(names, out_path):
    lines = [_BANNER.format(ts=_ts())]
    lines.append('#include <irq/irq_table.h>\n')
    lines.append('static const char * const _irq_names[IRQ_ID_TOTAL] = {\n')

    # UART block
    lines.append('\n    /* ── UART ──────────────────────────────────────────────────────── */\n')
    for n in range(IRQ_MAX_UART):
        lines.append(f'    [{uart_rx_macro(n):<28}] = "{names[uart_rx(n)]}",\n')
        lines.append(f'    [{uart_tx_done_macro(n):<28}] = "{names[uart_tx_done(n)]}",\n')
        lines.append(f'    [{uart_error_macro(n):<28}] = "{names[uart_error(n)]}",\n')
        if n < IRQ_MAX_UART - 1:
            lines.append('\n')

    # I2C block
    lines.append('\n    /* ── I2C ──────────────────────────────────────────────────────── */\n')
    for n in range(IRQ_MAX_I2C):
        lines.append(f'    [{i2c_tx_done_macro(n):<28}] = "{names[i2c_tx_done(n)]}",\n')
        lines.append(f'    [{i2c_rx_done_macro(n):<28}] = "{names[i2c_rx_done(n)]}",\n')
        lines.append(f'    [{i2c_error_macro(n):<28}] = "{names[i2c_error(n)]}",\n')
        if n < IRQ_MAX_I2C - 1:
            lines.append('\n')

    # SPI block
    lines.append('\n    /* ── SPI ──────────────────────────────────────────────────────── */\n')
    for n in range(IRQ_MAX_SPI):
        lines.append(f'    [{spi_tx_done_macro(n):<28}] = "{names[spi_tx_done(n)]}",\n')
        lines.append(f'    [{spi_rx_done_macro(n):<28}] = "{names[spi_rx_done(n)]}",\n')
        lines.append(f'    [{spi_txrx_done_macro(n):<28}] = "{names[spi_txrx_done(n)]}",\n')
        lines.append(f'    [{spi_error_macro(n):<28}] = "{names[spi_error(n)]}",\n')
        if n < IRQ_MAX_SPI - 1:
            lines.append('\n')

    # EXTI block
    lines.append('\n    /* ── GPIO EXTI lines ──────────────────────────────────────────── */\n')
    for n in range(IRQ_MAX_EXTI_LINES):
        lines.append(f'    [{exti_macro(n):<28}] = "{names[IRQ_ID_EXTI_BASE + n]}",\n')

    lines.append('};\n\n')
    lines.append('const char *irq_table_get_name(irq_id_t id)\n')
    lines.append('{\n')
    lines.append('    if (id >= IRQ_ID_TOTAL)\n')
    lines.append('        return "INVALID";\n')
    lines.append('    return _irq_names[id] ? _irq_names[id] : "UNKNOWN";\n')
    lines.append('}\n')

    _write(out_path, ''.join(lines))


def gen_irq_hw_init_c(hw_irqs, sys_irqs, out_path):
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
 * Call once at boot before any request_irq() or drv_irq_register() calls.
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

    out_irq_table_c   = os.path.join(project_root, "FreeRTOS-OS", "irq", "irq_table.c")
    out_hw_init_c     = os.path.join(project_root, "app", "board", "irq_hw_init_generated.c")
    out_hw_init_h     = os.path.join(project_root, "app", "board", "irq_hw_init_generated.h")

    print(f"gen_irq_table: reading {xml_path}")
    names, hw_irqs, sys_irqs = parse_xml(xml_path)

    gen_irq_table_c(names, out_irq_table_c)
    gen_irq_hw_init_c(hw_irqs, sys_irqs, out_hw_init_c)
    gen_irq_hw_init_h(out_hw_init_h)

    print("gen_irq_table: done.")

if __name__ == "__main__":
    main()
