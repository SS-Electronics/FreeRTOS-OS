#!/bin/bash
set -e

echo "=== Installing ARM GCC Toolchain (arm-none-eabi-gcc) ==="

# Detect distro
if [ -f /etc/debian_version ]; then
    echo "[*] Detected Debian/Ubuntu"
    sudo apt update
    sudo apt install -y gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch

elif [ -f /etc/fedora-release ]; then
    echo "[*] Detected Fedora"
    sudo dnf install -y arm-none-eabi-gcc-cs arm-none-eabi-binutils-cs arm-none-eabi-gdb

elif [ -f /etc/arch-release ]; then
    echo "[*] Detected Arch/Manjaro"
    sudo pacman -Syu --noconfirm arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-gdb

else
    echo "[!] Unknown distro, installing manually from ARM releases..."
    VERSION=13.2.rel1
    TARBALL=gcc-arm-none-eabi-$VERSION-x86_64-arm-none-eabi.tar.bz2
    URL=https://developer.arm.com/-/media/Files/downloads/gnu/$VERSION/binrel/$TARBALL

    wget -O $TARBALL $URL
    tar -xjf $TARBALL
    sudo mv gcc-arm-none-eabi-$VERSION /opt/
    echo "export PATH=/opt/gcc-arm-none-eabi-$VERSION/bin:\$PATH" >> ~/.bashrc
    source ~/.bashrc
fi

echo "=== Installation complete ==="
arm-none-eabi-gcc --version
