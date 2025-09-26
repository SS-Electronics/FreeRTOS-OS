#!/bin/bash
#------------------------------------------------------------
# install_doxygen.sh
# Install Doxygen and Graphviz (optional) on Linux/macOS
#------------------------------------------------------------

set -e

echo "----------------------------------------"
echo "Installing Doxygen and Graphviz..."
echo "----------------------------------------"

OS=$(uname)

if [[ "$OS" == "Linux" ]]; then
    if command -v apt >/dev/null 2>&1; then
        echo "Detected Debian/Ubuntu-based system"
        sudo apt update
        sudo apt install -y doxygen graphviz
    elif command -v dnf >/dev/null 2>&1; then
        echo "Detected Fedora/RedHat-based system"
        sudo dnf install -y doxygen graphviz
    else
        echo "Unsupported Linux distribution. Please install doxygen manually."
        exit 1
    fi
elif [[ "$OS" == "Darwin" ]]; then
    echo "Detected macOS"
    if ! command -v brew >/dev/null 2>&1; then
        echo "Homebrew not found. Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    brew install doxygen graphviz
else
    echo "Unsupported OS: $OS"
    echo "On Windows, download Doxygen from https://www.doxygen.nl/download.html"
    exit 1
fi

echo "----------------------------------------"
echo "Doxygen installation complete!"
echo "Check version:"
doxygen --version
echo "You can now run: doxygen Doxyfile"
echo "----------------------------------------"
