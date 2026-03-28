#!/bin/bash
# run_openocd_server.sh
#
# Starts an OpenOCD GDB server for the STM32F411VET6 target.
# Run this in a terminal, then connect with GDB or the VSCode
# "Attach" launch configuration.
#
# Usage:
#   bash scripts/run_openocd_server.sh [board_cfg]
#
# Arguments:
#   board_cfg  (optional) path to an OpenOCD .cfg file
#              default: arch/debug_cfg/stm32_f411xx_debug.cfg
#
# Ports exposed:
#   3333  — GDB remote protocol
#   4444  — Telnet (interactive OpenOCD console)
#   6666  — TCL RPC

set -e

# ── Project root (directory containing this script's parent) ──────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ── Configuration ─────────────────────────────────────────────────────────────
BOARD_CFG="${1:-${PROJECT_ROOT}/arch/debug_cfg/stm32_f411xx_debug.cfg}"
GDB_PORT=3333
TELNET_PORT=4444
TCL_PORT=6666

# ── Sanity checks ─────────────────────────────────────────────────────────────
if ! command -v openocd &>/dev/null; then
    echo "ERROR: openocd not found in PATH."
    echo "       Install with: sudo bash scripts/install_openocd.sh"
    exit 1
fi

if [ ! -f "${BOARD_CFG}" ]; then
    echo "ERROR: OpenOCD config not found: ${BOARD_CFG}"
    exit 1
fi

# ── Launch ────────────────────────────────────────────────────────────────────
echo "================================================"
echo " OpenOCD GDB Server"
echo " Config : ${BOARD_CFG}"
echo " GDB    : localhost:${GDB_PORT}"
echo " Telnet : localhost:${TELNET_PORT}"
echo " TCL    : localhost:${TCL_PORT}"
echo "================================================"
echo ""
echo "Connect GDB:"
echo "  arm-none-eabi-gdb build/kernel.elf"
echo "  (gdb) target remote localhost:${GDB_PORT}"
echo ""
echo "Or press F5 in VSCode (Attach config)."
echo ""

openocd \
    -f "${BOARD_CFG}" \
    -c "gdb_port   ${GDB_PORT}" \
    -c "telnet_port ${TELNET_PORT}" \
    -c "tcl_port   ${TCL_PORT}" \
    -c "gdb_memory_map enable" \
    -c "gdb_flash_program enable" \
    -c "gdb report_data_abort enable"
