#!/usr/bin/env bash
#
# JFrame Development Environment Setup Script
# Supports: macOS (Apple Silicon & Intel), Linux (Ubuntu/Debian, Fedora, Arch)
#
# This script is idempotent - safe to run multiple times without side effects.
#
# Usage:
#   ./setup.sh              # Full setup + build
#   ./setup.sh --no-build   # Setup only, skip build
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

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

show_help() {
    cat << EOF
JFrame Development Environment Setup Script

Usage: ./setup.sh [options]

Options:
    --no-build      Setup environment only, skip building JFrame
    --help          Show this help message

Description:
    This script sets up a complete development environment for JFrame:

    1. Installs Homebrew (macOS/Linux) if not present
    2. Installs LLVM/Clang 20+ (required for C++23 'import std;')
    3. Installs CMake, Ninja, and other build tools
    4. Installs vcpkg package manager
    5. Installs system libraries (Linux only)
    6. Configures and builds JFrame with C++23

    The script is idempotent - running it multiple times is safe and will
    only install/update components that are missing or outdated.

Manual Steps Required:
    FMOD Core API must be downloaded manually from https://fmod.com/download
    See docs/Installation.md for FMOD setup instructions.

Examples:
    ./setup.sh              # Full setup and build
    ./setup.sh --no-build   # Setup only
EOF
}

# =============================================================================
# Package Manager Installation
# =============================================================================

install_homebrew() {
    print_header "Installing Homebrew"

    if command_exists brew; then
        print_success "Homebrew is already installed"

        # Update Homebrew
        print_info "Updating Homebrew..."
        brew update || print_warning "Failed to update Homebrew (non-fatal)"
        return 0
    fi

    print_info "Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    # Add Homebrew to PATH for current session
    if [ "$(detect_os)" = "macos" ]; then
        if [ "$(detect_arch)" = "arm64" ]; then
            eval "$(/opt/homebrew/bin/brew shellenv)"
        else
            eval "$(/usr/local/bin/brew shellenv)"
        fi
    else
        # Linux
        eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"
    fi

    print_success "Homebrew installed successfully"
}

setup_homebrew_path() {
    # Ensure Homebrew is in PATH
    if ! command_exists brew; then
        if [ "$(detect_os)" = "macos" ]; then
            if [ "$(detect_arch)" = "arm64" ] && [ -x /opt/homebrew/bin/brew ]; then
                eval "$(/opt/homebrew/bin/brew shellenv)"
            elif [ -x /usr/local/bin/brew ]; then
                eval "$(/usr/local/bin/brew shellenv)"
            fi
        else
            if [ -x /home/linuxbrew/.linuxbrew/bin/brew ]; then
                eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"
            fi
        fi
    fi
}

# =============================================================================
# macOS Setup
# =============================================================================

setup_macos() {
    print_header "Setting up macOS Development Environment"

    # Install Xcode Command Line Tools
    print_info "Checking Xcode Command Line Tools..."
    if ! xcode-select -p &> /dev/null; then
        print_info "Installing Xcode Command Line Tools..."
        xcode-select --install
        print_warning "Please complete the Xcode Command Line Tools installation and re-run this script."
        exit 1
    fi
    print_success "Xcode Command Line Tools installed"

    # Install Homebrew
    install_homebrew
    setup_homebrew_path

    # Determine LLVM path based on architecture
    local LLVM_PATH
    if [ "$(detect_arch)" = "arm64" ]; then
        LLVM_PATH="/opt/homebrew/opt/llvm@20"
    else
        LLVM_PATH="/usr/local/opt/llvm@20"
    fi

    # Install LLVM 20 (required for import std;)
    print_info "Checking LLVM 20..."
    if [ ! -x "${LLVM_PATH}/bin/clang++" ]; then
        print_info "Installing LLVM 20..."
        brew install llvm@20
    fi

    # Verify LLVM version
    local LLVM_VERSION
    LLVM_VERSION=$("${LLVM_PATH}/bin/clang++" --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)
    if [ -n "$LLVM_VERSION" ]; then
        print_success "LLVM ${LLVM_VERSION} installed at ${LLVM_PATH}"
    else
        print_error "Failed to verify LLVM installation"
        exit 1
    fi

    # Verify std.cppm exists
    if [ ! -f "${LLVM_PATH}/share/libc++/v1/std.cppm" ]; then
        print_error "std.cppm not found - LLVM may not have module support"
        print_info "Try reinstalling: brew reinstall llvm@20"
        exit 1
    fi
    print_success "std.cppm module found"

    # Install CMake and Ninja
    print_info "Installing build tools..."
    brew install cmake ninja || true

    # Verify CMake version
    local CMAKE_VERSION
    CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
    if version_gte "$CMAKE_VERSION" "3.28.0"; then
        print_success "CMake ${CMAKE_VERSION} installed"
    else
        print_error "CMake version ${CMAKE_VERSION} is too old. Need 3.28+"
        print_info "Try: brew upgrade cmake"
        exit 1
    fi

    # Install Git
    if ! command_exists git; then
        print_info "Installing Git..."
        brew install git
    fi
    print_success "Git installed"

    # Install pkg-config (needed by some vcpkg packages)
    brew install pkg-config || true

    BUILD_PRESET="macos-debug"
}

