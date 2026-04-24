#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
#  gen_intellisense.sh  —  Generate c_cpp_properties.json for both workspaces
#
#  Usage:
#    ./scripts/gen_intellisense.sh            # generate for all variants
#    ./scripts/gen_intellisense.sh STM32F411xE # generate for one variant only
#
#  Called automatically by: make board-gen APP_DIR=../app
#
#  Output files (always overwritten — do not edit them by hand):
#    <repo-root>/.vscode/c_cpp_properties.json            (app root workspace)
#    <repo-root>/FreeRTOS-OS/.vscode/c_cpp_properties.json (OS sub-workspace)
#
#  To add a variant: append an entry to the VARIANTS array below.
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

# Script lives at FreeRTOS-OS/scripts/ — derive paths from there
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OS_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"      # FreeRTOS-OS/
ROOT_DIR="$(cd "$OS_DIR/.." && pwd)"         # FreeRTOS-OS-App/ (app root)

FILTER="${1:-}"   # optional variant filter (e.g. stm32f411)

# ─────────────────────────────────────────────────────────────────────────────
#  VARIANT TABLE  —  add new targets here
#
#  Format: "name|mcu_define|device_subpath|freertos_port"
#
#    name         VS Code configuration name shown in the status bar
#    mcu_define   MCU preprocessor define (selects CMSIS device header)
#    device_subpath  path under arch/devices/STM/STM32F4xx/ to device dir
#    freertos_port   FreeRTOS portable/ subdirectory (GCC/ARM_CM4F etc.)
# ─────────────────────────────────────────────────────────────────────────────
VARIANTS=(
    "STM32F411xE|STM32F411xE|STM32F411|GCC/ARM_CM4F"
    "STM32F401xE|STM32F401xE|STM32F401|GCC/ARM_CM4F"
)

# ─────────────────────────────────────────────────────────────────────────────
#  CONSTANT INCLUDES  —  paths that are the same for every variant
#  (relative to the OS root — prefix is added per workspace below)
# ─────────────────────────────────────────────────────────────────────────────
CONST_OS_INCLUDES=(
    ""                                          # OS root (bare <header.h> resolution)
    "include"
    "include/drivers"
    "include/drivers/hal/stm32"
    "include/drv_app"
    "include/drv_ext_chips"
    "include/init"
    "include/ipc"
    "include/irq"
    "include/lib"
    "include/mm"
    "include/os"
    "include/services"
    "include/shell"
    "config"
    "lib"
    "arch/arm/CMSIS_6"
    "arch/devices"
    "arch/devices/device_conf"
    "arch/devices/STM/stm32f4xx-hal-driver/Inc"
    "kernel"
    "kernel/FreeRTOS-Kernel/include"
    "kernel/FreeRTOS-Plus-CLI"
)

# App paths (relative to app/ directory)
CONST_APP_INCLUDES=(
    ""          # app root
    "board"
)

# ─────────────────────────────────────────────────────────────────────────────
#  CONSTANT DEFINES  —  same for every STM32 variant
# ─────────────────────────────────────────────────────────────────────────────
CONST_DEFINES=(
    "USE_HAL_DRIVER"
    "__ARM_ARCH_7EM__"
    "__FPU_PRESENT=1"
    "ARM_MATH_CM4"
    "CONFIG_DEVICE_VARIANT=2"
    "MCU_VAR_STM=2"
    "HAL_MODULE_ENABLED"
    "HAL_CORTEX_MODULE_ENABLED"
    "HAL_GPIO_MODULE_ENABLED"
    "HAL_EXTI_MODULE_ENABLED"
    "HAL_UART_MODULE_ENABLED"
    "HAL_I2C_MODULE_ENABLED"
    "HAL_SPI_MODULE_ENABLED"
    "HAL_TIM_MODULE_ENABLED"
    "HAL_RCC_MODULE_ENABLED"
    "HAL_DMA_MODULE_ENABLED"
    "HAL_FLASH_MODULE_ENABLED"
    "HAL_PWR_MODULE_ENABLED"
)

# ─────────────────────────────────────────────────────────────────────────────
#  JSON helpers
# ─────────────────────────────────────────────────────────────────────────────

