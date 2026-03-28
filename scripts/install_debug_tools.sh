#!/bin/bash
# install_debug_tools.sh
#
# One-shot setup for VSCode embedded debug support.
# Run this once after cloning the repo.
#
# What this script does:
#   1. Checks that arm-none-eabi-gdb and openocd are installed.
#   2. Downloads the STM32F411 SVD file into arch/debug_cfg/
#      (needed for the peripheral register viewer in VSCode).
#   3. Installs the Cortex-Debug VSCode extension.
#
# Usage:
#   bash scripts/install_debug_tools.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SVD_DIR="${PROJECT_ROOT}/arch/debug_cfg"
SVD_FILE="${SVD_DIR}/STM32F411.svd"

RED='\033[0;31m'
GRN='\033[0;32m'
YEL='\033[1;33m'
NC='\033[0m'

ok()   { echo -e "${GRN}[OK]${NC}  $*"; }
warn() { echo -e "${YEL}[WARN]${NC} $*"; }
err()  { echo -e "${RED}[ERR]${NC} $*"; }

echo "================================================"
echo "  FreeRTOS-OS  —  Debug tool installer"
echo "================================================"
echo ""

# ── 1. Check toolchain ────────────────────────────────────────────────────────
echo "── Checking toolchain ──────────────────────────"

if command -v arm-none-eabi-gdb &>/dev/null; then
    ok "arm-none-eabi-gdb: $(arm-none-eabi-gdb --version | head -1)"
else
    err "arm-none-eabi-gdb not found."
    echo "    Install with: sudo bash scripts/install_arm_gcc.sh"
    MISSING=1
fi

if command -v openocd &>/dev/null; then
    ok "openocd: $(openocd --version 2>&1 | head -1)"
else
    err "openocd not found."
    echo "    Install with: sudo bash scripts/install_openocd.sh"
    MISSING=1
fi

if [ "${MISSING}" = "1" ]; then
    echo ""
    err "Fix the missing tools above, then re-run this script."
    exit 1
fi

# ── 2. Download STM32F411 SVD ─────────────────────────────────────────────────
echo ""
echo "── STM32F411 SVD file ──────────────────────────"

if [ -f "${SVD_FILE}" ]; then
    ok "SVD already present: ${SVD_FILE}"
else
    mkdir -p "${SVD_DIR}"

    # Primary source: posborne/cmsis-svd (aggregates all vendor SVDs)
    SVD_URL="https://raw.githubusercontent.com/posborne/cmsis-svd/master/data/STMicro/STM32F411.svd"

    echo "Downloading STM32F411.svd ..."
    if command -v curl &>/dev/null; then
        curl -fsSL "${SVD_URL}" -o "${SVD_FILE}"
    elif command -v wget &>/dev/null; then
        wget -q "${SVD_URL}" -O "${SVD_FILE}"
    else
        err "Neither curl nor wget found. Cannot download SVD."
        err "Manually download from:"
        err "  ${SVD_URL}"
        err "and save to: ${SVD_FILE}"
        exit 1
    fi

    if [ -f "${SVD_FILE}" ] && [ -s "${SVD_FILE}" ]; then
        ok "SVD downloaded: ${SVD_FILE}"
    else
        err "SVD download failed. Check your internet connection."
        err "URL: ${SVD_URL}"
        exit 1
    fi
fi

# ── 3. Install Cortex-Debug VSCode extension ──────────────────────────────────
echo ""
echo "── VSCode Cortex-Debug extension ───────────────"

if command -v code &>/dev/null; then
    if code --list-extensions 2>/dev/null | grep -qi "marus25.cortex-debug"; then
        ok "marus25.cortex-debug already installed"
    else
        echo "Installing marus25.cortex-debug ..."
        code --install-extension marus25.cortex-debug
        ok "marus25.cortex-debug installed"
    fi

    # Install recommended extensions from extensions.json
    for EXT in \
        "ms-vscode.cpptools" \
        "ms-vscode.cpptools-extension-pack" \
        "redhat.vscode-xml" \
        "ms-vscode.makefile-tools" \
        "ms-vscode.vscode-serial-monitor" \
        "ms-vscode.hexeditor"
    do
        if code --list-extensions 2>/dev/null | grep -qi "${EXT}"; then
            ok "${EXT} already installed"
        else
            echo "Installing ${EXT} ..."
            code --install-extension "${EXT}" || warn "Could not install ${EXT}"
        fi
    done
else
    warn "'code' command not found — cannot install VSCode extensions automatically."
    warn "Open VSCode, press Ctrl+Shift+X, and install:"
    warn "  marus25.cortex-debug"
    warn "  ms-vscode.cpptools"
    warn "  ms-vscode.makefile-tools"
fi

# ── Done ──────────────────────────────────────────────────────────────────────
echo ""
echo "================================================"
ok "Debug tools ready."
echo ""
echo "Next steps:"
echo "  1. Open this folder in VSCode:  code ."
echo "  2. Connect your ST-Link to the board."
echo "  3. Press F5  →  'STM32F411 — Build, Flash & Debug'"
echo ""
echo "Panels available after debug starts:"
echo "  Peripherals   — STM32 register values (from SVD)"
echo "  RTOS Threads  — FreeRTOS task list + per-task stack"
echo "  Watch         — live variable expressions"
echo "  Memory        — raw memory inspector"
echo "  Disassembly   — mixed C/asm view"
echo "================================================"
