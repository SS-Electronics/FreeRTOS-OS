#!/usr/bin/env python3
"""
gen_stm32_hal_conf.py — Generate stm32f4xx_hal_conf.h or stm32h7xx_hal_conf.h
                         from the values in autoconf.h.

The output family (F4 or H7) is auto-detected from CONFIG_TARGET_MCU in
autoconf.h ("STM32H7…" → H7, everything else → F4).

Usage:
    python3 scripts/gen_stm32_hal_conf.py <autoconf_h> <output_path>

Called automatically by:
    make config-outputs
"""

import sys
import os
import re

# ── HAL modules: (short_name, include_file) ──────────────────────────────────
# Order controls the #include sequence, which matters for STM32 HAL.

_HAL_MODULES_F4 = [
    ("RCC",        "stm32f4xx_hal_rcc.h"),
    ("GPIO",       "stm32f4xx_hal_gpio.h"),
    ("EXTI",       "stm32f4xx_hal_exti.h"),
    ("DMA",        "stm32f4xx_hal_dma.h"),
    ("CORTEX",     "stm32f4xx_hal_cortex.h"),
    ("ADC",        "stm32f4xx_hal_adc.h"),
    ("CRC",        "stm32f4xx_hal_crc.h"),
    ("FLASH",      "stm32f4xx_hal_flash.h"),
    ("I2C",        "stm32f4xx_hal_i2c.h"),
    ("SMBUS",      "stm32f4xx_hal_smbus.h"),
    ("I2S",        "stm32f4xx_hal_i2s.h"),
    ("IWDG",       "stm32f4xx_hal_iwdg.h"),
    ("PWR",        "stm32f4xx_hal_pwr.h"),
    ("RNG",        "stm32f4xx_hal_rng.h"),
    ("RTC",        "stm32f4xx_hal_rtc.h"),
    ("SD",         "stm32f4xx_hal_sd.h"),
    ("SPI",        "stm32f4xx_hal_spi.h"),
    ("TIM",        "stm32f4xx_hal_tim.h"),
    ("UART",       "stm32f4xx_hal_uart.h"),
    ("USART",      "stm32f4xx_hal_usart.h"),
    ("IRDA",       "stm32f4xx_hal_irda.h"),
    ("SMARTCARD",  "stm32f4xx_hal_smartcard.h"),
    ("WWDG",       "stm32f4xx_hal_wwdg.h"),
    ("PCD",        "stm32f4xx_hal_pcd.h"),
    ("HCD",        "stm32f4xx_hal_hcd.h"),
]

_HAL_MODULES_H7 = [
    ("RCC",        "stm32h7xx_hal_rcc.h"),
    ("GPIO",       "stm32h7xx_hal_gpio.h"),
    ("EXTI",       "stm32h7xx_hal_exti.h"),
    ("DMA",        "stm32h7xx_hal_dma.h"),
    ("MDMA",       "stm32h7xx_hal_mdma.h"),
    ("CORTEX",     "stm32h7xx_hal_cortex.h"),
    ("ADC",        "stm32h7xx_hal_adc.h"),
    ("DAC",        "stm32h7xx_hal_dac.h"),
    ("CRC",        "stm32h7xx_hal_crc.h"),
    ("FLASH",      "stm32h7xx_hal_flash.h"),
    ("FDCAN",      "stm32h7xx_hal_fdcan.h"),
    ("I2C",        "stm32h7xx_hal_i2c.h"),
    ("SMBUS",      "stm32h7xx_hal_smbus.h"),
    ("I2S",        "stm32h7xx_hal_i2s.h"),
    ("IWDG",       "stm32h7xx_hal_iwdg.h"),
    ("LPTIM",      "stm32h7xx_hal_lptim.h"),
    ("PWR",        "stm32h7xx_hal_pwr.h"),
    ("RNG",        "stm32h7xx_hal_rng.h"),
    ("RTC",        "stm32h7xx_hal_rtc.h"),
    ("SDMMC",      "stm32h7xx_hal_sdmmc.h"),
    ("SPI",        "stm32h7xx_hal_spi.h"),
    ("TIM",        "stm32h7xx_hal_tim.h"),
    ("UART",       "stm32h7xx_hal_uart.h"),
    ("USART",      "stm32h7xx_hal_usart.h"),
    ("IRDA",       "stm32h7xx_hal_irda.h"),
    ("SMARTCARD",  "stm32h7xx_hal_smartcard.h"),
    ("WWDG",       "stm32h7xx_hal_wwdg.h"),
    ("ETH",        "stm32h7xx_hal_eth.h"),
    ("CRYP",       "stm32h7xx_hal_cryp.h"),
    ("HASH",       "stm32h7xx_hal_hash.h"),
    ("OSPI",       "stm32h7xx_hal_ospi.h"),
    ("PCD",        "stm32h7xx_hal_pcd.h"),
    ("HCD",        "stm32h7xx_hal_hcd.h"),
]