# =============================================================================
# Linux Setup
# =============================================================================

install_linux_system_libs_debian() {
    print_info "Installing system libraries (Debian/Ubuntu)..."

    local PACKAGES=(
        # Build essentials
        build-essential
        pkg-config
        curl
        wget
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
        # Wayland (optional, for future support)
        libwayland-dev
        libxkbcommon-dev
    )

    sudo apt-get update
    sudo apt-get install -y "${PACKAGES[@]}"
}

install_linux_system_libs_fedora() {
    print_info "Installing system libraries (Fedora/RHEL)..."

    local PACKAGES=(
        # Build essentials
        gcc-c++
        make
        pkgconfig
        curl
        wget
        # X11 and OpenGL
        libX11-devel
        libXrandr-devel
        libXinerama-devel
        libXcursor-devel
        libXi-devel
        mesa-libGL-devel
        mesa-libGLU-devel
        # Audio
        alsa-lib-devel
        pulseaudio-libs-devel
        # Wayland
        wayland-devel
        libxkbcommon-devel
    )

    sudo dnf install -y "${PACKAGES[@]}"
}

install_linux_system_libs_arch() {
    print_info "Installing system libraries (Arch)..."

    local PACKAGES=(
        # Build essentials
        base-devel
        pkgconf
        curl
        wget
        # X11 and OpenGL
        libx11
        libxrandr
        libxinerama
        libxcursor
        libxi
        mesa
        glu
        # Audio
        alsa-lib
        libpulse
        # Wayland
        wayland
        libxkbcommon
    )

    sudo pacman -S --noconfirm --needed "${PACKAGES[@]}"
}

setup_linux() {
    print_header "Setting up Linux Development Environment"

    local DISTRO
    DISTRO=$(detect_linux_distro)
    print_info "Detected distribution family: ${DISTRO}"

    # Install system libraries first (needed for Homebrew and other tools)
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
            print_warning "Unknown distribution. You may need to install system libraries manually."
            print_info "Required: X11, OpenGL, ALSA/PulseAudio development packages"
            ;;
    esac
    print_success "System libraries installed"

    # Install Homebrew for Linux
    install_homebrew
    setup_homebrew_path

    # LLVM path for Linux (Homebrew)
    local LLVM_PATH="/home/linuxbrew/.linuxbrew/opt/llvm@20"

    # Install LLVM 20 via Homebrew (required for import std;)
    print_info "Checking LLVM 20..."
    if [ ! -x "${LLVM_PATH}/bin/clang++" ]; then
        print_info "Installing LLVM 20..."
        brew install llvm@20
    fi

    # Verify LLVM version
    local LLVM_VERSION
    LLVM_VERSION=$("${LLVM_PATH}/bin/clang++" --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1)
    if [ -n "$LLVM_VERSION" ]; then
        print_success "LLVM ${LLVM_VERSION} installed at ${LLVM_PATH}"
    else
        print_error "Failed to verify LLVM installation"
        exit 1
    fi

    # Verify std.cppm exists
    if [ ! -f "${LLVM_PATH}/share/libc++/v1/std.cppm" ]; then
        print_error "std.cppm not found - LLVM may not have module support"
        print_info "Try reinstalling: brew reinstall llvm@20"
        exit 1
    fi
    print_success "std.cppm module found"

    # Install CMake and Ninja via Homebrew
    print_info "Installing build tools..."
    brew install cmake ninja || true

    # Verify CMake version
    local CMAKE_VERSION
    CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
    if version_gte "$CMAKE_VERSION" "3.28.0"; then
        print_success "CMake ${CMAKE_VERSION} installed"
    else
        print_error "CMake version ${CMAKE_VERSION} is too old. Need 3.28+"
        exit 1
    fi

    # Install Git if not present
    if ! command_exists git; then
        print_info "Installing Git..."
        brew install git
    fi
    print_success "Git installed"

    BUILD_PRESET="linux-debug"
}

# =============================================================================
# vcpkg Setup
# =============================================================================

