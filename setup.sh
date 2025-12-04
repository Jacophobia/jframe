#!/usr/bin/env bash
#
# JFrame Development Environment Setup Script
# Supports: macOS (Apple Silicon & Intel), Linux (Ubuntu/Debian, Fedora, Arch)
#
# This script is idempotent - safe to run multiple times without side effects.
# All dependencies are installed via package managers for easy global updates.
#
# Usage:
#   ./setup.sh              # Full interactive setup + build
#   ./setup.sh --no-build   # Setup only, skip build
#   ./setup.sh --ci         # Non-interactive mode for CI (no prompts, no build)
#   ./setup.sh --help       # Show help
#

set -e

# =============================================================================
# Configuration
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VCPKG_DIR="${VCPKG_ROOT:-$HOME/vcpkg}"
BUILD_PRESET=""
SKIP_BUILD=false
CI_MODE=false

# Colors for output (disabled in CI mode)
setup_colors() {
    if [ "$CI_MODE" = true ] || [ ! -t 1 ]; then
        RED=''
        GREEN=''
        YELLOW=''
        BLUE=''
        NC=''
    else
        RED='\033[0;31m'
        GREEN='\033[0;32m'
        YELLOW='\033[1;33m'
        BLUE='\033[0;34m'
        NC='\033[0m'
    fi
}

# =============================================================================
# Helper Functions
# =============================================================================

print_header() {
    echo -e "\n${BLUE}══════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}══════════════════════════════════════════════════════════════════${NC}\n"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${BLUE}→ $1${NC}"
}

command_exists() {
    command -v "$1" &> /dev/null
}

version_gte() {
    # Compare version strings: returns 0 if $1 >= $2
    printf '%s\n%s\n' "$2" "$1" | sort -V -C
}

detect_os() {
    case "$(uname -s)" in
        Darwin*)    echo "macos" ;;
        Linux*)     echo "linux" ;;
        *)          echo "unknown" ;;
    esac
}

detect_arch() {
    case "$(uname -m)" in
        arm64|aarch64)  echo "arm64" ;;
        x86_64|amd64)   echo "x86_64" ;;
        *)              echo "unknown" ;;
    esac
}

detect_linux_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        case "$ID" in
            ubuntu|debian|linuxmint|pop)    echo "debian" ;;
            fedora|rhel|centos|rocky|alma)  echo "fedora" ;;
            arch|manjaro|endeavouros)       echo "arch" ;;
            *)                              echo "unknown" ;;
        esac
    else
        echo "unknown"
    fi
}

# Wait for user confirmation (skipped in CI mode)
wait_for_user() {
    local message="$1"
    if [ "$CI_MODE" = true ]; then
        print_info "CI mode: skipping prompt - $message"
        return 0
    fi
    echo ""
    echo -e "${YELLOW}$message${NC}"
    echo ""
    read -p "Press Enter when ready to continue (or Ctrl+C to abort)... "
    echo ""
}

# Ask yes/no question (defaults to yes in CI mode)
ask_yes_no() {
    local question="$1"
    local default="${2:-y}"

    if [ "$CI_MODE" = true ]; then
        return 0  # Always yes in CI mode
    fi

    if [ "$default" = "y" ]; then
        read -p "$question [Y/n]: " response
        case "$response" in
            [nN][oO]|[nN]) return 1 ;;
            *) return 0 ;;
        esac
    else
        read -p "$question [y/N]: " response
        case "$response" in
            [yY][eE][sS]|[yY]) return 0 ;;
            *) return 1 ;;
        esac
    fi
}

