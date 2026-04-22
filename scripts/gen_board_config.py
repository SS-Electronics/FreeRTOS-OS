#!/usr/bin/env python3
"""
gen_board_config.py — Board configuration code generator for FreeRTOS-OS
=========================================================================

Reads a board XML description and generates two files:

  boards/<board_name>/board_config.c   — peripheral tables, API, callback tables
  include/board/board_device_ids.h     — user-facing device-ID #defines

Supports multiple vendors via pluggable codegen backends:
  STM      → STM32Codegen      (STM32 HAL constants, GPIO_AF, __HAL_RCC_*)
  INFINEON → InfineonCodegen   (XMC USIC channel stubs)
  MICROCHIP→ MicrochipCodegen  (SERCOM / USART stubs)

Usage
-----
  python3 scripts/gen_board_config.py boards/stm32f411_devboard.xml
  python3 scripts/gen_board_config.py boards/my_board.xml --outdir boards/my_board

Makefile integration
--------------------
  make board-gen               # re-generate from XML, then compile
  make CONFIG_BOARD=my_board board-gen all
"""

import xml.etree.ElementTree as ET
import argparse
import os
import re
import sys
from datetime import date
from pathlib import Path


# ──────────────────────────────────────────────────────────────────────────────
#  File header banner
# ──────────────────────────────────────────────────────────────────────────────

_BANNER = """\
/*
 * AUTO-GENERATED — DO NOT EDIT
 * Generator : scripts/gen_board_config.py
 * Source    : {xml_path}
 * Date      : {date}
 *
 * Re-generate:
 *   python3 scripts/gen_board_config.py {xml_path}
 */
"""

_INC_GUARD_OPEN  = "#ifndef {guard}\n#define {guard}\n"
_INC_GUARD_CLOSE = "#endif /* {guard} */\n"


# ──────────────────────────────────────────────────────────────────────────────
#  Vendor code-generation backends
# ──────────────────────────────────────────────────────────────────────────────

class VendorCodegen:
    """Abstract base — vendor-specific translation of XML attributes to C."""

    # ── Peripheral clock enable ──────────────────────────────────────────────

    def periph_clk_fn(self, periph_type: str, instance: str):
        """Return (fn_name: str, fn_definition: str) for a clock-enable wrapper."""
        raise NotImplementedError

    def all_clk_enable_body(self, ports: set, uarts: list, i2cs: list, spis: list) -> str:
        """Return body of board_clk_enable(void) — all RCC clock enables."""
        raise NotImplementedError

    # ── GPIO ─────────────────────────────────────────────────────────────────

    def gpio_port(self, letter: str) -> str:
        raise NotImplementedError

    def gpio_pin(self, num: str) -> str:
        raise NotImplementedError

    def gpio_speed(self, speed: str) -> str:
        raise NotImplementedError

    def gpio_pull(self, pull: str) -> str:
        raise NotImplementedError

    def gpio_af_mode(self, is_od=False) -> str:
        raise NotImplementedError

    def gpio_out_mode(self, is_od=False) -> str:
        raise NotImplementedError

    def gpio_in_mode(self) -> str:
        raise NotImplementedError

    def gpio_it_mode(self, edge: str) -> str:
        raise NotImplementedError

    def gpio_active_state(self, state: str) -> str:
        raise NotImplementedError

    def af_constant(self, af_num: str, instance: str) -> str:
        raise NotImplementedError

    # ── UART ─────────────────────────────────────────────────────────────────

    def uart_wordlen(self, bits: str) -> str:
        raise NotImplementedError

    def uart_stopbits(self, n: str) -> str:
        raise NotImplementedError

    def uart_parity(self, p: str) -> str:
        raise NotImplementedError

    def uart_mode(self, m: str) -> str:
        raise NotImplementedError

    # ── I2C ──────────────────────────────────────────────────────────────────

    def i2c_addr_mode(self, m: str) -> str:
        raise NotImplementedError

    def i2c_dual_addr(self, m: str) -> str:
        raise NotImplementedError

    # ── SPI ──────────────────────────────────────────────────────────────────

    def spi_mode(self, m: str) -> str:       raise NotImplementedError
    def spi_direction(self, d: str) -> str:  raise NotImplementedError
    def spi_datasize(self, n: str) -> str:   raise NotImplementedError
    def spi_polarity(self, p: str) -> str:   raise NotImplementedError
    def spi_phase(self, p: str) -> str:      raise NotImplementedError
    def spi_nss(self, n: str) -> str:        raise NotImplementedError
    def spi_prescaler(self, n: str) -> str:  raise NotImplementedError
    def spi_bitorder(self, b: str) -> str:   raise NotImplementedError


    # ── HAL handle generation ────────────────────────────────────────────────

    def hal_handle_type(self, periph_type: str) -> str:
        """Return the HAL handle typedef name for the given peripheral type."""
        return ''

    def hal_handle_name(self, instance: str) -> str:
        """Convert an instance name (e.g. 'USART1') to a handle name ('huart1')."""
        return ''

    def hal_handle_guard(self, periph_type: str) -> str:
        """Return the #ifdef guard for the HAL module (e.g. 'HAL_UART_MODULE_ENABLED')."""
        return ''

    def hal_handle_includes(self, periph_type: str) -> list:
        """Return list of #include lines required by this peripheral's handle type."""
        return []


# ── STM32 ─────────────────────────────────────────────────────────────────────

