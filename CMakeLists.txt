cmake_minimum_required(VERSION 3.16)
project(gtest-x86)

# Gtest requires atleast C++17
set(CMAKE_CXX_STANDARD 20)

# Enable Assembly language
enable_language(ASM)

# Add compiler flags for debugging
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Include directories
include_directories(src)

# Source files for the emulator library
set(SOURCE
    src/main.cpp
)

# Create a library for the emulator core
add_library(s_lib ${SOURCE})

# Google Test setup
# Add GoogleTest subdirectory (already cloned locally)
add_subdirectory(googletest)


# Enable testing
enable_testing()

# Test executable
add_executable(
  asm_tests
  tests/test_asm.cpp
  tests/test_main.cpp
)

# Link test executable with Google Test and emulator library
target_link_libraries(
  asm_tests
  s_lib
  gtest_main
  gtest
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(asm_tests)

# Add coverage support for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    endif()
endif()

# Enable more warnings
if(MSVC)
    target_compile_options(s_lib PRIVATE /W4)
else()
    target_compile_options(s_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add version information
#set(PROJECT_VERSION_MAJOR 1)
#set(PROJECT_VERSION_MINOR 0)
#set(PROJECT_VERSION_PATCH 0)
#configure_file(
#    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
#    "${CMAKE_CURRENT_BINARY_DIR}/src/version.h"
#)
#target_include_directories(s_lib PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/src")