# Emit a JSON array from bash array; caller passes indent string
json_string_array() {
    local indent="$1"; shift
    local arr=("$@")
    local count=${#arr[@]}
    local i=0
    printf "[\n"
    for item in "${arr[@]}"; do
        i=$((i+1))
        if [[ $i -lt $count ]]; then
            printf '%s    "%s",\n' "$indent" "$item"
        else
            printf '%s    "%s"\n' "$indent" "$item"
        fi
    done
    printf '%s]' "$indent"
}

# ─────────────────────────────────────────────────────────────────────────────
#  Build the includePath array for one configuration
#  $1 = os_prefix  (e.g. "${workspaceFolder}/FreeRTOS-OS" or "${workspaceFolder}")
#  $2 = app_prefix (e.g. "${workspaceFolder}/app"         or "${workspaceFolder}/../app")
#  $3 = device_subpath  (e.g. "STM32F411")
#  $4 = freertos_port   (e.g. "GCC/ARM_CM4F")
# ─────────────────────────────────────────────────────────────────────────────
build_includes() {
    local os_pfx="$1"
    local app_pfx="$2"
    local dev_sub="$3"
    local port="$4"
    local indent="                "
    local paths=()

    # ── Constant OS includes ──────────────────────────────────────────────
    for p in "${CONST_OS_INCLUDES[@]}"; do
        if [[ -z "$p" ]]; then
            paths+=("${os_pfx}")
        else
            paths+=("${os_pfx}/${p}")
        fi
    done

    # ── FreeRTOS portable (variant-specific) ─────────────────────────────
    paths+=("${os_pfx}/kernel/FreeRTOS-Kernel/portable/${port}")

    # ── Device include (variant-specific) ────────────────────────────────
    paths+=("${os_pfx}/arch/devices/STM/STM32F4xx")
    paths+=("${os_pfx}/arch/devices/STM/STM32F4xx/${dev_sub}")

    # ── App includes ─────────────────────────────────────────────────────
    for p in "${CONST_APP_INCLUDES[@]}"; do
        if [[ -z "$p" ]]; then
            paths+=("${app_pfx}")
        else
            paths+=("${app_pfx}/${p}")
        fi
    done

    json_string_array "$indent" "${paths[@]}"
}

# ─────────────────────────────────────────────────────────────────────────────
#  Build the defines array for one configuration
#  $1 = mcu_define  (e.g. "STM32F411xE")
# ─────────────────────────────────────────────────────────────────────────────
build_defines() {
    local mcu="$1"
    local indent="                "
    local defs=("$mcu" "${CONST_DEFINES[@]}")
    json_string_array "$indent" "${defs[@]}"
}

# ─────────────────────────────────────────────────────────────────────────────
#  Emit one named configuration block
# ─────────────────────────────────────────────────────────────────────────────
emit_config() {
    local name="$1"
    local os_pfx="$2"
    local app_pfx="$3"
    local mcu="$4"
    local dev_sub="$5"
    local port="$6"
    local last="$7"   # "1" = last entry (no trailing comma)

    local inc
    inc=$(build_includes "$os_pfx" "$app_pfx" "$dev_sub" "$port")
    local def
    def=$(build_defines "$mcu")

    printf '        {\n'
    printf '            "name": "%s",\n' "$name"
    printf '            "includePath": %s,\n' "$inc"
    printf '            "defines": %s,\n' "$def"
    printf '            "compilerPath": "/usr/bin/arm-none-eabi-gcc",\n'
    printf '            "compilerArgs": ["-mcpu=cortex-m4", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard", "-mthumb"],\n'
    printf '            "cStandard": "gnu99",\n'
    printf '            "cppStandard": "gnu++14",\n'
    printf '            "intelliSenseMode": "gcc-arm",\n'
    printf '            "forcedInclude": [\n'
    printf '                "%s/config/autoconf.h"\n' "$os_pfx"
    printf '            ]\n'
    if [[ "$last" == "1" ]]; then
        printf '        }\n'
    else
        printf '        },\n'
    fi
}

# ─────────────────────────────────────────────────────────────────────────────
#  Build all matching variant configs for one workspace
#  $1 = os_prefix  $2 = app_prefix
# ─────────────────────────────────────────────────────────────────────────────
emit_all_configs() {
    local os_pfx="$1"
    local app_pfx="$2"

    # Apply variant filter if requested
    local active=()
    for v in "${VARIANTS[@]}"; do
        local vname; vname=$(cut -d'|' -f1 <<< "$v")
        if [[ -z "$FILTER" ]] || [[ "${vname,,}" == *"${FILTER,,}"* ]]; then
            active+=("$v")
        fi
    done

    local total=${#active[@]}
    local i=0
    for v in "${active[@]}"; do
        i=$((i+1))
        IFS='|' read -r vname mcu dev_sub port <<< "$v"
        local last="0"; [[ $i -eq $total ]] && last="1"
        emit_config "$vname" "$os_pfx" "$app_pfx" "$mcu" "$dev_sub" "$port" "$last"
    done
}

# ─────────────────────────────────────────────────────────────────────────────
#  Write c_cpp_properties.json
#  $1 = output path  $2 = os_prefix  $3 = app_prefix
# ─────────────────────────────────────────────────────────────────────────────
write_properties() {
    local outfile="$1"
    local os_pfx="$2"
    local app_pfx="$3"

    {
        printf '// c_cpp_properties.json\n'
        printf '// GENERATED by scripts/gen_intellisense.sh — DO NOT EDIT MANUALLY\n'
        printf '// To update: edit gen_intellisense.sh then re-run the script.\n'
        printf '// Constant paths are in c_cpp_base.jsonc (reference only).\n'
        printf '{\n'
        printf '    "configurations": [\n'
        emit_all_configs "$os_pfx" "$app_pfx"
        printf '    ],\n'
        printf '    "version": 4\n'
        printf '}\n'
    } > "$outfile"

    echo "  wrote: $outfile"
}

# ─────────────────────────────────────────────────────────────────────────────
#  Main
# ─────────────────────────────────────────────────────────────────────────────
echo "gen_intellisense.sh — generating IntelliSense configs"
echo "  filter: ${FILTER:-<all variants>}"
echo ""

# App root workspace  (.../FreeRTOS-OS-App/)
write_properties \
    "$ROOT_DIR/.vscode/c_cpp_properties.json" \
    '${workspaceFolder}/FreeRTOS-OS' \
    '${workspaceFolder}/app'

# OS sub-workspace  (.../FreeRTOS-OS-App/FreeRTOS-OS/)
write_properties \
    "$OS_DIR/.vscode/c_cpp_properties.json" \
    '${workspaceFolder}' \
    '${workspaceFolder}/../app'

echo ""
echo "Done. Reload VS Code window (Ctrl+Shift+P → Reload Window) to apply."