class STM32Codegen(VendorCodegen):

    _SPEED = {
        'low':       'GPIO_SPEED_FREQ_LOW',
        'medium':    'GPIO_SPEED_FREQ_MEDIUM',
        'high':      'GPIO_SPEED_FREQ_HIGH',
        'very_high': 'GPIO_SPEED_FREQ_VERY_HIGH',
    }
    _PULL = {
        'none':     'GPIO_NOPULL',
        'pullup':   'GPIO_PULLUP',
        'pulldown': 'GPIO_PULLDOWN',
    }
    _ACTIVE = {'high': 'GPIO_PIN_SET', 'low': 'GPIO_PIN_RESET'}
    _WORDLEN  = {'8': 'UART_WORDLENGTH_8B', '9': 'UART_WORDLENGTH_9B'}
    _STOPBITS = {'1': 'UART_STOPBITS_1', '2': 'UART_STOPBITS_2'}
    _PARITY   = {'none': 'UART_PARITY_NONE', 'even': 'UART_PARITY_EVEN', 'odd': 'UART_PARITY_ODD'}
    _UMODE    = {'tx_rx': 'UART_MODE_TX_RX', 'tx': 'UART_MODE_TX', 'rx': 'UART_MODE_RX'}
    _I2C_ADDR = {'7bit': 'I2C_ADDRESSINGMODE_7BIT', '10bit': 'I2C_ADDRESSINGMODE_10BIT'}
    _I2C_DUAL = {'disable': 'I2C_DUALADDRESS_DISABLE', 'enable': 'I2C_DUALADDRESS_ENABLE'}
    _SPI_MODE = {'master': 'SPI_MODE_MASTER', 'slave': 'SPI_MODE_SLAVE'}
    _SPI_DIR  = {'2lines': 'SPI_DIRECTION_2LINES', '1line': 'SPI_DIRECTION_1LINE'}
    _SPI_DS   = {'8': 'SPI_DATASIZE_8BIT', '16': 'SPI_DATASIZE_16BIT'}
    _SPI_POL  = {'low': 'SPI_POLARITY_LOW', 'high': 'SPI_POLARITY_HIGH'}
    _SPI_PHA  = {'1edge': 'SPI_PHASE_1EDGE', '2edge': 'SPI_PHASE_2EDGE'}
    _SPI_NSS  = {'soft': 'SPI_NSS_SOFT', 'hard_input': 'SPI_NSS_HARD_INPUT', 'hard_output': 'SPI_NSS_HARD_OUTPUT'}
    _SPI_PRE  = {str(2**i): f'SPI_BAUDRATEPRESCALER_{2**i}' for i in range(1, 9)}
    _SPI_BIT  = {'msb': 'SPI_FIRSTBIT_MSB', 'lsb': 'SPI_FIRSTBIT_LSB'}

    def periph_clk_fn(self, periph_type, instance):
        fn  = f'_{instance.lower()}_clk_en'
        body = f'static void {fn}(void) {{ __HAL_RCC_{instance}_CLK_ENABLE(); }}'
        return fn, body

    def all_clk_enable_body(self, ports, uarts, i2cs, spis):
        lines = ['    /* System bus clocks */',
                 '    __HAL_RCC_SYSCFG_CLK_ENABLE();',
                 '    __HAL_RCC_PWR_CLK_ENABLE();']
        if uarts:
            lines += ['', '#ifdef HAL_UART_MODULE_ENABLED',
                      '    /* UART peripheral clocks */']
            for inst in uarts:
                lines.append(f'    __HAL_RCC_{inst}_CLK_ENABLE();')
            lines.append('#endif /* HAL_UART_MODULE_ENABLED */')
        if i2cs:
            lines += ['', '#ifdef HAL_I2C_MODULE_ENABLED',
                      '    /* I2C peripheral clocks */']
            for inst in i2cs:
                lines.append(f'    __HAL_RCC_{inst}_CLK_ENABLE();')
            lines.append('#endif /* HAL_I2C_MODULE_ENABLED */')
        if spis:
            lines += ['', '#ifdef HAL_SPI_MODULE_ENABLED',
                      '    /* SPI peripheral clocks */']
            for inst in spis:
                lines.append(f'    __HAL_RCC_{inst}_CLK_ENABLE();')
            lines.append('#endif /* HAL_SPI_MODULE_ENABLED */')
        if ports:
            lines += ['', '    /* GPIO port clocks for alternate-function peripheral pins */']
            for p in sorted(ports):
                lines.append(f'    __HAL_RCC_GPIO{p}_CLK_ENABLE();')
        return '\n'.join(lines)

    def gpio_port(self, letter):         return f'GPIO{letter.upper()}'
    def gpio_pin(self, num):             return f'GPIO_PIN_{num}'
    def gpio_speed(self, s):             return self._SPEED.get(s.lower(), 'GPIO_SPEED_FREQ_LOW')
    def gpio_pull(self, p):              return self._PULL.get(p.lower(), 'GPIO_NOPULL')
    def gpio_af_mode(self, is_od=False): return 'GPIO_MODE_AF_OD' if is_od else 'GPIO_MODE_AF_PP'
    def gpio_out_mode(self, is_od=False):return 'GPIO_MODE_OUTPUT_OD' if is_od else 'GPIO_MODE_OUTPUT_PP'
    def gpio_in_mode(self):              return 'GPIO_MODE_INPUT'
    def gpio_it_mode(self, edge):
        return {'rising':'GPIO_MODE_IT_RISING','falling':'GPIO_MODE_IT_FALLING',
                'both':'GPIO_MODE_IT_RISING_FALLING'}.get(edge.lower(),'GPIO_MODE_IT_RISING')
    def gpio_active_state(self, s):      return self._ACTIVE.get(s.lower(), 'GPIO_PIN_SET')
    def af_constant(self, af, instance): return f'GPIO_AF{af}_{instance}'

    def uart_wordlen(self, b):  return self._WORDLEN.get(str(b), 'UART_WORDLENGTH_8B')
    def uart_stopbits(self, n): return self._STOPBITS.get(str(n), 'UART_STOPBITS_1')
    def uart_parity(self, p):   return self._PARITY.get(p.lower(), 'UART_PARITY_NONE')
    def uart_mode(self, m):     return self._UMODE.get(m.lower(), 'UART_MODE_TX_RX')

    def i2c_addr_mode(self, m): return self._I2C_ADDR.get(m.lower(), 'I2C_ADDRESSINGMODE_7BIT')
    def i2c_dual_addr(self, m): return self._I2C_DUAL.get(m.lower(), 'I2C_DUALADDRESS_DISABLE')

    def spi_mode(self, m):        return self._SPI_MODE.get(m.lower(), 'SPI_MODE_MASTER')
    def spi_direction(self, d):   return self._SPI_DIR.get(d.lower(), 'SPI_DIRECTION_2LINES')
    def spi_datasize(self, n):    return self._SPI_DS.get(str(n), 'SPI_DATASIZE_8BIT')
    def spi_polarity(self, p):    return self._SPI_POL.get(p.lower(), 'SPI_POLARITY_LOW')
    def spi_phase(self, p):       return self._SPI_PHA.get(p.lower(), 'SPI_PHASE_1EDGE')
    def spi_nss(self, n):         return self._SPI_NSS.get(n.lower(), 'SPI_NSS_SOFT')
    def spi_prescaler(self, n):   return self._SPI_PRE.get(str(n), 'SPI_BAUDRATEPRESCALER_16')
    def spi_bitorder(self, b):    return self._SPI_BIT.get(b.lower(), 'SPI_FIRSTBIT_MSB')

    _HAL_HANDLE_TYPE = {
        'uart': 'UART_HandleTypeDef',
        'i2c':  'I2C_HandleTypeDef',
        'spi':  'SPI_HandleTypeDef',
    }
    _HAL_MODULE_GUARD = {
        'uart': 'HAL_UART_MODULE_ENABLED',
        'i2c':  'HAL_I2C_MODULE_ENABLED',
        'spi':  'HAL_SPI_MODULE_ENABLED',
    }
    _HAL_INCLUDE = {
        'uart': 'stm32f4xx_hal_uart.h',
        'i2c':  'stm32f4xx_hal_i2c.h',
        'spi':  'stm32f4xx_hal_spi.h',
    }

    def hal_handle_type(self, periph_type: str) -> str:
        return self._HAL_HANDLE_TYPE.get(periph_type.lower(), '')

    def hal_handle_name(self, instance: str) -> str:
        m = re.match(r'(USART|I2C|SPI|UART)(\d+)', instance, re.IGNORECASE)
        if m:
            prefix = m.group(1).lower()
            if prefix == 'usart':
                prefix = 'uart'
            return f'h{prefix}{m.group(2)}'
        return f'h{instance.lower()}'

    def hal_handle_guard(self, periph_type: str) -> str:
        return self._HAL_MODULE_GUARD.get(periph_type.lower(), '')

    def hal_handle_includes(self, periph_type: str) -> list:
        inc = self._HAL_INCLUDE.get(periph_type.lower(), '')
        return [f'#include "{inc}"'] if inc else []


