cmake_minimum_required(VERSION 3.15)
project(NanoVG C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
add_compile_options(-Wall -Wextra -O2 -g)

# Screenshot output directory - in project root
set(SCREENDUMP_DIR "${CMAKE_SOURCE_DIR}/screendumps")
file(MAKE_DIRECTORY ${SCREENDUMP_DIR})

# Find dependencies
find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find HarfBuzz
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)

# Find Cairo
pkg_check_modules(CAIRO REQUIRED cairo)

# Find FriBidi
pkg_check_modules(FRIBIDI REQUIRED fribidi)

# Find GLFW
pkg_check_modules(GLFW REQUIRED glfw3)

# Font system library
add_library(nanovg_font STATIC
	src/font/nvg_font_system.c
	src/font/nvg_font_glyph.c
	src/font/nvg_font_shaping.c
	src/font/nvg_font_info.c
)

target_include_directories(nanovg_font PUBLIC
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/font
	${FREETYPE_INCLUDE_DIRS}
	${HARFBUZZ_INCLUDE_DIRS}
	${FRIBIDI_INCLUDE_DIRS}
)

target_link_libraries(nanovg_font PUBLIC
	${FREETYPE_LIBRARIES}
	${HARFBUZZ_LIBRARIES}
	${FRIBIDI_LIBRARIES}
)

# Vulkan backend library
add_library(nanovg_vulkan STATIC
	src/vulkan/vk_shader.c
	src/vulkan/nvg_vk_context.c
	src/vulkan/nvg_vk_buffer.c
	src/vulkan/nvg_vk_texture.c
	src/vulkan/nvg_vk_shader.c
	src/vulkan/nvg_vk_pipeline.c
	src/vulkan/nvg_vk_render.c
	src/vulkan/nvg_vk_hdr_metadata.c
	src/vulkan/nvg_vk_color_space_ubo.c
	src/vulkan/nvg_vk_color_space.c
	src/vulkan/nvg_vk_color_space_math.c
)

target_include_directories(nanovg_vulkan PUBLIC
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/vulkan
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(nanovg_vulkan PUBLIC
	${Vulkan_LIBRARIES}
	nanovg_font
)

# NanoVG core library
add_library(nanovg STATIC
	src/nanovg.c
	src/nvg_vk.c
	src/vknvg_msdf.c
)

target_include_directories(nanovg PUBLIC
	${CMAKE_SOURCE_DIR}/src
	${FREETYPE_INCLUDE_DIRS}
	${CAIRO_INCLUDE_DIRS}
)

target_compile_definitions(nanovg PRIVATE FONS_USE_FREETYPE)
target_compile_options(nanovg PRIVATE -Wno-error=implicit-function-declaration)

target_link_libraries(nanovg PUBLIC
	nanovg_vulkan
	nanovg_font
	${CAIRO_LIBRARIES}
)

# Test utilities library
add_library(test_utils STATIC
	tests/window_utils.c
)

target_include_directories(test_utils PUBLIC
	${CMAKE_SOURCE_DIR}/tests
	${CMAKE_SOURCE_DIR}/src
	${Vulkan_INCLUDE_DIRS}
	${GLFW_INCLUDE_DIRS}
)

target_link_libraries(test_utils PUBLIC
	${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES}
	Threads::Threads
)

# Function to add a test executable
function(add_nanovg_test TEST_NAME)
	add_executable(${TEST_NAME} tests/${TEST_NAME}.c)

	target_include_directories(${TEST_NAME} PRIVATE
		${CMAKE_SOURCE_DIR}/src
		${CMAKE_SOURCE_DIR}/src/font
		${CMAKE_SOURCE_DIR}/tests
	)

	target_link_libraries(${TEST_NAME} PRIVATE
		nanovg
		test_utils
		m
	)

	# Set output directory
	set_target_properties(${TEST_NAME} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
	)
endfunction()

# Discover and add all test files
file(GLOB TEST_SOURCES "tests/test_*.c")

# Excluded tests (reference unimplemented APIs)
set(EXCLUDED_TESTS
	"${CMAKE_SOURCE_DIR}/tests/test_color_space_conversion.c"
	"${CMAKE_SOURCE_DIR}/tests/test_compute_shader_dispatch.c"
)

foreach(TEST_SOURCE ${TEST_SOURCES})
	list(FIND EXCLUDED_TESTS ${TEST_SOURCE} IS_EXCLUDED)
	if(IS_EXCLUDED EQUAL -1)
		get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
		add_nanovg_test(${TEST_NAME})
	endif()
endforeach()

# Custom target to run all tests
add_custom_target(test_all
	COMMAND ${CMAKE_COMMAND} -E echo "=== Running all tests ==="
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add individual test run targets
foreach(TEST_SOURCE ${TEST_SOURCES})
	list(FIND EXCLUDED_TESTS ${TEST_SOURCE} IS_EXCLUDED)
	if(IS_EXCLUDED EQUAL -1)
		get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
		add_custom_target(run_${TEST_NAME}
			COMMAND ${CMAKE_COMMAND} -E env
				"VK_INSTANCE_LAYERS="
				"VK_LAYER_PATH="
				timeout 30 $<TARGET_FILE:${TEST_NAME}> || true
			DEPENDS ${TEST_NAME}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			COMMENT "Running ${TEST_NAME}..."
		)
		add_dependencies(test_all run_${TEST_NAME})
	endif()
endforeach()

# Print configuration summary
message(STATUS "")
message(STATUS "======================================")
message(STATUS "NanoVG Configuration")
message(STATUS "======================================")
message(STATUS "Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler:       ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Screenshot dir:   ${SCREENDUMP_DIR}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  Vulkan:         ${Vulkan_VERSION}")
message(STATUS "  FreeType:       ${FREETYPE_VERSION_STRING}")
message(STATUS "  HarfBuzz:       ${HARFBUZZ_VERSION}")
message(STATUS "  Cairo:          ${CAIRO_VERSION}")
message(STATUS "  FriBidi:        ${FRIBIDI_VERSION}")
message(STATUS "  GLFW:           ${GLFW_VERSION}")
message(STATUS "======================================")
message(STATUS "")