setup_vcpkg() {
    print_header "Setting up vcpkg"

    if [ -x "${VCPKG_DIR}/vcpkg" ]; then
        print_success "vcpkg already installed at ${VCPKG_DIR}"

        # Update vcpkg
        print_info "Updating vcpkg..."
        cd "${VCPKG_DIR}"
        git pull --quiet || print_warning "Failed to update vcpkg (non-fatal)"
        ./bootstrap-vcpkg.sh -disableMetrics || true
        cd "${SCRIPT_DIR}"
        return 0
    fi

    print_info "Installing vcpkg to ${VCPKG_DIR}..."

    # Clone vcpkg
    git clone https://github.com/microsoft/vcpkg.git "${VCPKG_DIR}"

    # Bootstrap vcpkg
    cd "${VCPKG_DIR}"
    ./bootstrap-vcpkg.sh -disableMetrics
    cd "${SCRIPT_DIR}"

    print_success "vcpkg installed successfully"

    # Suggest adding to PATH
    print_info "Consider adding vcpkg to your shell profile:"
    echo ""
    echo "    export VCPKG_ROOT=\"${VCPKG_DIR}\""
    echo "    export PATH=\"\${VCPKG_ROOT}:\${PATH}\""
    echo ""
}

# =============================================================================
# Build JFrame
# =============================================================================

build_jframe() {
    print_header "Building JFrame"

    cd "${SCRIPT_DIR}"

    # Set VCPKG_ROOT for CMake
    export VCPKG_ROOT="${VCPKG_DIR}"

    # Check for FMOD
    if [ ! -d "${SCRIPT_DIR}/external/fmod/core" ]; then
        print_warning "FMOD not found in external/fmod/core"
        print_info "Audio features will not work without FMOD."
        print_info "Download FMOD Core API from: https://fmod.com/download"
        print_info "See docs/Installation.md for setup instructions."
        echo ""
    fi

    # Clean any existing configuration issues
    if [ -d "build/${BUILD_PRESET}" ] && [ -f "build/${BUILD_PRESET}/CMakeCache.txt" ]; then
        # Check if the CMake cache is compatible
        if ! grep -q "CMAKE_TOOLCHAIN_FILE.*vcpkg" "build/${BUILD_PRESET}/CMakeCache.txt" 2>/dev/null; then
            print_info "Removing incompatible build cache..."
            rm -rf "build/${BUILD_PRESET}"
        fi
    fi

    # Configure
    print_info "Configuring with preset: ${BUILD_PRESET}"
    if ! cmake --preset "${BUILD_PRESET}"; then
        print_error "CMake configuration failed"
        print_info "Try removing the build directory and running again:"
        echo "    rm -rf build/${BUILD_PRESET}"
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
    if ctest --preset "${BUILD_PRESET}" --output-on-failure; then
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

    echo "Your JFrame development environment is ready."
    echo ""

    if [ ! -d "${SCRIPT_DIR}/external/fmod/core" ]; then
        echo -e "${YELLOW}IMPORTANT: FMOD is not installed${NC}"
        echo ""
        echo "To enable audio features:"
        echo "  1. Download FMOD Core API from: https://fmod.com/download"
        echo "  2. Extract and copy to: external/fmod/core/"
        echo "  3. Re-run this script or rebuild manually"
        echo ""
    fi

    echo "Useful commands:"
    echo ""
    echo "  # Rebuild"
    echo "  cmake --build --preset ${BUILD_PRESET}"
    echo ""
    echo "  # Run tests"
    echo "  ctest --preset ${BUILD_PRESET}"
    echo ""
    echo "  # Clean rebuild"
    echo "  rm -rf build/${BUILD_PRESET}"
    echo "  cmake --preset ${BUILD_PRESET}"
    echo "  cmake --build --preset ${BUILD_PRESET}"
    echo ""

    # Shell profile suggestions
    if [ -z "$VCPKG_ROOT" ]; then
        echo "Add these lines to your shell profile (~/.bashrc or ~/.zshrc):"
        echo ""
        echo "  export VCPKG_ROOT=\"${VCPKG_DIR}\""
        if [ "$(detect_os)" = "linux" ]; then
            echo "  eval \"\$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)\""
        elif [ "$(detect_os)" = "macos" ]; then
            if [ "$(detect_arch)" = "arm64" ]; then
                echo "  eval \"\$(/opt/homebrew/bin/brew shellenv)\""
            else
                echo "  eval \"\$(/usr/local/bin/brew shellenv)\""
            fi
        fi
        echo ""
    fi

    echo "For more information, see:"
    echo "  - docs/Installation.md"
    echo "  - docs/Getting-Started.md"
    echo "  - docs/LLVM20-SETUP.md (macOS)"
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

    print_header "JFrame Development Environment Setup"

    local OS
    OS=$(detect_os)

    echo "Operating System: ${OS}"
    echo "Architecture: $(detect_arch)"
    echo "Script Directory: ${SCRIPT_DIR}"
    echo "vcpkg Directory: ${VCPKG_DIR}"
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

    if [ "$SKIP_BUILD" = false ]; then
        build_jframe
    else
        print_info "Skipping build (--no-build specified)"
    fi

    print_post_setup
}

main "$@"
