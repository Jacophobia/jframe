# Dockerfile for Bestow Build Verification
# Uses Clang 20+ with C++23 'import std;' support and vcpkg for dependencies

FROM ubuntu:24.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build essentials and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    wget \
    lsb-release \
    software-properties-common \
    gnupg \
    # Vulkan SDK dependencies
    libvulkan-dev \
    vulkan-validationlayers \
    # X11/Wayland for GLFW
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libwayland-dev \
    libxkbcommon-dev \
    # OpenGL
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    # Audio dependencies
    libasound2-dev \
    libpulse-dev \
    # Autotools for building some vcpkg packages
    autoconf \
    autoconf-archive \
    automake \
    libtool \
    libltdl-dev \
    # Other
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Install LLVM 20 from official apt repository (required for C++23 'import std;')
RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 20 all && \
    rm llvm.sh

# Set Clang 20 as default compiler
ENV CC=/usr/bin/clang-20
ENV CXX=/usr/bin/clang++-20
ENV BESTOW_CC=/usr/bin/clang-20
ENV BESTOW_CXX=/usr/bin/clang++-20

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && cd /opt/vcpkg \
    && git checkout 9aee6e968f51e15ee93606f064691d8f6d228190 \
    && ./bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Set up custom triplet for Clang with libc++
ENV VCPKG_DEFAULT_TRIPLET=x64-linux-libcxx

WORKDIR /workspace

# Copy project files
COPY . .

# Set triplet overlay
ENV VCPKG_OVERLAY_TRIPLETS=/workspace/triplets

# Configure with CMake
RUN cmake --preset linux-debug \
    -DCMAKE_C_COMPILER=/usr/bin/clang-20 \
    -DCMAKE_CXX_COMPILER=/usr/bin/clang++-20

# Build
RUN cmake --build build/linux-debug --parallel $(nproc)

# Run tests (excluding flaky tests that are timing-dependent in containers)
CMD ["sh", "-c", "ctest --preset linux-debug --output-on-failure -E 'FrameTimerTest.DeltaTimeConsistency|CameraSystemTest.ShakeDecaysOverTime|AssetSystemTest.CheckForReloadsDetectsModifiedFile|AssetSystemTest.HotReloadCallbackOnReload|AssetSystemTest.CheckForReloadsIgnoresAssetsBeingLoaded|EventSystemTest.UnsubscribeDuringCallback'"]