_MODULE_GROUPS_F4 = [
    ("Core",             ["CORTEX", "PWR", "RCC", "GPIO", "EXTI",
                          "FLASH", "DMA", "IWDG", "WWDG", "CRC"]),
    ("Communication",    ["UART", "USART", "SPI", "I2C", "I2S",
                          "IRDA", "SMARTCARD", "SMBUS"]),
    ("Timer",            ["TIM"]),
    ("Analog",           ["ADC"]),
    ("USB",              ["PCD", "HCD"]),
    ("Storage",          ["SD"]),
    ("RTC & Security",   ["RTC", "RNG"]),
]

_MODULE_GROUPS_H7 = [
    ("Core",             ["CORTEX", "PWR", "RCC", "GPIO", "EXTI",
                          "FLASH", "DMA", "MDMA", "IWDG", "WWDG", "CRC"]),
    ("Communication",    ["UART", "USART", "SPI", "I2C", "I2S", "FDCAN",
                          "IRDA", "SMARTCARD", "SMBUS", "ETH"]),
    ("Timer",            ["TIM", "LPTIM"]),
    ("Analog",           ["ADC", "DAC"]),
    ("USB",              ["PCD", "HCD"]),
    ("Storage",          ["SDMMC", "OSPI"]),
    ("RTC & Security",   ["RTC", "RNG", "CRYP", "HASH"]),
]

# ── Register-callback modules ──────────────────────────────────────────────────
_CALLBACK_MODULES_F4 = [
    "ADC", "CAN", "CEC", "CRYP", "DAC", "DCMI", "DFSDM", "DMA2D", "DSI",
    "ETH", "HASH", "HCD", "I2C", "FMPI2C", "FMPSMBUS", "I2S", "IRDA",
    "LPTIM", "LTDC", "MMC", "NAND", "NOR", "PCCARD", "PCD", "QSPI", "RNG",
    "RTC", "SAI", "SD", "SMARTCARD", "SDRAM", "SRAM", "SPDIFRX", "SMBUS",
    "SPI", "TIM", "UART", "USART", "WWDG",
]

_CALLBACK_MODULES_H7 = [
    "ADC", "CRYP", "DAC", "DCMI", "DFSDM", "DMA", "DMA2D", "DSI",
    "ETH", "FDCAN", "HASH", "HCD", "I2C", "I2S", "IRDA",
    "JPEG", "LPTIM", "LTDC", "MDMA", "MMC", "NAND", "NOR", "OSPI",
    "PCD", "RNG", "RTC", "SAI", "SD", "SMARTCARD", "SDRAM", "SRAM",
    "SMBUS", "SPDIFRX", "SPI", "TIM", "UART", "USART", "WWDG",
]

# ── Clock / scalar config items (common to F4 and H7) ────────────────────────
CLOCK_ITEMS = [
    ("CONFIG_HSE_VALUE",           "HSE_VALUE"),
    ("CONFIG_HSE_STARTUP_TIMEOUT", "HSE_STARTUP_TIMEOUT"),
    ("CONFIG_HSI_VALUE",           "HSI_VALUE"),
    ("CONFIG_LSI_VALUE",           "LSI_VALUE"),
    ("CONFIG_LSE_VALUE",           "LSE_VALUE"),
    ("CONFIG_LSE_STARTUP_TIMEOUT", "LSE_STARTUP_TIMEOUT"),
    ("CONFIG_EXTERNAL_CLOCK_VALUE","EXTERNAL_CLOCK_VALUE"),
]