show_help() {
    cat << EOF
JFrame Development Environment Setup Script

Usage: ./setup.sh [options]

Options:
    --no-build      Setup environment only, skip building JFrame
    --ci            Non-interactive CI mode (no prompts, implies --no-build)
    --help          Show this help message

Description:
    This script sets up a complete development environment for JFrame with
    ZERO prerequisites. Everything is installed via package managers for
    easy updates (brew upgrade, apt upgrade, etc.).

    macOS:
      1. Installs Xcode Command Line Tools (waits for completion)
      2. Installs Homebrew (if not present)
      3. Installs LLVM 20+ via Homebrew (for C++23 'import std;')
      4. Installs CMake, Ninja via Homebrew
      5. Installs vcpkg (cloned to ~/vcpkg)
      6. Prompts for FMOD installation (optional, for audio)
      7. Configures and builds JFrame

    Linux (Debian/Ubuntu):
      1. Installs system libraries via apt
      2. Installs LLVM 20 via apt (from apt.llvm.org repository)
      3. Installs CMake, Ninja via apt
      4. Installs vcpkg (cloned to ~/vcpkg)
      5. Prompts for FMOD installation (optional, for audio)
      6. Configures and builds JFrame

    The script is idempotent - running it multiple times is safe.

Package Managers Used:
    - macOS: Homebrew (brew upgrade updates all dependencies)
    - Linux: apt/dnf/pacman (system package managers)
    - vcpkg: For C++ libraries (updated via git pull)

Manual Steps:
    - FMOD Core API must be downloaded from https://fmod.com/download
      The script will pause and wait for you to complete this step.

Examples:
    ./setup.sh              # Full interactive setup and build
    ./setup.sh --no-build   # Setup only, skip build
    ./setup.sh --ci         # CI mode (non-interactive, no build)
EOF
}

# =============================================================================
# Package Manager Installation
# =============================================================================

install_homebrew() {
    print_header "Setting up Homebrew"

    if command_exists brew; then
        print_success "Homebrew is already installed"

        # Update Homebrew (skip in CI for speed)
        if [ "$CI_MODE" = false ]; then
            print_info "Updating Homebrew..."
            brew update || print_warning "Failed to update Homebrew (non-fatal)"
        fi
        return 0
    fi

    print_info "Homebrew is not installed."
    print_info "Homebrew is the package manager for macOS and will install LLVM, CMake, etc."

    if ! ask_yes_no "Install Homebrew now?"; then
        print_error "Homebrew is required. Please install it manually from https://brew.sh"
        exit 1
    fi

    print_info "Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    # Add Homebrew to PATH for current session
    if [ "$(detect_arch)" = "arm64" ]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    else
        eval "$(/usr/local/bin/brew shellenv)"
    fi

    print_success "Homebrew installed successfully"
}

setup_homebrew_path() {
    # Ensure Homebrew is in PATH
    if ! command_exists brew; then
        if [ "$(detect_arch)" = "arm64" ] && [ -x /opt/homebrew/bin/brew ]; then
            eval "$(/opt/homebrew/bin/brew shellenv)"
        elif [ -x /usr/local/bin/brew ]; then
            eval "$(/usr/local/bin/brew shellenv)"
        fi
    fi
}

# =============================================================================
# macOS Setup
# =============================================================================

setup_xcode_clt() {
    print_header "Checking Xcode Command Line Tools"

    if xcode-select -p &> /dev/null; then
        print_success "Xcode Command Line Tools already installed"
        return 0
    fi

    print_info "Xcode Command Line Tools are required but not installed."
    print_info "This provides essential build tools (clang, make, git, etc.)"

    # Start installation
    print_info "Starting Xcode Command Line Tools installation..."
    xcode-select --install 2>/dev/null || true

    # Wait for user to complete installation
    wait_for_user "Please complete the Xcode Command Line Tools installation in the popup dialog."

    # Verify installation
    if xcode-select -p &> /dev/null; then
        print_success "Xcode Command Line Tools installed successfully"
    else
        print_error "Xcode Command Line Tools installation not detected."
        print_info "Please run 'xcode-select --install' manually and try again."
        exit 1
    fi
}

