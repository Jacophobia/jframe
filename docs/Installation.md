# Installation Guide

This guide covers detailed setup instructions for JFrame on macOS, Windows, and Linux.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [macOS Setup](#macos-setup)
3. [Windows Setup](#windows-setup)
4. [Linux Setup](#linux-setup)
5. [FMOD Installation](#fmod-installation)
6. [Verifying Your Installation](#verifying-your-installation)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Tools (All Platforms)

| Tool | Minimum Version | Purpose |
|------|-----------------|---------|
| **CMake** | 3.28+ | Build system with C++23 module support |
| **vcpkg** | Latest | Dependency management |
| **Git** | Any | Version control and vcpkg |

### Compiler Requirements

JFrame requires a C++23-compliant compiler with full module support, including `import std;`.

| Platform | Compiler | Version | Notes |
|----------|----------|---------|-------|
| **macOS** | LLVM Clang | 20.0+ | **Required** - Apple Clang does NOT support `import std;` |
| **Windows** | MSVC | 19.38+ (VS 2022 17.8+) | Visual Studio 2022 with `/std:c++latest` |
| **Windows** | Clang-CL | 17.0+ | LLVM for Windows (alternative to MSVC) |
| **Linux** | GCC | 13.0+ | Standard package manager |
| **Linux** | Clang | 17.0+ | Standard package manager |

### Third-Party Dependencies

Most dependencies are installed automatically via vcpkg, but FMOD requires manual installation:

- **FMOD Core API** - Download from [fmod.com](https://www.fmod.com/download)
  - Free for indie developers and evaluation
  - Commercial license required for revenue over $200k/year
  - Version 2.02+ recommended

---

## macOS Setup

### Step 1: Install Xcode Command Line Tools

```bash
xcode-select --install
```

Verify installation:
```bash
xcode-select -p
# Should output: /Library/Developer/CommandLineTools
```

### Step 2: Install Homebrew

If you don't have Homebrew installed:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Verify installation:
```bash
brew --version
```

### Step 3: Install LLVM 20

**CRITICAL:** JFrame requires LLVM Clang 20+ for `import std;` support. Apple Clang (bundled with Xcode) does NOT support this feature.

```bash
# Install LLVM 20
brew install llvm@20

# Verify installation
/opt/homebrew/opt/llvm@20/bin/clang++ --version
# Should output: clang version 20.x.x
```

**Intel Mac users:** Replace `/opt/homebrew/` with `/usr/local/` in all paths.

Verify the standard library module exists:
```bash
ls /opt/homebrew/opt/llvm@20/share/libc++/v1/std.cppm
# Should exist
```

For detailed LLVM 20 setup and troubleshooting, see [LLVM20-SETUP.md](LLVM20-SETUP.md).

### Step 4: Install CMake and Ninja

```bash
brew install cmake ninja
```

Verify installation:
```bash
cmake --version  # Should be 3.28 or higher
ninja --version
```

### Step 5: Install vcpkg

```bash
# Clone vcpkg to your home directory
cd ~
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
./bootstrap-vcpkg.sh
```

**Optional:** Add vcpkg to your PATH in `~/.zshrc` or `~/.bash_profile`:
```bash
export PATH="$HOME/vcpkg:$PATH"
```

Verify installation:
```bash
~/vcpkg/vcpkg --version
```

### Step 6: Clone JFrame

```bash
git clone https://github.com/yourusername/jframe.git
cd jframe
```

### Step 7: Install FMOD

See [FMOD Installation](#fmod-installation) below.

### Step 8: Build JFrame

```bash
# Configure
cmake --preset macos-debug

# Build
cmake --build --preset macos-debug

# Test
ctest --preset macos-debug
```

If all tests pass, you're ready to start developing!

---

## Windows Setup

### Step 1: Install Visual Studio 2022

Download and install [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) (Community Edition is free).

During installation, select:
- **Workload:** Desktop development with C++
- **Individual Components:**
  - MSVC v143 or later
  - C++ CMake tools for Windows
  - C++23 standard library modules (experimental)

Verify installation:
```cmd
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl
```

You should see the MSVC compiler version (19.38+).

### Step 2: Install Git

Download and install [Git for Windows](https://git-scm.com/download/win).

Verify installation:
```cmd
git --version
```

### Step 3: Install vcpkg

```cmd
# Clone vcpkg to C:\
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat
```

Add vcpkg to your PATH (optional):
1. Open "Edit the system environment variables"
2. Click "Environment Variables"
3. Under "System variables", edit "Path"
4. Add `C:\vcpkg`

Verify installation:
```cmd
C:\vcpkg\vcpkg --version
```

### Step 4: Clone JFrame

```cmd
cd C:\Dev
git clone https://github.com/yourusername/jframe.git
cd jframe
```

### Step 5: Install FMOD

See [FMOD Installation](#fmod-installation) below.

### Step 6: Configure vcpkg Path

Edit `CMakePresets.json` to point to your vcpkg installation:

```json
"toolchainFile": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

Or set the environment variable:
```cmd
setx VCPKG_ROOT C:\vcpkg
```

### Step 7: Build JFrame

Open a **Developer Command Prompt for VS 2022**:

```cmd
# Configure
cmake --preset windows-debug

# Build
cmake --build --preset windows-debug

# Test
ctest --preset windows-debug
```

If all tests pass, you're ready to start developing!

---

## Linux Setup

### Step 1: Install Compiler

#### Ubuntu/Debian (GCC 13+)

```bash
sudo apt update
sudo apt install build-essential gcc-13 g++-13
```

Set GCC 13 as default:
```bash
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
```

Verify:
```bash
g++ --version  # Should show 13.x or higher
```

#### Ubuntu/Debian (Clang 17+)

```bash
sudo apt update
sudo apt install clang-17 libc++-17-dev libc++abi-17-dev
```

Verify:
```bash
clang++-17 --version  # Should show 17.x or higher
```

#### Fedora/RHEL

```bash
sudo dnf install gcc-c++ clang cmake ninja-build
```

#### Arch Linux

```bash
sudo pacman -S gcc clang cmake ninja
```

### Step 2: Install Build Tools

```bash
# Ubuntu/Debian
sudo apt install cmake ninja-build git pkg-config

# Fedora/RHEL
sudo dnf install cmake ninja-build git pkgconfig

# Arch
sudo pacman -S cmake ninja git pkgconfig
```

Verify:
```bash
cmake --version  # Should be 3.28+
ninja --version
```

### Step 3: Install System Libraries

```bash
# Ubuntu/Debian
sudo apt install \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libgl1-mesa-dev \
    libasound2-dev \
    libpulse-dev

# Fedora/RHEL
sudo dnf install \
    libX11-devel \
    libXrandr-devel \
    libXinerama-devel \
    libXcursor-devel \
    libXi-devel \
    mesa-libGL-devel \
    alsa-lib-devel \
    pulseaudio-libs-devel

# Arch
sudo pacman -S \
    libx11 \
    libxrandr \
    libxinerama \
    libxcursor \
    libxi \
    mesa \
    alsa-lib \
    libpulse
```

### Step 4: Install vcpkg

```bash
# Clone vcpkg to your home directory
cd ~
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
./bootstrap-vcpkg.sh
```

Add to PATH (add to `~/.bashrc` or `~/.zshrc`):
```bash
export PATH="$HOME/vcpkg:$PATH"
```

Verify:
```bash
~/vcpkg/vcpkg --version
```

### Step 5: Clone JFrame

```bash
git clone https://github.com/yourusername/jframe.git
cd jframe
```

### Step 6: Install FMOD

See [FMOD Installation](#fmod-installation) below.

### Step 7: Build JFrame

```bash
# Configure
cmake --preset linux-debug

# Build
cmake --build --preset linux-debug

# Test
ctest --preset linux-debug
```

If all tests pass, you're ready to start developing!

---

## FMOD Installation

FMOD Core API is required for audio functionality. It must be installed manually.

### Step 1: Download FMOD

1. Visit [fmod.com/download](https://www.fmod.com/download)
2. Sign in (create a free account if needed)
3. Download **FMOD Core API** for your platform:
   - macOS: FMOD Core API for Mac
   - Windows: FMOD Core API for Windows
   - Linux: FMOD Core API for Linux

### Step 2: Extract FMOD

Extract the downloaded archive to a temporary location.

### Step 3: Copy to JFrame

Create the `external/fmod/` directory in your JFrame project and copy the FMOD files:

#### macOS

```bash
cd /path/to/jframe
mkdir -p external/fmod
cp -r /path/to/downloaded/fmodstudioapi20223mac/api/core external/fmod/
```

Your structure should look like:
```
jframe/
└── external/
    └── fmod/
        └── core/
            ├── inc/
            │   ├── fmod.h
            │   ├── fmod.hpp
            │   ├── fmod_common.h
            │   └── ...
            └── lib/
                ├── libfmod.dylib
                └── ...
```

#### Windows

```cmd
cd C:\path\to\jframe
mkdir external\fmod
xcopy /E /I C:\path\to\downloaded\fmodstudioapi20223win\api\core external\fmod\core
```

Your structure should look like:
```
jframe\
└── external\
    └── fmod\
        └── core\
            ├── inc\
            │   ├── fmod.h
            │   ├── fmod.hpp
            │   └── ...
            └── lib\
                ├── x64\
                │   ├── fmod.dll
                │   └── fmod_vc.lib
                └── ...
```

#### Linux

```bash
cd /path/to/jframe
mkdir -p external/fmod
cp -r /path/to/downloaded/fmodstudioapi20223linux/api/core external/fmod/
```

Your structure should look like:
```
jframe/
└── external/
    └── fmod/
        └── core/
            ├── inc/
            │   ├── fmod.h
            │   ├── fmod.hpp
            │   └── ...
            └── lib/
                ├── x86_64/
                │   ├── libfmod.so
                │   └── ...
                └── ...
```

### Step 4: Verify FMOD Installation

Check that the required files exist:

```bash
# macOS/Linux
ls external/fmod/core/inc/fmod.h
ls external/fmod/core/lib/

# Windows
dir external\fmod\core\inc\fmod.h
dir external\fmod\core\lib\x64\
```

If these files exist, FMOD is installed correctly.

---

## Verifying Your Installation

After completing all installation steps, verify everything works:

### 1. Build JFrame

```bash
# macOS
cmake --preset macos-debug
cmake --build --preset macos-debug

# Windows
cmake --preset windows-debug
cmake --build --preset windows-debug

# Linux
cmake --preset linux-debug
cmake --build --preset linux-debug
```

Look for success messages:
- "Pre-compiling C++ standard library module..."
- "Building CXX object ..."
- "Linking CXX executable ..."

### 2. Run Tests

```bash
# macOS
ctest --preset macos-debug --output-on-failure

# Windows
ctest --preset windows-debug --output-on-failure

# Linux
ctest --preset linux-debug --output-on-failure
```

All tests should pass. If any fail, see [Troubleshooting](#troubleshooting).

### 3. Run Example Game

```bash
# macOS
./build/macos-debug/examples/platformer-demo/platformer-demo

# Windows
.\build\windows-debug\examples\platformer-demo\Debug\platformer-demo.exe

# Linux
./build/linux-debug/examples/platformer-demo/platformer-demo
```

You should see a window with a playable platformer game. Use arrow keys or WASD to move, Space to jump.

---

## Troubleshooting

### Build Errors

#### Error: "cannot find -lfmod"

**Cause:** FMOD not installed or in wrong location.

**Solution:** Verify FMOD installation:
```bash
# Should exist
ls external/fmod/core/inc/fmod.h
ls external/fmod/core/lib/
```

If missing, reinstall FMOD following [FMOD Installation](#fmod-installation).

#### Error: "failed to find module file for module 'std'"

**Cause:** Standard library module not pre-compiled or not found.

**Solution (macOS):**
```bash
# Verify LLVM 20 installed
/opt/homebrew/opt/llvm@20/bin/clang++ --version

# Verify std.cppm exists
ls /opt/homebrew/opt/llvm@20/share/libc++/v1/std.cppm

# Clean rebuild
rm -rf build/
cmake --preset macos-debug
cmake --build --preset macos-debug
```

**Solution (Windows):**
```cmd
# Clean rebuild
rmdir /s /q build
cmake --preset windows-debug
cmake --build --preset windows-debug
```

#### Error: "'time.h' file not found" (macOS only)

**Cause:** LLVM can't find macOS SDK headers.

**Solution:** Verify SDK path in CMakePresets.json:
```bash
# Find your SDK path
xcrun --show-sdk-path

# Update CMakePresets.json if different from:
# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
```

#### Error: "vcpkg.cmake not found"

**Cause:** vcpkg toolchain path incorrect in CMakePresets.json.

**Solution:** Update the toolchain path:
```json
// macOS/Linux
"toolchainFile": "$env{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake"

// Windows
"toolchainFile": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

Or set environment variable:
```bash
# macOS/Linux
export VCPKG_ROOT=~/vcpkg

# Windows
setx VCPKG_ROOT C:\vcpkg
```

### Runtime Errors

#### Error: "dyld: Library not loaded: @rpath/libfmod.dylib" (macOS)

**Cause:** FMOD dynamic library not in runtime path.

**Solution:** Copy FMOD library to build output:
```bash
cp external/fmod/core/lib/libfmod.dylib build/macos-debug/examples/platformer-demo/
```

Or add to `DYLD_LIBRARY_PATH`:
```bash
export DYLD_LIBRARY_PATH="$PWD/external/fmod/core/lib:$DYLD_LIBRARY_PATH"
./build/macos-debug/examples/platformer-demo/platformer-demo
```

#### Error: "fmod.dll not found" (Windows)

**Cause:** FMOD DLL not in executable directory or PATH.

**Solution:** Copy FMOD DLL to build output:
```cmd
copy external\fmod\core\lib\x64\fmod.dll build\windows-debug\examples\platformer-demo\Debug\
```

#### Error: "libfmod.so: cannot open shared object file" (Linux)

**Cause:** FMOD shared library not in library path.

**Solution:** Add to `LD_LIBRARY_PATH`:
```bash
export LD_LIBRARY_PATH="$PWD/external/fmod/core/lib/x86_64:$LD_LIBRARY_PATH"
./build/linux-debug/examples/platformer-demo/platformer-demo
```

### Compiler Version Issues

#### Using Apple Clang instead of LLVM Clang (macOS)

**Problem:** Build uses `/usr/bin/clang++` instead of `/opt/homebrew/opt/llvm@20/bin/clang++`

**Solution:** Ensure CMakePresets.json has correct compiler paths and clean rebuild:
```bash
rm -rf build/
cmake --preset macos-debug
```

Verify the compiler being used:
```bash
cmake --preset macos-debug 2>&1 | grep "The CXX compiler"
# Should show: /opt/homebrew/opt/llvm@20/bin/clang++
```

#### MSVC version too old (Windows)

**Problem:** MSVC version is below 19.38

**Solution:** Update Visual Studio 2022:
1. Open Visual Studio Installer
2. Click "Update" for Visual Studio 2022
3. Ensure you have version 17.8 or higher

### vcpkg Issues

#### vcpkg packages fail to install

**Cause:** Network issues or outdated vcpkg.

**Solution:** Update vcpkg and retry:
```bash
# macOS/Linux
cd ~/vcpkg
git pull
./bootstrap-vcpkg.sh

# Windows
cd C:\vcpkg
git pull
.\bootstrap-vcpkg.bat
```

Then clean and rebuild JFrame:
```bash
rm -rf build/
cmake --preset [your-preset]
cmake --build --preset [your-preset]
```

### CMake Version Issues

#### CMake version too old

**Problem:** CMake version below 3.28

**Solution (macOS):**
```bash
brew upgrade cmake
cmake --version  # Verify 3.28+
```

**Solution (Windows):**
Download latest CMake from [cmake.org/download](https://cmake.org/download/)

**Solution (Linux):**
```bash
# Ubuntu/Debian - may need to add Kitware APT repository
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
sudo apt update
sudo apt install cmake

# Or build from source
wget https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1.tar.gz
tar -xzf cmake-3.28.1.tar.gz
cd cmake-3.28.1
./bootstrap && make && sudo make install
```

---

## Next Steps

Once installation is complete and verified:

1. Read the [Getting Started Guide](Getting-Started.md) to build your first game
2. Study the [Technical Design](jframe-technical-design.md) to understand the architecture
3. Explore the example projects in `examples/`
4. Join the community on GitHub Discussions

---

## Getting Help

If you encounter issues not covered here:

1. Check [LLVM20-SETUP.md](LLVM20-SETUP.md) for compiler-specific issues
2. Search [GitHub Issues](https://github.com/yourusername/jframe/issues)
3. Ask in [GitHub Discussions](https://github.com/yourusername/jframe/discussions)
4. Join the Discord community (link TBD)

When asking for help, include:
- Your operating system and version
- Compiler version (`clang++ --version` or `cl` on Windows)
- CMake version (`cmake --version`)
- Full error message and stack trace
- Steps to reproduce the issue
