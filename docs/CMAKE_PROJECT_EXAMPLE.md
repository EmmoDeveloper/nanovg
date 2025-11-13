# ✅ This setup gives you:

- A library target (my_library)
- A test executable linked against the library
- build, test, and clean-all targets

# 📂 Directory Structure

```
my_library/
├── CMakeLists.txt
├── include/
│   └── my_library.hpp
├── src/
│   └── my_library.cpp
└── tests/
    └── test_my_library.cpp
```

# 📄 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyLibrary LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Library ---
add_library(my_library src/my_library.cpp)
target_include_directories(my_library PUBLIC include)

# --- Tests ---
enable_testing()
add_executable(test_my_library tests/test_my_library.cpp)
target_link_libraries(test_my_library PRIVATE my_library)
add_test(NAME MyLibraryTest COMMAND test_my_library)

# --- Custom clean target ---
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/*
    COMMENT "Cleaning all build files"
)
```

# 📄 include/my_library.hpp

```cpp
#pragma once

int add(int a, int b);
```

# 📄 src/my_library.cpp

```cpp
#include "my_library.hpp"

int add(int a, int b) {
    return a + b;
}
```

# 📄 tests/test_my_library.cpp

```cpp
#include "my_library.hpp"
#include <iostream>

int main() {
    std::cout << "Running library tests..." << std::endl;

    if (add(2, 3) == 5) {
        std::cout << "Test passed!" << std::endl;
        return 0;
    } else {
        std::cout << "Test failed!" << std::endl;
        return 1;
    }
}
```

# ⚙️ Usage

```sh
# Configure
cmake -S . -B build

# Build library + tests
cmake --build build

# Run tests
ctest --test-dir build

# Clean build directory
cmake --build build --target clean-all
```

```sh
# Configure
cmake -S . -B build

# Build library + tests
cmake --build build

# Run tests
ctest --test-dir build

# Clean build directory
cmake --build build --target clean-all
```