# ── Infineon XMC ──────────────────────────────────────────────────────────────

class InfineonCodegen(VendorCodegen):
    """Infineon XMC series — USIC-based peripherals, baremetal register access."""

    def periph_clk_fn(self, periph_type, instance):
        fn   = f'_{instance.lower().replace("_","").replace(" ","")}_clk_en'
        body = (f'static void {fn}(void) {{\n'
                f'    /* TODO: Infineon {instance} clock / module enable\n'
                f'     * e.g.: XMC_SCU_CLOCK_EnableModule(XMC_SCU_MODULE_{instance}); */\n'
                f'}}')
        return fn, body

    def all_clk_enable_body(self, ports, uarts, i2cs, spis):
        return '    /* INFO: Infineon XMC GPIO ports are always clocked — no enable needed. */'

    def gpio_port(self, letter):          return f'XMC_GPIO_PORT{letter.upper()}'
    def gpio_pin(self, num):              return str(num)
    def gpio_speed(self, s):
        return {'low':'XMC_GPIO_OUTPUT_STRENGTH_MEDIUM','high':'XMC_GPIO_OUTPUT_STRENGTH_STRONG_SHARP_EDGE',
                'very_high':'XMC_GPIO_OUTPUT_STRENGTH_STRONG_SHARP_EDGE'}.get(s.lower(),'XMC_GPIO_OUTPUT_STRENGTH_MEDIUM')
    def gpio_pull(self, p):
        return {'none':'XMC_GPIO_MODE_INPUT_TRISTATE','pullup':'XMC_GPIO_MODE_INPUT_PULL_UP',
                'pulldown':'XMC_GPIO_MODE_INPUT_PULL_DOWN'}.get(p.lower(),'XMC_GPIO_MODE_INPUT_TRISTATE')
    def gpio_af_mode(self, is_od=False):  return 'XMC_GPIO_MODE_OUTPUT_ALT1  /* TODO: set correct ALT */'
    def gpio_out_mode(self, is_od=False): return 'XMC_GPIO_MODE_OUTPUT_PUSH_PULL'
    def gpio_in_mode(self):               return 'XMC_GPIO_MODE_INPUT_TRISTATE'
    def gpio_it_mode(self, edge):         return 'XMC_GPIO_MODE_INPUT_TRISTATE  /* TODO: ERU config needed */'
    def gpio_active_state(self, s):       return '1' if s.lower() == 'high' else '0'
    def af_constant(self, af, instance):  return f'{af}  /* Infineon ALT{af} for {instance} */'

    def uart_wordlen(self, b):   return f'{b}U  /* XMC_USIC_CH_WORD_LENGTH_{b} */'
    def uart_stopbits(self, n):  return f'{n}U  /* XMC_USIC_CH_STOP_BITS_{n} */'
    def uart_parity(self, p):
        return {'none':'XMC_USIC_CH_PARITY_MODE_NONE','even':'XMC_USIC_CH_PARITY_MODE_EVEN',
                'odd':'XMC_USIC_CH_PARITY_MODE_ODD'}.get(p.lower(),'XMC_USIC_CH_PARITY_MODE_NONE')
    def uart_mode(self, m): return '0U  /* TODO: Infineon UART mode flags */'

    def i2c_addr_mode(self, m): return '0U  /* TODO: XMC I2C address mode */'
    def i2c_dual_addr(self, m): return '0U'

    def spi_mode(self, m):       return '0U  /* TODO: XMC SPI mode */'
    def spi_direction(self, d):  return '0U'
    def spi_datasize(self, n):   return f'{n}U'
    def spi_polarity(self, p):   return '0U'
    def spi_phase(self, p):      return '0U'
    def spi_nss(self, n):        return '0U'
    def spi_prescaler(self, n):  return f'{n}U'
    def spi_bitorder(self, b):   return '0U'



# ── Microchip SAM / PIC32 ─────────────────────────────────────────────────────

class MicrochipCodegen(VendorCodegen):
    """Microchip SAM (ASF4/START) and PIC32 (Harmony) — stub implementation."""

    def periph_clk_fn(self, periph_type, instance):
        fn   = f'_{instance.lower()}_clk_en'
        body = (f'static void {fn}(void) {{\n'
                f'    /* TODO: Microchip {instance} clock enable\n'
                f'     * SAM:  hri_mclk_set_APBCMASK_{instance}_bit(MCLK);\n'
                f'     * PIC32: PMD registers / SFR unlock sequence */\n'
                f'}}')
        return fn, body

    def all_clk_enable_body(self, ports, uarts, i2cs, spis):
        return '    /* TODO: Microchip — enable GPIO port clocks via PORT/GPIO SFRs */'

    def gpio_port(self, letter):          return f'PORT_{letter.upper()}'
    def gpio_pin(self, num):              return f'PIN_P{num}'
    def gpio_speed(self, s):              return '0U  /* TODO: Microchip drive strength */'
    def gpio_pull(self, p):
        return {'none':'PORT_PIN_PULL_OFF','pullup':'PORT_PIN_PULL_UP',
                'pulldown':'PORT_PIN_PULL_DOWN'}.get(p.lower(),'PORT_PIN_PULL_OFF')
    def gpio_af_mode(self, is_od=False):  return '0U  /* TODO: Microchip MUX function */'
    def gpio_out_mode(self, is_od=False): return 'GPIO_OUTPUT  /* TODO: Microchip output mode */'
    def gpio_in_mode(self):               return 'GPIO_INPUT'
    def gpio_it_mode(self, edge):         return 'GPIO_INPUT  /* TODO: EIC config */'
    def gpio_active_state(self, s):       return '1U' if s.lower() == 'high' else '0U'
    def af_constant(self, af, instance):  return f'{af}  /* TODO: Microchip MUX{af} */'

    def uart_wordlen(self, b):   return f'{b}U'
    def uart_stopbits(self, n):  return f'{n}U'
    def uart_parity(self, p):
        return {'none':'USART_PARITY_NO','even':'USART_PARITY_EVEN',
                'odd':'USART_PARITY_ODD'}.get(p.lower(),'USART_PARITY_NO')
    def uart_mode(self, m): return '0U'

    def i2c_addr_mode(self, m): return '0U'
    def i2c_dual_addr(self, m): return '0U'

    def spi_mode(self, m):       return '0U'
    def spi_direction(self, d):  return '0U'
    def spi_datasize(self, n):   return f'{n}U'
    def spi_polarity(self, p):   return '0U'
    def spi_phase(self, p):      return '0U'
    def spi_nss(self, n):        return '0U'
    def spi_prescaler(self, n):  return f'{n}U'
    def spi_bitorder(self, b):   return '0U'



