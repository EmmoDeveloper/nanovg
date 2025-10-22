# Building NanoVG Vulkan Backend

## Prerequisites

### Required
- **Vulkan SDK**: Download from [LunarG](https://vulkan.lunarg.com/)
- **NanoVG**: Core library (instructions below)
- **C Compiler**: GCC, Clang, or MSVC
- **Build System**: CMake 3.10+ or Make

### Installing Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get install vulkan-sdk libvulkan-dev cmake build-essential
```

#### macOS
```bash
brew install vulkan-headers vulkan-loader cmake
```

#### Windows
Download and install [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

## Getting NanoVG

Clone the NanoVG repository alongside this project:

```bash
cd /work/java/ai-ide-jvm
git clone https://github.com/memononen/nanovg.git
```

Your directory structure should be:
```
ai-ide-jvm/
├── nanovg/           ← NanoVG core library
└── nanovg-vulkan/    ← This project
```

## Build Methods

### Method 1: CMake (Recommended)

```bash
cd nanovg-vulkan
mkdir build && cd build
cmake ..
make
```

**Build options:**
```bash
# Use system-installed NanoVG
cmake -DNANOVG_VK_USE_SYSTEM_NANOVG=ON ..

# Skip building tests
cmake -DNANOVG_VK_BUILD_TESTS=OFF ..

# Specify NanoVG location
cmake -DNANOVG_DIR=/path/to/nanovg ..
```

### Method 2: Makefile

```bash
cd nanovg-vulkan
make
```

**Makefile options:**
```bash
# Custom NanoVG location
make NANOVG_DIR=/path/to/nanovg

# Custom Vulkan SDK
make VULKAN_SDK=/path/to/vulkan

# See all options
make help
```

## Testing

After building, run the test programs:

```bash
# With CMake
cd build
./test_compile
./test_simple

# With Makefile
./test_compile
./test_simple
```

## Using in Your Project

### Header-Only (Recommended)

```c
#include "nanovg.h"

#define NANOVG_VK_IMPLEMENTATION
#include "nanovg_vk.h"

// Your code here
```

### CMake Integration

```cmake
add_subdirectory(path/to/nanovg-vulkan)
target_link_libraries(your_app PRIVATE nanovg-vk nanovg)
```

### Manual Integration

1. Add `nanovg-vulkan/src` to your include path
2. Add `nanovg/src` to your include path
3. Link against: `-lvulkan -lm -lpthread`
4. Compile `nanovg.c` into your project

## Troubleshooting

### "NanoVG not found"
Ensure NanoVG is cloned in the correct location:
```bash
cd /work/java/ai-ide-jvm
git clone https://github.com/memononen/nanovg.git
```

### "Vulkan SDK not found"
Install the Vulkan SDK or specify its location:
```bash
export VULKAN_SDK=/path/to/vulkan
```

### "undefined reference to vk..."
Link against the Vulkan library: `-lvulkan`

## Clean Build

```bash
# CMake
cd build && make clean

# Makefile
make clean
```
