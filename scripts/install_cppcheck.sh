#!/usr/bin/env bash
# install_cppcheck.sh
# Installs cppcheck and all dependencies needed for static analysis
# with MISRA C:2012 checks on the FreeRTOS-OS project.
#
# Usage:
#   ./scripts/install_cppcheck.sh [--source] [--version=<ver>]
#
#   --source           Build cppcheck from source instead of apt
#   --version=<ver>    Source build version (default: 2.15.0)

set -euo pipefail

INSTALL_FROM_SOURCE=0
CPPCHECK_SRC_VERSION="2.15.0"
CPPCHECK_SRC_URL="https://github.com/danmar/cppcheck/archive/refs/tags/${CPPCHECK_SRC_VERSION}.tar.gz"

# ── colour helpers ──────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

info()    { echo -e "${CYAN}[INFO]${RESET}  $*"; }
success() { echo -e "${GREEN}[OK]${RESET}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
error()   { echo -e "${RED}[ERR]${RESET}   $*" >&2; }
header()  { echo -e "\n${BOLD}${CYAN}═══ $* ═══${RESET}"; }

# ── argument parsing ────────────────────────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --source)            INSTALL_FROM_SOURCE=1 ;;
        --version=*)         CPPCHECK_SRC_VERSION="${arg#*=}"
                             CPPCHECK_SRC_URL="https://github.com/danmar/cppcheck/archive/refs/tags/${CPPCHECK_SRC_VERSION}.tar.gz" ;;
        -h|--help)
            echo "Usage: $0 [--source] [--version=<ver>]"
            echo ""
            echo "  --source          Build cppcheck from source (latest fixes, slower)"
            echo "  --version=<ver>   Source version to build (default: 2.15.0)"
            exit 0 ;;
        *) warn "Unknown argument: $arg (ignored)" ;;
    esac
done

# ── privilege check ─────────────────────────────────────────────────────────
SUDO=""
if [[ $EUID -ne 0 ]]; then
    if command -v sudo &>/dev/null; then
        SUDO="sudo"
        info "Running with sudo."
    else
        error "Not root and sudo unavailable. Re-run as root."
        exit 1
    fi
fi

# ── detect distro ───────────────────────────────────────────────────────────
detect_distro() {
    if [[ -f /etc/os-release ]]; then
        # shellcheck disable=SC1091
        source /etc/os-release
        echo "${ID:-unknown}"
    else
        echo "unknown"
    fi
}

DISTRO=$(detect_distro)
info "Detected distro: ${DISTRO}"

# ── install from apt (Debian/Ubuntu) ────────────────────────────────────────
install_apt() {
    header "Installing cppcheck via apt"
    $SUDO apt-get update -qq
    $SUDO apt-get install -y \
        cppcheck \
        python3 \
        python3-pip \
        python3-pygments \
        libpcre3

    success "cppcheck installed via apt."
}

# ── install from source ──────────────────────────────────────────────────────
install_from_source() {
    header "Building cppcheck ${CPPCHECK_SRC_VERSION} from source"

    BUILD_DEPS="cmake g++ make libpcre3-dev python3 python3-pip python3-pygments"
    case "$DISTRO" in
        ubuntu|debian|linuxmint|pop)
            $SUDO apt-get update -qq
            # shellcheck disable=SC2086
            $SUDO apt-get install -y $BUILD_DEPS ;;
        fedora|rhel|centos|rocky|almalinux)
            $SUDO dnf install -y cmake gcc-c++ make pcre-devel python3 python3-pip python3-pygments ;;
        arch|manjaro)
            $SUDO pacman -Sy --noconfirm cmake gcc make pcre python python-pip python-pygments ;;
        *)
            warn "Unsupported distro '${DISTRO}'. Install build deps manually."
            warn "Required: cmake g++ make libpcre3-dev python3 python3-pip" ;;
    esac

    WORK_DIR=$(mktemp -d)
    trap 'rm -rf "$WORK_DIR"' EXIT

    info "Downloading cppcheck ${CPPCHECK_SRC_VERSION} ..."
    curl -fsSL "$CPPCHECK_SRC_URL" -o "${WORK_DIR}/cppcheck.tar.gz"
    tar -xzf "${WORK_DIR}/cppcheck.tar.gz" -C "$WORK_DIR"

    SRC_DIR="${WORK_DIR}/cppcheck-${CPPCHECK_SRC_VERSION}"
    BUILD_DIR="${SRC_DIR}/build"
    mkdir -p "$BUILD_DIR"

    info "Configuring build ..."
    cmake -S "$SRC_DIR" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DUSE_MATCHCOMPILER=ON \
        -DHAVE_RULES=ON \
        -DBUILD_GUI=OFF \
        -DWITH_QCHART=OFF

    info "Building (this may take a few minutes) ..."
    cmake --build "$BUILD_DIR" --parallel "$(nproc)"

    info "Installing ..."
    $SUDO cmake --install "$BUILD_DIR"

    success "cppcheck ${CPPCHECK_SRC_VERSION} built and installed."
}