# ──────────────────────────────────────────────────────────────────────────────
#  Main generator
# ──────────────────────────────────────────────────────────────────────────────

_VENDOR_MAP = {
    'STM':       STM32Codegen,
    'INFINEON':  InfineonCodegen,
    'MICROCHIP': MicrochipCodegen,
}

# MCU → hardware-fixed peripheral counts and UART port mapping.
# Extend this table when adding support for new MCU variants.
_MCU_PERIPH_MAP = {
    'STM32F411VET6': {
        'vendor_var':    'MCU_VAR_STM',
        'max_uart':      3,
        'max_iic':       3,
        'max_spi':       5,
        'max_tim':       8,
        'note':          'No CAN, No DAC, No FMC, No ETH on STM32F411',
        # Ordered list of UART_N identifiers that map to physical USART instances
        # (activation order: 1st entry enabled when NO_OF_UART>=1, etc.)
        'uart_en_order': ['UART_1', 'UART_2', 'UART_6'],
        'uart_ports':    [('UART_1', 'USART1'), ('UART_2', 'USART2'),
                          ('UART_3', 'not on F411, placeholder'),
                          ('UART_4', 'not on F411, placeholder'),
                          ('UART_5', 'not on F411, placeholder'),
                          ('UART_6', 'USART6'), ('UART_7', ''), ('UART_8', '')],
        'iic_ports':     [('IIC_1', ''), ('IIC_2', ''), ('IIC_3', '')],
        'tim_ports':     [('TIMER_1', ''), ('TIMER_2', ''),
                          ('TIMER_3', ''), ('TIMER_4', ''), ('INVALID_TIMER_ID', '')],
    },
    'STM32F407VGTx': {
        'vendor_var':    'MCU_VAR_STM',
        'max_uart':      6,
        'max_iic':       3,
        'max_spi':       3,
        'max_tim':       14,
        'note':          'CAN1/CAN2, No ETH variant',
        'uart_en_order': ['UART_1', 'UART_2', 'UART_3', 'UART_4', 'UART_5', 'UART_6'],
        'uart_ports':    [('UART_1', 'USART1'), ('UART_2', 'USART2'),
                          ('UART_3', 'USART3'), ('UART_4', 'UART4'),
                          ('UART_5', 'UART5'),  ('UART_6', 'USART6'),
                          ('UART_7', ''), ('UART_8', '')],
        'iic_ports':     [('IIC_1', ''), ('IIC_2', ''), ('IIC_3', '')],
        'tim_ports':     [('TIMER_1', ''), ('TIMER_2', ''),
                          ('TIMER_3', ''), ('TIMER_4', ''), ('INVALID_TIMER_ID', '')],
    },
}


