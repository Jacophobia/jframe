# LLVM 20 Setup Guide for Bestow

> **Last Updated:** 2025-01-25
> **Required For:** C++23 `import std;` support

---

## Overview

Bestow uses C++23 modules with `import std;` which requires LLVM Clang 20+. Apple Clang (Xcode) does not yet support `import std;`, so we use Homebrew's LLVM 20.

## Prerequisites

- macOS with Homebrew installed
- Xcode Command Line Tools (for macOS SDK headers)

## Installation

### 1. Install LLVM 20

```bash
brew install llvm@20
```

This installs to `/opt/homebrew/opt/llvm@20/` (Apple Silicon) or `/usr/local/opt/llvm@20/` (Intel).

### 2. Verify Installation

```bash
/opt/homebrew/opt/llvm@20/bin/clang++ --version
# Should show: clang version 20.x.x
```

### 3. Verify std.cppm Exists

```bash
ls /opt/homebrew/opt/llvm@20/share/libc++/v1/std.cppm
```

If this file doesn't exist, LLVM 20 wasn't installed with module support.

## How It Works

### CMake Configuration

The project uses `CMakePresets.json` to configure the LLVM 20 compiler:

```json
{
  "cacheVariables": {
    "CMAKE_C_COMPILER": "/opt/homebrew/opt/llvm@20/bin/clang",
    "CMAKE_CXX_COMPILER": "/opt/homebrew/opt/llvm@20/bin/clang++",
    "CMAKE_OSX_SYSROOT": "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
  }
}
```

**Important:** The `CMAKE_OSX_SYSROOT` is required because LLVM (unlike Apple Clang) doesn't automatically find macOS SDK headers.

### Standard Library Module Pre-compilation

The `cmake/StandardLibraryModules.cmake` file handles pre-compiling the standard library module:

1. Locates `std.cppm` in LLVM's libc++ installation
2. Pre-compiles it to `std.pcm` in the build directory
3. Provides `target_use_std_module()` function for targets

```cmake
# In any CMakeLists.txt
target_use_std_module(my_target)
```

This adds:
- `-fprebuilt-module-path=${CMAKE_BINARY_DIR}/pcm` to compiler flags
- Dependency on the `bestow-std-module` target

## Common Issues

### Issue: `'time.h' file not found`

**Cause:** LLVM can't find macOS SDK headers.

**Solution:** Ensure `CMAKE_OSX_SYSROOT` is set in CMakePresets.json:

```json
"CMAKE_OSX_SYSROOT": "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
```

If the SDK isn't at that path, find it with:
```bash
xcrun --show-sdk-path
```

### Issue: `failed to find module file for module 'std'`

**Cause:** Target not configured to use the pre-compiled std module.

**Solution:** Add `target_use_std_module(target_name)` in the target's CMakeLists.txt.

### Issue: `cannot add 'abi_tag' attribute in a redeclaration`

**Cause:** Mixing `#include <standard_header>` with `import std;` in the same translation unit or linked modules.

**Solution:** Ensure ALL source files that use standard library features use `import std;` consistently. Replace:

```cpp
// BAD - causes ABI conflicts
#include <memory>
#include <string>
import my_module;  // if my_module uses import std;
```

With:

```cpp
// GOOD - consistent module imports
import std;
import my_module;
```

### Issue: Pre-compiled module version mismatch

**Cause:** The `std.pcm` was compiled with a different compiler version.

**Solution:** Delete the build directory and rebuild:

```bash
rm -rf build/
cmake --preset macos-debug
cmake --build --preset macos-debug
```

## Module Usage Patterns

### In Module Interface Files (`.cppm`)

```cpp
// Global module fragment for third-party includes
module;

#include <third_party/header.hpp>

export module mymodule;

// Import standard library
import std;

export namespace myns {
    // Use std:: types freely
    std::string getName();
    std::vector<int> getNumbers();
}
```

### In Regular Source Files (`.cpp`)

```cpp
// At the top of the file, before any other code
import std;
import bestow;

int main() {
    std::cout << "Hello\n";
    return 0;
}
```

### Third-Party Headers

Third-party libraries that aren't module-aware must be included in the **global module fragment** (before `export module`):

```cpp
module;

// These go BEFORE the module declaration
#include <entt/entity/registry.hpp>
#include <glm/vec2.hpp>
#include <box2d/box2d.h>

export module bestow.physics;

import std;  // Standard library via module

// Now use both std:: and third-party types
```

## Switching Back to Apple Clang

If Apple Clang gains `import std;` support in a future Xcode release:

1. Update `CMakePresets.json`:
   ```json
   "CMAKE_C_COMPILER": "clang",
   "CMAKE_CXX_COMPILER": "clang++"
   ```

2. Remove `CMAKE_OSX_SYSROOT` (Apple Clang finds it automatically)

3. Update `cmake/StandardLibraryModules.cmake` to find Apple's `std.cppm` location

## Verification

After setup, verify everything works:

```bash
# Configure
cmake --preset macos-debug

# Build (should show "Pre-compiling C++ standard library module...")
cmake --build --preset macos-debug

# Test
ctest --preset macos-debug
```

All 65+ tests should pass.

## Troubleshooting Checklist

- [ ] LLVM 20 installed: `/opt/homebrew/opt/llvm@20/bin/clang++ --version`
- [ ] std.cppm exists: `ls /opt/homebrew/opt/llvm@20/share/libc++/v1/std.cppm`
- [ ] Xcode CLT installed: `xcode-select -p`
- [ ] SDK path valid: `ls /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk`
- [ ] Build directory clean: `rm -rf build/`
- [ ] No mixed includes: Check for `#include <standard_header>` in source files