# H7-specific clock items not present on F4
CLOCK_ITEMS_H7 = [
    ("CONFIG_CSI_VALUE", "CSI_VALUE"),
]

# Default CSI value for H7 when not set in Kconfig (hardware constant: 4 MHz)
_H7_CSI_DEFAULT = 4000000


def _detect_family(cfg: dict) -> str:
    """Return 'H7' when CONFIG_TARGET_MCU starts with STM32H7, else 'F4'."""
    mcu = cfg.get("CONFIG_TARGET_MCU", "")
    return "H7" if mcu.upper().startswith("STM32H7") else "F4"


def parse_autoconf(path):
    """Parse include/config/autoconf.h and return a dict of CONFIG_* values.

    All numeric values (including 0 and 1) are stored as int.
    String values are stored without surrounding quotes.
    """
    cfg = {}
    if not os.path.isfile(path):
        return cfg
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            m = re.match(r'^#define\s+(CONFIG_\w+)\s+(.+)$', line)
            if m:
                key = m.group(1)
                raw = m.group(2).split("/*")[0].strip()
                if raw.startswith('"') and raw.endswith('"'):
                    cfg[key] = raw[1:-1]
                else:
                    try:
                        cfg[key] = int(raw)
                    except ValueError:
                        cfg[key] = raw
    return cfg


def section(title):
    bar = "#" * 74
    return [
        "",
        f"/* {bar}",
        f" * {title}",
        f" * {bar} */",
        "",
    ]


