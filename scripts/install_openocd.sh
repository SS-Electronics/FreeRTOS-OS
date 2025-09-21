#!/bin/bash
#
# Script: install_openocd.sh
# Author: Subhajit Roy
# Purpose: Install OpenOCD server on Linux
#

set -e

echo "###############################################"
echo " Installing OpenOCD on Linux..."
echo "###############################################"

# Detect Linux distro
if [ -f /etc/debian_version ]; then
    DISTRO="debian"
elif [ -f /etc/redhat-release ]; then
    DISTRO="redhat"
else
    DISTRO="unknown"
fi

# Update system
echo "[*] Updating package lists..."
if [ "$DISTRO" = "debian" ]; then
    sudo apt-get update -y || true
fi

# Install dependencies
echo "[*] Installing build dependencies..."
if [ "$DISTRO" = "debian" ]; then
    sudo apt-get install -y \
        git \
        build-essential \
        autoconf \
        automake \
        libtool \
        pkg-config \
        libusb-1.0-0-dev \
        libftdi1-dev \
        libhidapi-dev \
        libjaylink-dev \
        libjaylink0 \
        make \
        libudev-dev \
        texinfo \
        autoconf-archive
fi

# Clone OpenOCD (official repo)
if [ ! -d "openocd" ]; then
    echo "[*] Cloning OpenOCD repository..."
    git clone --recursive https://github.com/openocd-org/openocd.git
else
    echo "[*] OpenOCD directory already exists. Pulling latest..."
    cd openocd
    git pull
    git submodule update --init --recursive
    cd ..
fi

# Build OpenOCD
echo "[*] Building OpenOCD..."
cd openocd

# Ensure jimtcl is initialized
git submodule update --init --recursive

# Clean previous builds
make distclean || true

# Rebuild autotools
./bootstrap

# Add jimtcl include/lib to search path
CPPFLAGS="-I$(pwd)/jimtcl" \
LDFLAGS="-L$(pwd)/jimtcl" \
./configure --enable-stlink --enable-cmsis-dap --enable-jlink

make -j"$(nproc)"

# Install
echo "[*] Installing OpenOCD..."
sudo make install
cd ..

# Udev rules for ST-LINK / CMSIS-DAP / J-Link
echo "[*] Setting up udev rules..."
cat <<EOF | sudo tee /etc/udev/rules.d/60-openocd.rules
# ST-LINK/V2
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3748", MODE="0666"
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374b", MODE="0666"
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374d", MODE="0666"
# CMSIS-DAP
SUBSYSTEM=="usb", ATTR{idVendor}=="0d28", MODE="0666"
# SEGGER J-Link
SUBSYSTEM=="usb", ATTR{idVendor}=="1366", MODE="0666"
EOF

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

echo "###############################################"
echo " OpenOCD installation completed successfully!"
echo " Version: $(openocd --version)"
echo "###############################################"
