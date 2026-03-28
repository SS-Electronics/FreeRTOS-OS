#!/bin/bash
# run_gdb.sh
#
# Connects arm-none-eabi-gdb to a running OpenOCD server.
# Start the server first with:  bash scripts/run_openocd_server.sh
#
# Usage:
#   bash scripts/run_gdb.sh [elf_file] [gdb_port]
#
# Defaults:
#   elf_file : build/kernel.elf
#   gdb_port : 3333

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

ELF="${1:-${PROJECT_ROOT}/build/kernel.elf}"
PORT="${2:-3333}"
GDB="arm-none-eabi-gdb"

if ! command -v "${GDB}" &>/dev/null; then
    echo "ERROR: ${GDB} not found. Install arm-none-eabi-gcc toolchain."
    exit 1
fi

if [ ! -f "${ELF}" ]; then
    echo "ERROR: ELF not found: ${ELF}"
    echo "       Run 'make all' first."
    exit 1
fi

echo "Connecting GDB to OpenOCD on localhost:${PORT} ..."
echo "ELF: ${ELF}"
echo ""

"${GDB}" \
    -ex "set pagination off" \
    -ex "set print pretty on" \
    -ex "set mem inaccessible-by-default off" \
    -ex "target remote localhost:${PORT}" \
    -ex "monitor reset halt" \
    -ex "load" \
    -ex "monitor reset halt" \
    -ex "break main" \
    -ex "continue" \
    "${ELF}"