def generate(cfg, out_path):
    family = _detect_family(cfg)

    if family == "H7":
        hal_modules    = _HAL_MODULES_H7
        module_groups  = _MODULE_GROUPS_H7
        cb_modules     = _CALLBACK_MODULES_H7
        guard          = "__STM32H7xx_HAL_CONF_H"
        fname          = "stm32h7xx_hal_conf.h"
    else:
        hal_modules    = _HAL_MODULES_F4
        module_groups  = _MODULE_GROUPS_F4
        cb_modules     = _CALLBACK_MODULES_F4
        guard          = "__STM32F4xx_HAL_CONF_H"
        fname          = "stm32f4xx_hal_conf.h"

    L = []

    # ── File header ──────────────────────────────────────────────────────────
    L += [
        "/**",
        " ******************************************************************************",
        f" * @file    {fname}",
        " * @brief   HAL configuration — generated from menuconfig (.config)",
        " *",
        " * AUTO-GENERATED by scripts/gen_stm32_hal_conf.py",
        " * DO NOT hand-edit module enables or clock values here.",
        " * Run  make menuconfig      to change settings interactively.",
        " * Run  make config-outputs  to regenerate this file.",
        " * Then rebuild with  make all.",
        " *",
        " * Original template: STMicroelectronics MCD Application Team",
        " * Adapted for FreeRTOS-OS Kconfig integration: Subhajit Roy",
        " ******************************************************************************",
        " */",
        "",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "",
        '/* Pull in every CONFIG_* symbol produced by "make config-outputs" */',
        '#include "autoconf.h"',
        "",
    ]

    # ── HAL Module enables ────────────────────────────────────────────────────
    L += section("HAL Module Selection  —  generated from menuconfig")
    L.append("#define HAL_MODULE_ENABLED   /* master HAL enable — always on */")
    L.append("")

    for group_name, mods in module_groups:
        enabled = [m for m in mods
                   if cfg.get(f"CONFIG_HAL_{m}_MODULE_ENABLED") == 1]
        if enabled:
            L.append(f"/* {group_name} */")
            for mod in enabled:
                L.append(f"#define HAL_{mod}_MODULE_ENABLED")
            L.append("")

    # ── Clock values ──────────────────────────────────────────────────────────
    L += section("Clock values  —  baked in from menuconfig integers")

    for cfg_key, macro in CLOCK_ITEMS:
        val = cfg.get(cfg_key)
        if val is not None:
            L += [
                f"#if !defined({macro})",
                f"  #define {macro:<28} ((uint32_t){val}U)",
                f"#endif",
                "",
            ]

    if family == "H7":
        for cfg_key, macro in CLOCK_ITEMS_H7:
            val = cfg.get(cfg_key, _H7_CSI_DEFAULT)
            L += [
                f"#if !defined({macro})",
                f"  #define {macro:<28} ((uint32_t){val}U)",
                f"#endif",
                "",
            ]

    # ── System / board config ─────────────────────────────────────────────────
    L += section("System / board configuration")

    vdd       = cfg.get("CONFIG_VDD_VALUE", 3300)
    tick_pri  = cfg.get("CONFIG_TICK_INT_PRIORITY", 1)
    prefetch  = 1 if cfg.get("CONFIG_PREFETCH_ENABLE") == 1 else 0
    icache    = 1 if cfg.get("CONFIG_INSTRUCTION_CACHE_ENABLE") == 1 else 0
    dcache    = 1 if cfg.get("CONFIG_DATA_CACHE_ENABLE") == 1 else 0

    L += [
        f"#define VDD_VALUE                     ((uint32_t){vdd}U)",
        f"#define TICK_INT_PRIORITY             ((uint32_t){tick_pri}U)",
        "#define USE_RTOS                      0U   /* Must stay 0 — STM32 HAL rejects 1 */",
        f"#define PREFETCH_ENABLE               {prefetch}U",
        f"#define INSTRUCTION_CACHE_ENABLE      {icache}U",
        f"#define DATA_CACHE_ENABLE             {dcache}U",
    ]

    # ── Register-callback switches ────────────────────────────────────────────
    L += section("Register-callback switches  —  all off by default.\n"
                 " * Enable per-peripheral if you need runtime-swappable callbacks.")

    max_len = max(len(f"USE_HAL_{m}_REGISTER_CALLBACKS") for m in cb_modules)
    for mod in cb_modules:
        name = f"USE_HAL_{mod}_REGISTER_CALLBACKS"
        L.append(f"#define {name:<{max_len}}   0U")

    # ── assert_param ──────────────────────────────────────────────────────────
    L += section("assert_param")

    if cfg.get("CONFIG_USE_FULL_ASSERT") == 1:
        L.append("#define USE_FULL_ASSERT    1U")
        L.append("")

    L += [
        "#ifdef USE_FULL_ASSERT",
        "  #define assert_param(expr) \\",
        "      ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))",
        "  void assert_failed(uint8_t *file, uint32_t line);",
        "#else",
        "  #define assert_param(expr) ((void)0U)",
        "#endif",
    ]

    # ── SPI CRC ───────────────────────────────────────────────────────────────
    L += section("SPI CRC feature")
    L.append("#define USE_SPI_CRC    0U")

    # ── Conditional HAL includes (only enabled modules) ───────────────────────
    L += section("HAL driver includes  —  only for modules enabled in menuconfig")

    for mod, inc in hal_modules:
        if cfg.get(f"CONFIG_HAL_{mod}_MODULE_ENABLED") == 1:
            L.append(f'#include "{inc}"')

    # ── Footer ────────────────────────────────────────────────────────────────
    L += [
        "",
        "",
        '#include "device.h"',
        "",
        "#ifdef __cplusplus",
        "}",
        "#endif",
        "",
        f"#endif /* {guard} */",
        "",
    ]

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(L))


def main():
    if len(sys.argv) < 3:
        print("Usage: gen_stm32_hal_conf.py <autoconf_h> <output_path>",
              file=sys.stderr)
        sys.exit(1)

    cfg = parse_autoconf(sys.argv[1])
    generate(cfg, sys.argv[2])
    print(f"### {sys.argv[2]} written ({len(cfg)} CONFIG_* keys read)")


if __name__ == "__main__":
    main()