class BoardConfigGenerator:

    def __init__(self, xml_path: str):
        self.xml_path   = Path(xml_path)
        tree            = ET.parse(xml_path)
        root            = tree.getroot()

        self.board_name  = root.get('name', 'unknown_board')
        self.vendor      = root.get('vendor', 'STM').upper()
        self.mcu         = root.get('mcu', '')
        self.description = root.get('description', '')

        periph = root.find('peripherals')
        if periph is None:
            periph = root
        self.uarts = periph.findall('uart')
        self.i2cs  = periph.findall('i2c')
        self.spis  = periph.findall('spi')
        self.gpios = periph.findall('gpio')

        # Detect role-assigned UARTs (role="shell", role="debug", …)
        self.uart_roles = {}   # role_name → uart element
        for u in self.uarts:
            role = u.get('role', '').strip().lower()
            if role:
                self.uart_roles[role] = u

        cls     = _VENDOR_MAP.get(self.vendor, STM32Codegen)
        self.cg = cls()

    # ── Public ────────────────────────────────────────────────────────────────

    def generate(self, c_outdir: str, h_outdir: str):
        Path(c_outdir).mkdir(parents=True, exist_ok=True)
        Path(h_outdir).mkdir(parents=True, exist_ok=True)

        c_path   = Path(c_outdir) / 'board_config.c'
        h_path   = Path(h_outdir) / 'board_device_ids.h'
        bh_path  = Path(h_outdir) / 'board_handles.h'
        mcu_path = Path(h_outdir) / 'mcu_config.h'

        c_path.write_text(self._gen_board_config_c())
        h_path.write_text(self._gen_device_ids_h())
        bh_path.write_text(self._gen_board_handles_h())
        mcu_path.write_text(self._gen_mcu_config_h())

        print(f'[gen_board_config] Board   : {self.board_name}')
        print(f'[gen_board_config] Vendor  : {self.vendor}  MCU: {self.mcu}')
        print(f'[gen_board_config] UART    : {len(self.uarts)}   '
              f'I2C: {len(self.i2cs)}   SPI: {len(self.spis)}   GPIO: {len(self.gpios)}')
        print(f'[gen_board_config] -> {c_path}')
        print(f'[gen_board_config] -> {h_path}')
        print(f'[gen_board_config] -> {bh_path}')
        print(f'[gen_board_config] -> {mcu_path}')

    # ── board_device_ids.h ────────────────────────────────────────────────────

    def _gen_device_ids_h(self) -> str:
        guard = f'BOARD_{self.board_name.upper()}_DEVICE_IDS_H_'
        L = []
        L.append(_BANNER.format(xml_path=self.xml_path, date=date.today()))
        L.append(_INC_GUARD_OPEN.format(guard=guard))
        L.append(f'/* Board: {self.board_name}  MCU: {self.mcu} */\n')

        def section(tag, items, label):
            if not items: return
            L.append(f'/* {label} device IDs */')
            for i, el in enumerate(items):
                uid = el.get('id', f'{tag.upper()}_{i}')
                dev = el.get('dev_id', str(i))
                L.append(f'#define {uid:<32} {dev}')
            L.append(f'#define BOARD_{tag.upper()}_COUNT             {len(items)}')
            L.append('')

        section('uart', self.uarts, 'UART')
        section('iic',  self.i2cs,  'I2C')
        section('spi',  self.spis,  'SPI')
        section('gpio', self.gpios, 'GPIO')

        # Emit role-based UART aliases
        if self.uart_roles:
            L.append('/* UART role assignments */')
            for role, u in self.uart_roles.items():
                uid   = u.get('id', '')
                alias = f'BOARD_UART_{role.upper()}_ID'
                L.append(f'#define {alias:<28} {uid:<20} /* role="{role}" */')
            L.append('')

        L.append(_INC_GUARD_CLOSE.format(guard=guard))
        return '\n'.join(L)

    # ── board_handles.h ───────────────────────────────────────────────────────

    def _gen_board_handles_h(self) -> str:
        cg    = self.cg
        guard = f'BOARD_{self.board_name.upper()}_HANDLES_H_'
        L     = []
        L.append(_BANNER.format(xml_path=self.xml_path, date=date.today()))
        L.append(_INC_GUARD_OPEN.format(guard=guard))
        L.append(f'/* Board: {self.board_name}  MCU: {self.mcu} */')
        L.append('')

        periph_groups = [('uart', self.uarts), ('i2c', self.i2cs), ('spi', self.spis)]
        for ptype, items in periph_groups:
            htype = cg.hal_handle_type(ptype)
            if not htype or not items:
                continue
            mod_guard = cg.hal_handle_guard(ptype)
            includes  = cg.hal_handle_includes(ptype)
            if mod_guard:
                L.append(f'#ifdef {mod_guard}')
            for inc in includes:
                L.append(inc)
            for item in items:
                inst  = item.get('instance', '')
                hname = cg.hal_handle_name(inst)
                if hname:
                    L.append(f'extern {htype} {hname};')
            if mod_guard:
                L.append(f'#endif /* {mod_guard} */')
            L.append('')

        L.append(_INC_GUARD_CLOSE.format(guard=guard))
        return '\n'.join(L)

    # ── mcu_config.h ──────────────────────────────────────────────────────────

    def _gen_mcu_config_h(self) -> str:
        guard = 'BOARD_MCU_CONFIG_H_'
        info  = _MCU_PERIPH_MAP.get(self.mcu, {})
        vendor_var    = info.get('vendor_var',    'MCU_VAR_STM')
        max_uart      = info.get('max_uart',      1)
        max_iic       = info.get('max_iic',       0)
        max_spi       = info.get('max_spi',       0)
        max_tim       = info.get('max_tim',       0)
        note          = info.get('note',          '')
        uart_ports    = info.get('uart_ports',    [('UART_1', '')])
        iic_ports     = info.get('iic_ports',     [('IIC_1', '')])
        tim_ports     = info.get('tim_ports',     [('TIMER_1', ''), ('INVALID_TIMER_ID', '')])
        # Build enable map: port_name → minimum NO_OF_UART value (or None = disabled)
        _en_order     = info.get('uart_en_order', [p[0] for p in uart_ports[:max_uart]])
        uart_en_map   = {name: (i + 1) for i, name in enumerate(_en_order)}

        L = []
        L.append(_BANNER.format(xml_path=self.xml_path, date=date.today()))
        L.append(_INC_GUARD_OPEN.format(guard=guard))
        L.append(f'/* Board: {self.board_name}  MCU: {self.mcu} */')
        L.append('')
        L.append('#include "autoconf.h"')
        L.append('')

        L.append('/* ── MCU Vendor / Variant Selection ──────────────────────────────────────── */')
        L.append('#define MCU_VAR_MICROCHIP   1')
        L.append('#define MCU_VAR_STM         2')
        L.append('')
        L.append(f'#define CONFIG_DEVICE_VARIANT    {vendor_var}   /* {self.mcu} */')
        L.append('')

        L.append('/* ── Hardware-fixed peripheral instance counts ───────────────────────────── */')
        L.append(f'#define MCU_MAX_UART_PERIPHERAL     {max_uart}')
        L.append(f'#define MCU_MAX_IIC_PERIPHERAL      {max_iic}')
        L.append(f'#define MCU_MAX_SPI_PERIPHERAL      {max_spi}')
        L.append(f'#define MCU_MAX_TIM_PERIPHERAL      {max_tim}')
        if note:
            L.append(f'/* {note} */')
        L.append('')

        L.append('/* ── Board device IDs — generated from XML by gen_board_config.py ─────────── */')
        L.append('/* Defines BOARD_UART_COUNT, BOARD_IIC_COUNT, BOARD_SPI_COUNT, BOARD_GPIO_COUNT */')
        L.append('#include <board/board_device_ids.h>')
        L.append('')

        L.append('/* ── OS-managed peripheral counts — sourced from board config ────────────── */')
        for macro, board_macro, fallback in [
            ('NO_OF_UART', 'BOARD_UART_COUNT', '1'),
            ('NO_OF_IIC',  'BOARD_IIC_COUNT',  '0'),
            ('NO_OF_SPI',  'BOARD_SPI_COUNT',  '0'),
            ('NO_OF_GPIO', 'BOARD_GPIO_COUNT', '0'),
        ]:
            L.append(f'#ifdef {board_macro}')
            L.append(f'  #define {macro:<12}  {board_macro}')
            L.append(f'#else')
            L.append(f"  #define {macro:<12}  {fallback}   /* fallback — run 'make board-gen' */")
            L.append(f'#endif')
            L.append('')

        L.append('/* ── Peripheral enable flags ─────────────────────────────────────────────── */')
        L.append('#ifdef CONFIG_HAL_IWDG_MODULE_ENABLED')
        L.append('  #define CONFIG_MCU_WDG_EN               (1)')
        L.append('#else')
        L.append('  #define CONFIG_MCU_WDG_EN               (0)')
        L.append('#endif')
        L.append('')
        L.append('#define CONFIG_MCU_FLASH_DRV_EN           (1)   /* Always available */')
        L.append('')
        L.append('#define CONFIG_MCU_NO_OF_UART_PERIPHERAL  NO_OF_UART')
        L.append('#define CONFIG_MCU_NO_OF_IIC_PERIPHERAL   NO_OF_IIC')
        L.append('#define CONFIG_MCU_NO_OF_SPI_PERIPHERAL   NO_OF_SPI')
        L.append('#define CONFIG_MCU_NO_OF_GPIO_PERIPHERAL  NO_OF_GPIO')
        L.append('')
        L.append('#define CONFIG_MCU_NO_OF_RS485_PERIPHERAL (0)')
        L.append('#define CONFIG_MCU_NO_OF_CAN_PERIPHERAL   (0)')
        L.append('#define CONFIG_MCU_NO_OF_USB_PERIPHERAL   (0)')
        L.append('#define CONFIG_MCU_NO_OF_ETH_PERIPHERAL   (0)')
        L.append('')
        L.append('#ifdef CONFIG_HAL_TIM_MODULE_ENABLED')
        L.append(f'  #define CONFIG_MCU_NO_OF_TIMER_PERIPHERAL ({min(max_tim, 2)})')
        L.append('#else')
        L.append('  #define CONFIG_MCU_NO_OF_TIMER_PERIPHERAL (0)')
        L.append('#endif')
        L.append('')
        L.append('#define NO_OF_TIMER   CONFIG_MCU_NO_OF_TIMER_PERIPHERAL')
        L.append('')

        L.append('/* ── Individual UART instance enable — derived from board UART count ─────── */')
        for port_name, _ in uart_ports:
            pos = uart_en_map.get(port_name)
            if pos is not None:
                L.append(f'#define {port_name}_EN    (NO_OF_UART >= {pos} ? 1 : 0)')
            else:
                L.append(f'#define {port_name}_EN    (0)')
        L.append('')

        L.append('/* ── Peripheral identifier enumerations ─────────────────────────────────── */')
        L.append('typedef enum')
        L.append('{')
        for i, (port_name, comment) in enumerate(uart_ports):
            comma = ',' if i < len(uart_ports) - 1 else ''
            suffix = f'  /* {comment} */' if comment else ''
            assign = ' = 0' if i == 0 else ''
            L.append(f'    {port_name}{assign}{comma}{suffix}')
        L.append('} UART_PORTS;')
        L.append('')
        L.append('typedef enum')
        L.append('{')
        for i, (port_name, comment) in enumerate(iic_ports):
            comma = ',' if i < len(iic_ports) - 1 else ''
            suffix = f'  /* {comment} */' if comment else ''
            assign = ' = 0' if i == 0 else ''
            L.append(f'    {port_name}{assign}{comma}{suffix}')
        L.append('} IIC_PORTS;')
        L.append('')
        L.append('typedef enum')
        L.append('{')
        for i, (port_name, comment) in enumerate(tim_ports):
            comma = ',' if i < len(tim_ports) - 1 else ''
            suffix = f'  /* {comment} */' if comment else ''
            L.append(f'    {port_name}{comma}{suffix}')
        L.append('} TIMER_PORTS;')
        L.append('')

        L.append(_INC_GUARD_CLOSE.format(guard=guard))
        return '\n'.join(L)

    # ── board_config.c ────────────────────────────────────────────────────────

    def _gen_board_config_c(self) -> str:
        cg = self.cg
        L  = []

        # ── Header
        L.append(_BANNER.format(xml_path=self.xml_path, date=date.today()))
        # mcu_config.h first — defines CONFIG_DEVICE_VARIANT and MCU_VAR_* used below
        L.append('#include <board/mcu_config.h>')
        L.append('')
        L.append('/* Variant-gated device entry point.')
        L.append(' * arch/devices/device.h selects the correct vendor HAL and device_conf:')
        L.append(' *   MCU_VAR_STM       \u2192 device_conf/stm32f4xx_hal_conf.h + stm32f4xx_hal.h')
        L.append(' *   MCU_VAR_INFINEON  \u2192 TODO: device_conf/xmc_conf.h')
        L.append(' *   MCU_VAR_MICROCHIP \u2192 TODO: device_conf/asf4_conf.h */')
        L.append('#if   (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)')
        L.append('#  include <device.h>')
        L.append('#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_INFINEON)')
        L.append('#  include <device.h>')
        L.append('#elif (CONFIG_DEVICE_VARIANT == MCU_VAR_MICROCHIP)')
        L.append('#  include <device.h>')
        L.append('#else')
        L.append('#  error "board_config.c: unknown CONFIG_DEVICE_VARIANT — update mcu_config.h"')
        L.append('#endif')
        L.append('')
        L.append('#include <board/board_config.h>')
        L.append('#include <board/board_device_ids.h>')
        L.append('')

        # ── CMSIS system variables (STM32 only)
        # SystemCoreClock and the prescaler LUTs must be defined exactly once.
        # They live in board_config.c so every board variant has its own initial
        # value (BOARD_SYSCLK_HZ) and they are placed in .boot_data via the
        # __SECTION_BOOT_DATA attribute so they are valid before .data copy.
        if self.vendor == 'STM':
            L.append('/* ── CMSIS system variables ─────────────────────────────────────────────── */')
            L.append('/* Placed in .boot_data: valid before the .data copy in Reset_Handler.      */')
            L.append('#if (CONFIG_DEVICE_VARIANT == MCU_VAR_STM)')
            L.append('#include <def_attributes.h>')
            L.append('__SECTION_BOOT_DATA uint32_t SystemCoreClock = BOARD_SYSCLK_HZ;')
            L.append('const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};')
            L.append('const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};')
            L.append('#endif /* CONFIG_DEVICE_VARIANT == MCU_VAR_STM */')
            L.append('')

        # ── HAL peripheral handle definitions (storage for extern decls in board_handles.h)
        periph_groups = [('uart', self.uarts), ('i2c', self.i2cs), ('spi', self.spis)]
        has_handles = any(cg.hal_handle_type(pt) and items for pt, items in periph_groups)
        if has_handles:
            L.append('/* ── HAL peripheral handles ─────────────────────────────────────────────── */')
            for ptype, items in periph_groups:
                htype = cg.hal_handle_type(ptype)
                if not htype or not items:
                    continue
                mod_guard = cg.hal_handle_guard(ptype)
                if mod_guard:
                    L.append(f'#ifdef {mod_guard}')
                for item in items:
                    inst  = item.get('instance', '')
                    hname = cg.hal_handle_name(inst)
                    if hname:
                        L.append(f'{htype} {hname};')
                if mod_guard:
                    L.append(f'#endif /* {mod_guard} */')
            L.append('')

        # ── Collect all GPIO ports used (for gpio_clk_enable)
        all_ports = set()
        for u in self.uarts:
            for tag in ['tx', 'rx']:
                e = u.find(tag)
                if e is not None and e.get('port'):
                    all_ports.add(e.get('port').upper())
        for c in self.i2cs:
            for tag in ['scl', 'sda']:
                e = c.find(tag)
                if e is not None and e.get('port'):
                    all_ports.add(e.get('port').upper())
        for s in self.spis:
            for tag in ['sck', 'miso', 'mosi', 'nss_pin']:
                e = s.find(tag)
                if e is not None and e.get('port'):
                    all_ports.add(e.get('port').upper())
        for g in self.gpios:
            if g.get('port'):
                all_ports.add(g.get('port').upper())

        # ── UART table
        if self.uarts:
            L.append('/* ── UART table ─────────────────────────────────────────────────────────── */')
            L.append('#ifdef HAL_UART_MODULE_ENABLED')
            L.append('static const board_uart_desc_t _uart_table[BOARD_UART_COUNT] = {')
            for i, u in enumerate(self.uarts):
                L.extend(self._uart_entry(u, i))
            L.append('};')
            L.append('#define _UART_TABLE  _uart_table')
            L.append('#else')
            L.append('#define _UART_TABLE  NULL')
            L.append('#undef  BOARD_UART_COUNT')
            L.append('#define BOARD_UART_COUNT 0')
            L.append('#endif /* HAL_UART_MODULE_ENABLED */')
            L.append('')

        # ── I2C table
        if self.i2cs:
            L.append('/* ── I2C table ──────────────────────────────────────────────────────────── */')
            L.append('#ifdef HAL_I2C_MODULE_ENABLED')
            L.append('static const board_iic_desc_t _iic_table[BOARD_IIC_COUNT] = {')
            for i, c in enumerate(self.i2cs):
                L.extend(self._iic_entry(c, i))
            L.append('};')
            L.append('#define _IIC_TABLE  _iic_table')
            L.append('#else')
            L.append('#define _IIC_TABLE  NULL')
            L.append('#undef  BOARD_IIC_COUNT')
            L.append('#define BOARD_IIC_COUNT 0')
            L.append('#endif /* HAL_I2C_MODULE_ENABLED */')
            L.append('')

        # ── SPI table
        if self.spis:
            L.append('/* ── SPI table ──────────────────────────────────────────────────────────── */')
            L.append('#ifdef HAL_SPI_MODULE_ENABLED')
            L.append('static const board_spi_desc_t _spi_table[BOARD_SPI_COUNT] = {')
            for i, s in enumerate(self.spis):
                L.extend(self._spi_entry(s, i))
            L.append('};')
            L.append('#define _SPI_TABLE  _spi_table')
            L.append('#else')
            L.append('#define _SPI_TABLE  NULL')
            L.append('#undef  BOARD_SPI_COUNT')
            L.append('#define BOARD_SPI_COUNT 0')
            L.append('#endif /* HAL_SPI_MODULE_ENABLED */')
            L.append('')

        # ── GPIO table
        if self.gpios:
            L.append('/* ── GPIO table ─────────────────────────────────────────────────────────── */')
            L.append('static const board_gpio_desc_t _gpio_table[BOARD_GPIO_COUNT] = {')
            for i, g in enumerate(self.gpios):
                L.extend(self._gpio_entry(g, i))
            L.append('};')
            L.append('')

        # ── Top-level board_config_t
        shell_u   = self.uart_roles.get('shell')
        shell_uid = shell_u.get('id', 'UART_APP') if shell_u else '0'

        L.append('/* ── Top-level board config ─────────────────────────────────────────────── */')
        L.append('static const board_config_t g_board_config = {')
        L.append(f'    .board_name    = "{self.board_name}",')
        L.append(f'    .uart_table    = {"_UART_TABLE" if self.uarts else "NULL"},')
        L.append(f'    .uart_count    = {"BOARD_UART_COUNT" if self.uarts else "0"},')
        if shell_u:
            L.append(f'    .uart_shell_id = {shell_uid},  /* role="shell" */')
        else:
            L.append( '    .uart_shell_id = 0,')
        L.append(f'    .iic_table     = {"_IIC_TABLE" if self.i2cs else "NULL"},')
        L.append(f'    .iic_count     = {"BOARD_IIC_COUNT" if self.i2cs else "0"},')
        L.append(f'    .spi_table     = {"_SPI_TABLE" if self.spis else "NULL"},')
        L.append(f'    .spi_count     = {"BOARD_SPI_COUNT" if self.spis else "0"},')
        L.append(f'    .gpio_table    = {"_gpio_table" if self.gpios else "NULL"},')
        L.append(f'    .gpio_count    = {"BOARD_GPIO_COUNT" if self.gpios else "0"},')
        L.append('};')
        L.append('')

        # ── API
        uart_insts = [u.get('hal_inst','') for u in self.uarts if u.get('hal_inst','')]
        i2c_insts  = [c.get('hal_inst','') for c in self.i2cs  if c.get('hal_inst','')]
        spi_insts  = [s.get('hal_inst','') for s in self.spis  if s.get('hal_inst','')]
        L.extend(self._api_impl(all_ports, uart_insts, i2c_insts, spi_insts))
        return '\n'.join(L) + '\n'

    # ── Pin struct helper ─────────────────────────────────────────────────────

    def _pin_struct(self, el, instance, is_od=False) -> str:
        cg     = self.cg
        port   = el.get('port', 'A')
        pin    = el.get('pin', '0')
        af     = el.get('af', '0')
        speed  = el.get('speed', 'very_high')
        pull   = el.get('pull', 'none')
        return (
            f'            .port      = {cg.gpio_port(port)},\n'
            f'            .pin       = {cg.gpio_pin(pin)},\n'
            f'            .mode      = {cg.gpio_af_mode(is_od)},\n'
            f'            .pull      = {cg.gpio_pull(pull)},\n'
            f'            .speed     = {cg.gpio_speed(speed)},\n'
            f'            .alternate = {cg.af_constant(af, instance)},'
        )

    # ── Per-peripheral entry generators ──────────────────────────────────────

    def _uart_entry(self, u, idx) -> list:
        cg   = self.cg
        inst = u.get('instance', f'USART{idx+1}')
        uid  = u.get('id', f'UART_{idx}')
        irq  = u.find('irq')
        irq_name = irq.get('name', f'{inst}_IRQn') if irq is not None else f'{inst}_IRQn'
        irq_prio = irq.get('priority', '2')        if irq is not None else '2'
        tx = u.find('tx')
        if tx is None:
            tx = u.find('TX')
        rx = u.find('rx')
        if rx is None:
            rx = u.find('RX')
        L = [
            f'    /* {uid} → {inst} */',
            f'    [{idx}] = {{',
            f'        .dev_id       = {u.get("dev_id", str(idx))},',
            f'        .instance     = {inst},',
            f'        .baudrate     = {u.get("baudrate","115200")},',
            f'        .word_len     = {cg.uart_wordlen(u.get("word_len","8"))},',
            f'        .stop_bits    = {cg.uart_stopbits(u.get("stop_bits","1"))},',
            f'        .parity       = {cg.uart_parity(u.get("parity","none"))},',
            f'        .mode         = {cg.uart_mode(u.get("mode","tx_rx"))},',
            f'        .tx_pin       = {{',
        ]
        if tx is not None:
            L.append(self._pin_struct(tx, inst))
        L += [f'        }},', f'        .rx_pin       = {{']
        if rx is not None:
            L.append(self._pin_struct(rx, inst))
        L += [
            f'        }},',
            f'        .irqn         = {irq_name},',
            f'        .irq_priority = {irq_prio},',
            f'    }},',
        ]
        return L

    def _iic_entry(self, c, idx) -> list:
        cg   = self.cg
        inst = c.get('instance', f'I2C{idx+1}')
        cid  = c.get('id', f'I2C_{idx}')
        ev   = c.find('irq_ev')
        er   = c.find('irq_er')
        ev_name = ev.get('name', f'{inst}_EV_IRQn') if ev is not None else f'{inst}_EV_IRQn'
        er_name = er.get('name', f'{inst}_ER_IRQn') if er is not None else f'{inst}_ER_IRQn'
        prio    = ev.get('priority', '3') if ev is not None else '3'
        scl = c.find('scl')
        sda = c.find('sda')
        L = [
            f'    /* {cid} → {inst} */',
            f'    [{idx}] = {{',
            f'        .dev_id       = {c.get("dev_id", str(idx))},',
            f'        .instance     = {inst},',
            f'        .clock_hz     = {c.get("clock_hz","400000")},',
            f'        .addr_mode    = {cg.i2c_addr_mode(c.get("addr_mode","7bit"))},',
            f'        .dual_addr    = {cg.i2c_dual_addr(c.get("dual_addr","disable"))},',
            f'        .scl_pin      = {{',
        ]
        if scl is not None:
            L.append(self._pin_struct(scl, inst, is_od=True))
        L += ['        },', '        .sda_pin      = {']
        if sda is not None:
            L.append(self._pin_struct(sda, inst, is_od=True))
        L += [
            '        },',
            f'        .ev_irqn      = {ev_name},',
            f'        .er_irqn      = {er_name},',
            f'        .irq_priority = {prio},',
            '    },',
        ]
        return L

    def _spi_entry(self, s, idx) -> list:
        cg   = self.cg
        inst = s.get('instance', f'SPI{idx+1}')
        sid  = s.get('id', f'SPI_{idx}')
        irq  = s.find('irq')
        irq_name = irq.get('name', f'{inst}_IRQn') if irq is not None else f'{inst}_IRQn'
        irq_prio = irq.get('priority', '3')        if irq is not None else '3'
        sck  = s.find('sck')
        miso = s.find('miso')
        mosi = s.find('mosi')
        nss  = s.find('nss_pin')
        L = [
            f'    /* {sid} → {inst} */',
            f'    [{idx}] = {{',
            f'        .dev_id         = {s.get("dev_id", str(idx))},',
            f'        .instance       = {inst},',
            f'        .mode           = {cg.spi_mode(s.get("mode","master"))},',
            f'        .direction      = {cg.spi_direction(s.get("direction","2lines"))},',
            f'        .data_size      = {cg.spi_datasize(s.get("data_size","8"))},',
            f'        .clk_polarity   = {cg.spi_polarity(s.get("clk_polarity","low"))},',
            f'        .clk_phase      = {cg.spi_phase(s.get("clk_phase","1edge"))},',
            f'        .nss            = {cg.spi_nss(s.get("nss","soft"))},',
            f'        .baud_prescaler = {cg.spi_prescaler(s.get("baud_prescaler","16"))},',
            f'        .bit_order      = {cg.spi_bitorder(s.get("bit_order","msb"))},',
            f'        .sck_pin        = {{',
        ]
        if sck  is not None: L.append(self._pin_struct(sck,  inst))
        L += ['        },', '        .miso_pin       = {']
        if miso is not None: L.append(self._pin_struct(miso, inst))
        L += ['        },', '        .mosi_pin       = {']
        if mosi is not None: L.append(self._pin_struct(mosi, inst))
        L.append('        },')
        if nss is not None:
            L.append('        .nss_pin        = {')
            L.append(self._pin_struct(nss, inst))
            L.append('        },')
        else:
            L.append('        .nss_pin        = { .pin = 0 },  /* software NSS */')
        L += [
            f'        .irqn           = {irq_name},',
            f'        .irq_priority   = {irq_prio},',
            '    },',
        ]
        return L

    def _gpio_entry(self, g, idx) -> list:
        cg    = self.cg
        gid   = g.get('id', f'GPIO_{idx}')
        mode  = g.get('mode', 'output_pp')
        act   = cg.gpio_active_state(g.get('active_state', 'high'))
        init  = '1' if g.get('initial_state','inactive').lower() == 'active' else '0'

        if mode == 'input':
            mode_ref = cg.gpio_in_mode()
        elif mode.startswith('it_'):
            mode_ref = cg.gpio_it_mode(mode[3:])
        elif mode == 'output_od':
            mode_ref = cg.gpio_out_mode(is_od=True)
        else:
            mode_ref = cg.gpio_out_mode(is_od=False)

        return [
            f'    /* {gid} */',
            f'    [{idx}] = {{',
            f'        .dev_id        = {g.get("dev_id", str(idx))},',
            f'        .label         = "{gid}",',
            f'        .port          = {cg.gpio_port(g.get("port","A"))},',
            f'        .pin           = {cg.gpio_pin(g.get("pin","0"))},',
            f'        .mode          = {mode_ref},',
            f'        .pull          = {cg.gpio_pull(g.get("pull","none"))},',
            f'        .speed         = {cg.gpio_speed(g.get("speed","low"))},',
            f'        .active_state  = {act},',
            f'        .initial_state = {init},',
            f'    }},',
        ]

    # ── API implementation ────────────────────────────────────────────────────

    def _api_impl(self, all_ports, uart_insts, i2c_insts, spi_insts) -> list:
        cg = self.cg
        L  = [
            '/* ═══════════════════════════════════════════════════════════════════════════',
            ' * Board API implementation',
            ' * ═══════════════════════════════════════════════════════════════════════════ */',
            '',
            'const board_config_t *board_get_config(void)    { return &g_board_config; }',
            'uint8_t board_get_shell_uart_id(void)           { return g_board_config.uart_shell_id; }',
            '',
            'const board_uart_desc_t *board_find_uart(USART_TypeDef *instance)',
            '{',
            '    for (uint8_t i = 0; i < g_board_config.uart_count; i++)',
            '        if (g_board_config.uart_table && g_board_config.uart_table[i].instance == instance)',
            '            return &g_board_config.uart_table[i];',
            '    return NULL;',
            '}',
            '',
            'const board_iic_desc_t *board_find_iic(I2C_TypeDef *instance)',
            '{',
            '    for (uint8_t i = 0; i < g_board_config.iic_count; i++)',
            '        if (g_board_config.iic_table && g_board_config.iic_table[i].instance == instance)',
            '            return &g_board_config.iic_table[i];',
            '    return NULL;',
            '}',
            '',
            'const board_spi_desc_t *board_find_spi(SPI_TypeDef *instance)',
            '{',
            '    for (uint8_t i = 0; i < g_board_config.spi_count; i++)',
            '        if (g_board_config.spi_table && g_board_config.spi_table[i].instance == instance)',
            '            return &g_board_config.spi_table[i];',
            '    return NULL;',
            '}',
            '',
            'void board_clk_enable(void)',
            '{',
            cg.all_clk_enable_body(all_ports, uart_insts, i2c_insts, spi_insts),
            '}',
            '',
        ]
        return L


# ──────────────────────────────────────────────────────────────────────────────
#  Entry point
# ──────────────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description='Generate board_config.c and board_device_ids.h from a board XML file.'
    )
    parser.add_argument('xml',
        help='Path to the board XML file (e.g. boards/stm32f411_devboard.xml)')
    parser.add_argument('--outdir', '-o',
        help='Directory for board_config.c  '
             '(default: boards/<board_name>/)')
    parser.add_argument('--incdir', '-i',
        help='Directory for board_device_ids.h  '
             '(default: include/board/)')
    args = parser.parse_args()

    xml_path = Path(args.xml)
    if not xml_path.exists():
        print(f'Error: XML file not found: {xml_path}', file=sys.stderr)
        sys.exit(1)

    gen = BoardConfigGenerator(str(xml_path))

    # Resolve output directories relative to project root (parent of scripts/)
    script_dir  = Path(__file__).parent
    project_root = script_dir.parent

    c_outdir = Path(args.outdir)  if args.outdir  else project_root / 'boards' / gen.board_name
    h_outdir = Path(args.incdir)  if args.incdir  else project_root / 'include' / 'board'

    gen.generate(str(c_outdir), str(h_outdir))


if __name__ == '__main__':
    main()