setup_macos() {
    print_header "Setting up macOS Development Environment"

    # Step 1: Xcode Command Line Tools
    setup_xcode_clt

    # Step 2: Homebrew
    install_homebrew
    setup_homebrew_path

    # Step 3: LLVM 20 via Homebrew
    print_header "Setting up LLVM 20"

    local LLVM_PATH
    if [ "$(detect_arch)" = "arm64" ]; then
        LLVM_PATH="/opt/homebrew/opt/llvm@20"
    else
        LLVM_PATH="/usr/local/opt/llvm@20"
    fi

    if [ -x "${LLVM_PATH}/bin/clang++" ]; then
        local LLVM_VERSION
        LLVM_VERSION=$("${LLVM_PATH}/bin/clang++" --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)
        print_success "LLVM ${LLVM_VERSION} already installed via Homebrew"
    else
        print_info "Installing LLVM 20 via Homebrew..."
        print_info "This may take several minutes..."
        brew install llvm@20
        print_success "LLVM 20 installed"
    fi

    # Verify std.cppm exists (required for import std;)
    if [ -f "${LLVM_PATH}/share/libc++/v1/std.cppm" ]; then
        print_success "C++23 module support (std.cppm) verified"
    else
        print_warning "std.cppm not found - C++23 module support may be incomplete"
        print_info "Try: brew reinstall llvm@20"
    fi

    # Step 4: Build tools via Homebrew
    print_header "Setting up Build Tools"

    # CMake
    if command_exists cmake; then
        local CMAKE_VERSION
        CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
        if version_gte "$CMAKE_VERSION" "3.28.0"; then
            print_success "CMake ${CMAKE_VERSION} already installed"
        else
            print_info "CMake ${CMAKE_VERSION} is too old, upgrading..."
            brew upgrade cmake
        fi
    else
        print_info "Installing CMake via Homebrew..."
        brew install cmake
        print_success "CMake installed"
    fi

    # Ninja
    if command_exists ninja; then
        print_success "Ninja already installed"
    else
        print_info "Installing Ninja via Homebrew..."
        brew install ninja
        print_success "Ninja installed"
    fi

    # pkg-config (needed by some vcpkg packages)
    if command_exists pkg-config; then
        print_success "pkg-config already installed"
    else
        print_info "Installing pkg-config via Homebrew..."
        brew install pkg-config
    fi

    # Git (usually comes with Xcode CLT, but ensure it's available)
    if command_exists git; then
        print_success "Git already installed"
    else
        print_info "Installing Git via Homebrew..."
        brew install git
    fi

    # Set environment variables for build
    export JFRAME_CC="${LLVM_PATH}/bin/clang"
    export JFRAME_CXX="${LLVM_PATH}/bin/clang++"
    export LDFLAGS="-L${LLVM_PATH}/lib -L${LLVM_PATH}/lib/c++ -Wl,-rpath,${LLVM_PATH}/lib/c++"
    export CPPFLAGS="-I${LLVM_PATH}/include"
    export VCPKG_OVERLAY_TRIPLETS="${SCRIPT_DIR}/triplets"

    BUILD_PRESET="macos-debug"
}

# =============================================================================
# Linux Setup
# =============================================================================

install_linux_system_libs_debian() {
    print_info "Installing system libraries via apt..."

    # All packages needed for JFrame development
    local PACKAGES=(
        # Build essentials
        build-essential
        pkg-config
        curl
        wget
        git
        # X11 and OpenGL
        libx11-dev
        libxrandr-dev
        libxinerama-dev
        libxcursor-dev
        libxi-dev
        libgl1-mesa-dev
        libglu1-mesa-dev
        # Audio
        libasound2-dev
        libpulse-dev
        # Wayland
        libwayland-dev
        libxkbcommon-dev
        # Autotools (required for vcpkg packages)
        autoconf
        autoconf-archive
        automake
        libtool
        libltdl-dev
        # Build tools
        ninja-build
        cmake
        # LLVM prerequisites
        lsb-release
        software-properties-common
        gnupg
    )

    sudo apt-get update
    sudo apt-get install -y "${PACKAGES[@]}"
    print_success "System libraries installed via apt"
}

