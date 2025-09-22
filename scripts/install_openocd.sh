#!/bin/bash
set -e

echo "=== Installing OpenOCD via APT ==="

# Check for sudo
if ! command -v sudo &> /dev/null; then
    echo "[!] This script requires sudo privileges."
    exit 1
fi

# Update package lists
echo "[*] Updating package lists..."
sudo apt update

# Install OpenOCD
echo "[*] Installing OpenOCD..."
sudo apt install -y openocd

# Verify installation
echo "[*] Installation complete. OpenOCD version:"
openocd --version

echo "[*] You can now run OpenOCD, for example:"
echo "    openocd -f interface/stlink.cfg -f target/stm32f4x.cfg"
