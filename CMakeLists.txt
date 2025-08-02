cmake_minimum_required(VERSION 3.20)
project(x86_asm_test_framework VERSION 1.0.0 LANGUAGES CXX ASM)

# Force C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check compiler version
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
        message(FATAL_ERROR "GCC 10+ required for C++20 support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
        message(WARNING "GCC 13+ recommended for full std::format support. Current version: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endif()

# Enable Assembly language
enable_language(ASM)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Assembly flags for x86-64
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --64")

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(src)

# Add GoogleTest (locally cloned)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
    add_subdirectory(googletest)
    set(GTEST_FOUND TRUE)
else()
    message(FATAL_ERROR "Google Test not found. Please ensure googletest is cloned in the repository root.")
endif()

# Create the x86 ASM test framework library
add_library(x86_asm_test_lib STATIC
    src/x86_asm_test.cpp
    src/x86_asm_test.h
)

target_include_directories(x86_asm_test_lib PUBLIC
    src
    ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include
)

target_compile_features(x86_asm_test_lib PUBLIC cxx_std_20)
target_link_libraries(x86_asm_test_lib PUBLIC gtest Threads::Threads)

# Assembly test programs
set(ASM_PROGRAMS calc string_processor)

foreach(PROGRAM ${ASM_PROGRAMS})
    set(ASM_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/test_programs/${PROGRAM}.s")
    set(ASM_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}.o")
    set(ASM_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}")
    
    if(EXISTS ${ASM_SOURCE})
        # Assemble
        add_custom_command(
            OUTPUT ${ASM_OBJECT}
            COMMAND as --64 -o ${ASM_OBJECT} ${ASM_SOURCE}
            DEPENDS ${ASM_SOURCE}
            COMMENT "Assembling ${PROGRAM}.s"
        )
        
        # Link
        add_custom_command(
            OUTPUT ${ASM_EXECUTABLE}
            COMMAND ld -o ${ASM_EXECUTABLE} ${ASM_OBJECT}
            DEPENDS ${ASM_OBJECT}
            COMMENT "Linking ${PROGRAM}"
        )
        
        # Create target
        add_custom_target(${PROGRAM}_target ALL DEPENDS ${ASM_EXECUTABLE})
    endif()
endforeach()

# Example usage executable
add_executable(asm_test_examples src/example_usage.cpp)
target_link_libraries(asm_test_examples PRIVATE x86_asm_test_lib gtest_main)
target_compile_features(asm_test_examples PRIVATE cxx_std_20)

# Make sure assembly programs are built before test executable
foreach(PROGRAM ${ASM_PROGRAMS})
    if(TARGET ${PROGRAM}_target)
        add_dependencies(asm_test_examples ${PROGRAM}_target)
    endif()
endforeach()

# Enable testing
enable_testing()
add_test(NAME x86_asm_framework_tests 
         COMMAND asm_test_examples
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Documentation with Doxygen (optional and clean)
find_package(Doxygen QUIET COMPONENTS dot)
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME "x86 ASM Test Framework")
    set(DOXYGEN_PROJECT_VERSION ${CMAKE_PROJECT_VERSION})
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
    set(DOXYGEN_INPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    
    doxygen_add_docs(docs
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        COMMENT "Generating documentation with Doxygen"
    )
endif()

# Install targets
install(TARGETS x86_asm_test_lib asm_test_examples
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES src/x86_asm_test.h DESTINATION include)

foreach(PROGRAM ${ASM_PROGRAMS})
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}")
        install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM} DESTINATION bin)
    endif()
endforeach()

# Configuration summary
message(STATUS "")
message(STATUS "x86 ASM Test Framework Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Assembly Programs: ${ASM_PROGRAMS}")
message(STATUS "  Google Test: ${GTEST_FOUND}")
message(STATUS "  Doxygen: ${DOXYGEN_FOUND}")
message(STATUS "")