# ── install html report tool ─────────────────────────────────────────────────
install_htmlreport() {
    header "Installing cppcheck-htmlreport"
    if command -v cppcheck-htmlreport &>/dev/null; then
        success "cppcheck-htmlreport already available."
        return
    fi

    # Try system package first
    case "$DISTRO" in
        ubuntu|debian|linuxmint|pop)
            $SUDO apt-get install -y cppcheck && true ;;  # bundled on some distros
    esac

    # Install via pip if still missing
    if ! command -v cppcheck-htmlreport &>/dev/null; then
        info "Installing cppcheck-htmlreport via pip3 ..."
        pip3 install --user cppcheck-htmlreport 2>/dev/null || \
            warn "pip install failed — HTML reports will be unavailable."
    fi
}

# ── install python MISRA addon deps ──────────────────────────────────────────
install_misra_deps() {
    header "Installing MISRA addon dependencies"

    # Pygments is needed by the misra.py addon for coloured output
    if python3 -c "import pygments" &>/dev/null; then
        success "pygments already installed."
    else
        info "Installing pygments ..."
        pip3 install --user pygments 2>/dev/null || \
            $SUDO pip3 install pygments 2>/dev/null || \
            warn "pygments install failed — MISRA output may lack colours."
    fi

    # Verify misra.py is present
    MISRA_PY_CANDIDATES=(
        "/usr/lib/x86_64-linux-gnu/cppcheck/addons/misra.py"
        "/usr/share/cppcheck/addons/misra.py"
        "/usr/local/share/cppcheck/addons/misra.py"
    )
    MISRA_PY=""
    for f in "${MISRA_PY_CANDIDATES[@]}"; do
        if [[ -f "$f" ]]; then
            MISRA_PY="$f"
            break
        fi
    done

    if [[ -n "$MISRA_PY" ]]; then
        success "misra.py found: ${MISRA_PY}"
    else
        warn "misra.py not found in expected locations."
        warn "MISRA checks will require the addon path to be set manually in run_cppcheck.sh."
    fi
}

# ── main ─────────────────────────────────────────────────────────────────────
header "FreeRTOS-OS CPPcheck + MISRA Installer"

if [[ "$INSTALL_FROM_SOURCE" -eq 1 ]]; then
    install_from_source
else
    case "$DISTRO" in
        ubuntu|debian|linuxmint|pop)
            install_apt ;;
        fedora|rhel|centos|rocky|almalinux)
            header "Installing cppcheck via dnf"
            $SUDO dnf install -y cppcheck python3 python3-pip python3-pygments
            success "cppcheck installed via dnf." ;;
        arch|manjaro)
            header "Installing cppcheck via pacman"
            $SUDO pacman -Sy --noconfirm cppcheck python python-pip python-pygments
            success "cppcheck installed via pacman." ;;
        *)
            warn "Auto-install not supported for '${DISTRO}'."
            warn "Install manually and re-run with --source to build from source."
            exit 1 ;;
    esac
fi

install_htmlreport
install_misra_deps

# ── verify ────────────────────────────────────────────────────────────────────
header "Verification"

if command -v cppcheck &>/dev/null; then
    VER=$(cppcheck --version)
    success "cppcheck:           ${VER}"
else
    error "cppcheck binary not found after installation."
    exit 1
fi

if command -v cppcheck-htmlreport &>/dev/null; then
    success "cppcheck-htmlreport: available"
else
    warn "cppcheck-htmlreport: NOT available (HTML reports disabled)"
fi

if python3 -c "import pygments" &>/dev/null; then
    success "pygments:            available"
else
    warn "pygments:            NOT available"
fi

echo ""
success "Installation complete. Run ./scripts/run_cppcheck.sh --help to get started."