install_linux_system_libs_fedora() {
    print_info "Installing system libraries via dnf..."

    local PACKAGES=(
        gcc-c++
        make
        pkgconfig
        curl
        wget
        git
        libX11-devel
        libXrandr-devel
        libXinerama-devel
        libXcursor-devel
        libXi-devel
        mesa-libGL-devel
        mesa-libGLU-devel
        alsa-lib-devel
        pulseaudio-libs-devel
        wayland-devel
        libxkbcommon-devel
        autoconf
        autoconf-archive
        automake
        libtool
        libtool-ltdl-devel
        ninja-build
        cmake
        redhat-lsb-core
        gnupg2
    )

    sudo dnf install -y "${PACKAGES[@]}"
    print_success "System libraries installed via dnf"
}

install_linux_system_libs_arch() {
    print_info "Installing system libraries via pacman..."

    local PACKAGES=(
        base-devel
        pkgconf
        curl
        wget
        git
        libx11
        libxrandr
        libxinerama
        libxcursor
        libxi
        mesa
        glu
        alsa-lib
        libpulse
        wayland
        libxkbcommon
        autoconf
        autoconf-archive
        automake
        libtool
        ninja
        cmake
        lsb-release
        gnupg
    )

    sudo pacman -S --noconfirm --needed "${PACKAGES[@]}"
    print_success "System libraries installed via pacman"
}

install_llvm_apt() {
    print_header "Setting up LLVM 20 via apt"

    # Check if already installed
    if [ -x /usr/bin/clang-20 ]; then
        local LLVM_VERSION
        LLVM_VERSION=$(/usr/bin/clang-20 --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)
        print_success "LLVM ${LLVM_VERSION} already installed via apt"

        # Verify std.cppm
        if [ -f "/usr/lib/llvm-20/share/libc++/v1/std.cppm" ]; then
            print_success "C++23 module support (std.cppm) verified"
        else
            print_warning "std.cppm not found - module support may be incomplete"
        fi
        return 0
    fi

    print_info "LLVM 20 will be installed from apt.llvm.org"
    print_info "This adds the official LLVM apt repository, so 'apt upgrade' will update LLVM."

    # Download and run the official LLVM installation script
    # This script adds the apt.llvm.org repository and installs via apt
    print_info "Downloading LLVM installation script from apt.llvm.org..."
    wget -q https://apt.llvm.org/llvm.sh -O /tmp/llvm.sh
    chmod +x /tmp/llvm.sh

    print_info "Installing LLVM 20 (this may take several minutes)..."
    sudo /tmp/llvm.sh 20 all

    rm -f /tmp/llvm.sh

    # Verify installation
    if [ -x /usr/bin/clang-20 ]; then
        local LLVM_VERSION
        LLVM_VERSION=$(/usr/bin/clang-20 --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)
        print_success "LLVM ${LLVM_VERSION} installed via apt"
    else
        print_error "LLVM 20 installation failed"
        exit 1
    fi

    # Verify std.cppm
    if [ -f "/usr/lib/llvm-20/share/libc++/v1/std.cppm" ]; then
        print_success "C++23 module support (std.cppm) verified"
    else
        print_warning "std.cppm not found - module support may be incomplete"
    fi
}

verify_cmake_version() {
    print_header "Verifying CMake Version"

    if ! command_exists cmake; then
        print_error "CMake not found after installation"
        exit 1
    fi

    local CMAKE_VERSION
    CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')

    if version_gte "$CMAKE_VERSION" "3.28.0"; then
        print_success "CMake ${CMAKE_VERSION} meets minimum requirement (3.28+)"
    else
        print_error "CMake ${CMAKE_VERSION} is too old. Need 3.28+ for C++23 modules."
        print_info "Please install a newer CMake from https://cmake.org/download/"
        exit 1
    fi
}

