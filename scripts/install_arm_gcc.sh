#!/bin/bash
set -e

echo "=== Installing ARM GCC Toolchain (arm-none-eabi-gcc) and GDB ==="

# Detect distro
if [ -f /etc/debian_version ]; then
    echo "[*] Detected Debian/Ubuntu"
    sudo apt update
    # On Debian/Ubuntu the ARM GDB ships as gdb-multiarch (no arm-none-eabi-gdb package).
    # After install we create a symlink so every tool that calls arm-none-eabi-gdb works.
    sudo apt install -y gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch

    if ! command -v arm-none-eabi-gdb &>/dev/null; then
        echo "[*] Creating arm-none-eabi-gdb symlink -> gdb-multiarch"
        sudo ln -sf "$(command -v gdb-multiarch)" /usr/local/bin/arm-none-eabi-gdb
    fi

elif [ -f /etc/fedora-release ]; then
    echo "[*] Detected Fedora"
    sudo dnf install -y arm-none-eabi-gcc-cs arm-none-eabi-binutils-cs arm-none-eabi-gdb

elif [ -f /etc/arch-release ]; then
    echo "[*] Detected Arch/Manjaro"
    sudo pacman -Syu --noconfirm arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-gdb

else
    echo "[!] Unknown distro, installing manually from ARM releases..."

    VERSION=13.2.rel1
    TARBALL=gcc-arm-none-eabi-${VERSION}-x86_64-arm-none-eabi.tar.xz
    URL="https://developer.arm.com/-/media/Files/downloads/gnu/${VERSION}/binrel/${TARBALL}"

    echo "[*] Downloading ${TARBALL} from ARM..."
    wget -O "${TARBALL}" "${URL}" || { echo "[!] Download failed. Check the URL."; exit 1; }

    echo "[*] Extracting..."
    tar -xf "${TARBALL}"

    echo "[*] Moving to /opt/"
    sudo mv "gcc-arm-none-eabi-${VERSION}" /opt/

    TOOLCHAIN_BIN="/opt/gcc-arm-none-eabi-${VERSION}/bin"
    echo "[*] Creating symlinks in /usr/local/bin ..."
    for BIN in "${TOOLCHAIN_BIN}"/arm-none-eabi-*; do
        sudo ln -sf "${BIN}" "/usr/local/bin/$(basename "${BIN}")"
    done
fi

echo ""
echo "=== Installation complete ==="
echo "[*] Verify GCC: arm-none-eabi-gcc --version"
echo "[*] Verify GDB: arm-none-eabi-gdb --version"
