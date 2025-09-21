#!/bin/bash
set -e

echo "============================================="
echo " Installing kconfig-frontends (menuconfig)..."
echo "============================================="

# Try installing via apt
if apt-cache show kconfig-frontends >/dev/null 2>&1; then
    echo "[INFO] Installing via apt..."
    sudo apt update
    sudo apt install -y kconfig-frontends
else
    echo "[WARN] Package not found in apt. Building from source..."

    sudo apt update
    sudo apt install -y build-essential bison flex gperf libncurses5-dev \
        libncursesw5-dev gettext pkg-config wget tar

    TMPDIR=$(mktemp -d)
    cd "$TMPDIR"
    wget https://bitbucket.org/nuttx/tools/downloads/kconfig-frontends-4.11.0.1.tar.bz2
    tar -xf kconfig-frontends-4.11.0.1.tar.bz2
    cd kconfig-frontends-4.11.0.1
    ./configure --enable-conf --enable-mconf --disable-shared --enable-static
    make -j"$(nproc)"
    sudo make install
    cd ~
    rm -rf "$TMPDIR"
fi

# Verify installation
if command -v kconfig-mconf >/dev/null 2>&1; then
    echo "✅ Installation successful!"
    echo "Run it with: kconfig-mconf Kconfig"
    which kconfig-mconf
else
    echo "❌ Installation failed. Please check logs."
    exit 1
fi