setup_linux() {
    print_header "Setting up Linux Development Environment"

    local DISTRO
    DISTRO=$(detect_linux_distro)
    print_info "Detected distribution family: ${DISTRO}"

    # Step 1: System libraries via package manager
    print_header "Installing System Libraries"
    case "$DISTRO" in
        debian)
            install_linux_system_libs_debian
            ;;
        fedora)
            install_linux_system_libs_fedora
            ;;
        arch)
            install_linux_system_libs_arch
            ;;
        *)
            print_warning "Unknown distribution: $DISTRO"
            print_info "You may need to install system libraries manually."
            wait_for_user "Install these packages: libx11-dev, libgl1-mesa-dev, libasound2-dev, autoconf, automake, libtool, cmake, ninja-build"
            ;;
    esac

    # Step 2: LLVM 20 via apt (adds apt.llvm.org repository)
    if [ "$DISTRO" = "debian" ]; then
        install_llvm_apt
    else
        # For non-Debian, try to use system LLVM or provide guidance
        if [ -x /usr/bin/clang-20 ] || [ -x /usr/bin/clang ]; then
            print_success "Clang compiler found"
        else
            print_warning "LLVM 20 installation for $DISTRO may require manual steps"
            print_info "Please install clang-20 or equivalent from your package manager"
            wait_for_user "Install LLVM/Clang 20+ for your distribution"
        fi
    fi

    # Step 3: Verify CMake version
    verify_cmake_version

    # Set environment variables for build
    export JFRAME_CC="/usr/bin/clang-20"
    export JFRAME_CXX="/usr/bin/clang++-20"
    export VCPKG_DEFAULT_TRIPLET="x64-linux-libcxx"
    export VCPKG_OVERLAY_TRIPLETS="${SCRIPT_DIR}/triplets"

    BUILD_PRESET="linux-debug"
}

# =============================================================================
# vcpkg Setup
# =============================================================================

setup_vcpkg() {
    print_header "Setting up vcpkg"

    # Check if vcpkg is already installed and functional
    if [ -x "${VCPKG_DIR}/vcpkg" ]; then
        print_success "vcpkg already installed at ${VCPKG_DIR}"

        # Update vcpkg (skip in CI for speed, CI uses specific commit)
        if [ "$CI_MODE" = false ]; then
            print_info "Updating vcpkg..."
            cd "${VCPKG_DIR}"
            git pull --quiet || print_warning "Failed to update vcpkg (non-fatal)"
            ./bootstrap-vcpkg.sh -disableMetrics >/dev/null 2>&1 || true
            cd "${SCRIPT_DIR}"
        fi
        return 0
    fi

    print_info "vcpkg is a C++ package manager that will install project dependencies."
    print_info "It will be cloned to: ${VCPKG_DIR}"
    print_info "Update it anytime with: cd ${VCPKG_DIR} && git pull && ./bootstrap-vcpkg.sh"

    if ! ask_yes_no "Install vcpkg now?"; then
        print_error "vcpkg is required for building JFrame."
        exit 1
    fi

    print_info "Cloning vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git "${VCPKG_DIR}"

    print_info "Bootstrapping vcpkg..."
    cd "${VCPKG_DIR}"
    ./bootstrap-vcpkg.sh -disableMetrics
    cd "${SCRIPT_DIR}"

    print_success "vcpkg installed successfully"
}

# =============================================================================
# FMOD Setup (Optional)
# =============================================================================

