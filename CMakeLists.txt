cmake_minimum_required(VERSION 3.10)
project(nanovg-vulkan VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(NANOVG_VK_BUILD_TESTS "Build test programs" ON)
option(NANOVG_VK_USE_SYSTEM_NANOVG "Use system-installed NanoVG instead of local copy" OFF)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Handle NanoVG dependency
if(NANOVG_VK_USE_SYSTEM_NANOVG)
	find_path(NANOVG_INCLUDE_DIR nanovg.h
		PATHS /usr/include /usr/local/include
		PATH_SUFFIXES nanovg)
	if(NOT NANOVG_INCLUDE_DIR)
		message(FATAL_ERROR "NanoVG not found. Install NanoVG or set NANOVG_VK_USE_SYSTEM_NANOVG=OFF")
	endif()
else()
	# Use local NanoVG (expect it in ../nanovg relative to this project)
	set(NANOVG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../nanovg")
	if(NOT EXISTS "${NANOVG_DIR}/src/nanovg.h")
		message(FATAL_ERROR
			"NanoVG not found at ${NANOVG_DIR}\n"
			"Please clone NanoVG:\n"
			"  cd ${CMAKE_CURRENT_SOURCE_DIR}/..\n"
			"  git clone https://github.com/memononen/nanovg.git\n"
			"Or install NanoVG system-wide and set NANOVG_VK_USE_SYSTEM_NANOVG=ON")
	endif()
	set(NANOVG_INCLUDE_DIR "${NANOVG_DIR}/src")

	# Build NanoVG library
	add_library(nanovg STATIC
		${NANOVG_DIR}/src/nanovg.c
	)
	target_include_directories(nanovg PUBLIC ${NANOVG_INCLUDE_DIR})
	target_compile_definitions(nanovg PRIVATE NANOVG_NO_STB)
endif()

# NanoVG Vulkan Backend - header-only library
add_library(nanovg-vk INTERFACE)
target_include_directories(nanovg-vk INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${NANOVG_INCLUDE_DIR}
)
target_link_libraries(nanovg-vk INTERFACE
	Vulkan::Vulkan
	m
	pthread
)

# Build test programs
if(NANOVG_VK_BUILD_TESTS)
	# Compile test
	add_executable(test_compile tests/test_compile.c)
	target_link_libraries(test_compile PRIVATE nanovg-vk nanovg)

	# Simple test
	add_executable(test_simple tests/test_simple.c)
	target_link_libraries(test_simple PRIVATE nanovg-vk nanovg)

	# Init test
	add_executable(test_init tests/test_init.c)
	target_link_libraries(test_init PRIVATE nanovg-vk nanovg)
endif()

# Installation
install(DIRECTORY src/
	DESTINATION include/nanovg-vk
	FILES_MATCHING PATTERN "*.h"
)

# Print configuration summary
message(STATUS "")
message(STATUS "NanoVG Vulkan Backend Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  NanoVG include: ${NANOVG_INCLUDE_DIR}")
message(STATUS "  Vulkan found: ${Vulkan_FOUND}")
message(STATUS "  Build tests: ${NANOVG_VK_BUILD_TESTS}")
message(STATUS "")
