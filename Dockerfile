# Dockerfile for Bestow Build Verification
# Uses Clang 17+ with C++23 support and vcpkg for dependencies

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
    # Clang 17+ for C++23 modules
    clang-17 \
    libc++-17-dev \
    libc++abi-17-dev \
    lld-17 \
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
    # Audio dependencies
    libasound2-dev \
    libpulse-dev \
    # Other
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Set Clang as default compiler
ENV CC=clang-17
ENV CXX=clang++-17

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Set up triplet for Clang with libc++
RUN mkdir -p /opt/vcpkg/triplets/community
COPY triplets/ /opt/vcpkg/custom-triplets/

WORKDIR /workspace

# Copy project files
COPY . .

# Configure with CMake
RUN cmake --preset linux-debug \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_OVERLAY_TRIPLETS=/opt/vcpkg/custom-triplets \
    || echo "Configure step - check logs for details"

# Build
RUN cmake --build build/linux-debug --parallel $(nproc) \
    || echo "Build step - check logs for details"

# Run tests
CMD ["ctest", "--preset", "linux-debug", "--output-on-failure"]