setup_fmod() {
    print_header "FMOD Audio Library (Optional)"

    if [ -d "${SCRIPT_DIR}/external/fmod/core" ]; then
        print_success "FMOD already installed at external/fmod/core"
        return 0
    fi

    print_info "FMOD is required for audio features but is NOT installed."
    print_info "FMOD is proprietary and must be downloaded manually from:"
    echo ""
    echo "    https://fmod.com/download"
    echo ""
    print_info "After downloading:"
    echo "    1. Extract the FMOD Core API archive"
    echo "    2. Copy the contents to: ${SCRIPT_DIR}/external/fmod/core/"
    echo ""

    if [ "$CI_MODE" = true ]; then
        print_warning "CI mode: Skipping FMOD (audio features will be disabled)"
        return 0
    fi

    if ask_yes_no "Would you like to install FMOD now?" "n"; then
        print_info "Opening FMOD download page..."
        if command_exists open; then
            open "https://fmod.com/download"
        elif command_exists xdg-open; then
            xdg-open "https://fmod.com/download"
        fi

        wait_for_user "Please download and extract FMOD Core API to: ${SCRIPT_DIR}/external/fmod/core/"

        if [ -d "${SCRIPT_DIR}/external/fmod/core" ]; then
            print_success "FMOD installation detected"
        else
            print_warning "FMOD not detected. Audio features will be disabled."
        fi
    else
        print_warning "Skipping FMOD. Audio features will be disabled."
        print_info "You can install FMOD later and re-run this script."
    fi
}

# =============================================================================
# Build JFrame
# =============================================================================

build_jframe() {
    print_header "Building JFrame"

    cd "${SCRIPT_DIR}"

    # Set environment for build
    export VCPKG_ROOT="${VCPKG_DIR}"

    # Set compiler paths
    if [ "$(detect_os)" = "macos" ]; then
        local LLVM_PATH
        if [ "$(detect_arch)" = "arm64" ]; then
            LLVM_PATH="/opt/homebrew/opt/llvm@20"
        else
            LLVM_PATH="/usr/local/opt/llvm@20"
        fi
        export CC="${LLVM_PATH}/bin/clang"
        export CXX="${LLVM_PATH}/bin/clang++"
        export JFRAME_CC="${CC}"
        export JFRAME_CXX="${CXX}"
    elif [ "$(detect_os)" = "linux" ]; then
        export CC="/usr/bin/clang-20"
        export CXX="/usr/bin/clang++-20"
        export JFRAME_CC="${CC}"
        export JFRAME_CXX="${CXX}"
    fi

    print_info "Using compiler: ${CXX}"

    # Clean incompatible build cache
    if [ -d "build/${BUILD_PRESET}" ] && [ -f "build/${BUILD_PRESET}/CMakeCache.txt" ]; then
        if ! grep -q "CMAKE_TOOLCHAIN_FILE.*vcpkg" "build/${BUILD_PRESET}/CMakeCache.txt" 2>/dev/null; then
            print_info "Removing incompatible build cache..."
            rm -rf "build/${BUILD_PRESET}"
        fi
    fi

    # Configure
    print_info "Configuring with preset: ${BUILD_PRESET}"
    print_info "This will download and build vcpkg dependencies (may take several minutes on first run)..."

    if ! cmake --preset "${BUILD_PRESET}" \
        -DCMAKE_C_COMPILER="${JFRAME_CC}" \
        -DCMAKE_CXX_COMPILER="${JFRAME_CXX}"; then
        print_error "CMake configuration failed"
        print_info "Try removing the build directory: rm -rf build/${BUILD_PRESET}"
        exit 1
    fi
    print_success "Configuration complete"

    # Build
    print_info "Building JFrame..."
    if ! cmake --build --preset "${BUILD_PRESET}" --parallel; then
        print_error "Build failed"
        exit 1
    fi
    print_success "Build complete"

    # Run tests
    print_info "Running tests..."
    local CPU_COUNT JOBS
    if [ "$(detect_os)" = "macos" ]; then
        CPU_COUNT=$(sysctl -n hw.ncpu)
    else
        CPU_COUNT=$(nproc)
    fi
    JOBS=$(( CPU_COUNT - 1 ))
    [ $JOBS -lt 1 ] && JOBS=1

    print_info "Running tests with ${JOBS} parallel jobs"

    if ctest --preset "${BUILD_PRESET}" -j "${JOBS}" --output-on-failure; then
        print_success "All tests passed"
    else
        print_warning "Some tests failed - check output above"
    fi
}

# =============================================================================
# Post-Setup Instructions
# =============================================================================

print_post_setup() {
    print_header "Setup Complete!"

    local OS
    OS=$(detect_os)

    echo "Your JFrame development environment is ready."
    echo ""

    if [ "$OS" = "macos" ]; then
        echo "Compiler: LLVM Clang 20 (via Homebrew)"
        echo "  Update with: brew upgrade llvm@20"
    elif [ "$OS" = "linux" ]; then
        echo "Compiler: LLVM Clang 20 (via apt.llvm.org)"
        echo "  Update with: sudo apt upgrade"
    fi
    echo ""

    if [ ! -d "${SCRIPT_DIR}/external/fmod/core" ]; then
        echo -e "${YELLOW}NOTE: FMOD is not installed (audio features disabled)${NC}"
        echo "  Install from: https://fmod.com/download"
        echo "  Extract to: external/fmod/core/"
        echo ""
    fi

    echo "Useful commands:"
    echo ""
    echo "  # Rebuild"
    echo "  cmake --build --preset ${BUILD_PRESET} --parallel"
    echo ""
    echo "  # Run tests"
    echo "  ctest --preset ${BUILD_PRESET}"
    echo ""
    echo "  # Clean rebuild"
    echo "  rm -rf build/${BUILD_PRESET} && cmake --preset ${BUILD_PRESET} && cmake --build --preset ${BUILD_PRESET}"
    echo ""
    echo "  # Update all dependencies"
    if [ "$OS" = "macos" ]; then
        echo "  brew upgrade                    # Update Homebrew packages"
    elif [ "$OS" = "linux" ]; then
        echo "  sudo apt update && sudo apt upgrade  # Update apt packages"
    fi
    echo "  cd ~/vcpkg && git pull          # Update vcpkg"
    echo ""

    echo "Add to your shell profile (~/.bashrc or ~/.zshrc):"
    echo ""
    echo "  export VCPKG_ROOT=\"${VCPKG_DIR}\""
    if [ "$OS" = "linux" ]; then
        echo "  export JFRAME_CC=/usr/bin/clang-20"
        echo "  export JFRAME_CXX=/usr/bin/clang++-20"
    elif [ "$OS" = "macos" ]; then
        local LLVM_PATH
        if [ "$(detect_arch)" = "arm64" ]; then
            LLVM_PATH="/opt/homebrew/opt/llvm@20"
            echo "  eval \"\$(/opt/homebrew/bin/brew shellenv)\""
        else
            LLVM_PATH="/usr/local/opt/llvm@20"
            echo "  eval \"\$(/usr/local/bin/brew shellenv)\""
        fi
        echo "  export JFRAME_CC=\"${LLVM_PATH}/bin/clang\""
        echo "  export JFRAME_CXX=\"${LLVM_PATH}/bin/clang++\""
    fi
    echo ""
}

# =============================================================================
# Main
# =============================================================================

main() {
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --no-build)
                SKIP_BUILD=true
                shift
                ;;
            --ci)
                CI_MODE=true
                SKIP_BUILD=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done

    # Setup colors based on mode
    setup_colors

    print_header "JFrame Development Environment Setup"

    local OS
    OS=$(detect_os)

    echo "Operating System: ${OS}"
    echo "Architecture: $(detect_arch)"
    echo "Script Directory: ${SCRIPT_DIR}"
    echo "vcpkg Directory: ${VCPKG_DIR}"
    if [ "$CI_MODE" = true ]; then
        echo "Mode: CI (non-interactive)"
    else
        echo "Mode: Interactive"
    fi
    echo ""

    case "$OS" in
        macos)
            setup_macos
            ;;
        linux)
            setup_linux
            ;;
        *)
            print_error "Unsupported operating system: ${OS}"
            print_info "For Windows, please use setup.ps1 instead."
            exit 1
            ;;
    esac

    setup_vcpkg
    setup_fmod

    if [ "$SKIP_BUILD" = false ]; then
        build_jframe
    else
        print_info "Skipping build (--no-build or --ci specified)"
    fi

    print_post_setup
}

main "$@"
