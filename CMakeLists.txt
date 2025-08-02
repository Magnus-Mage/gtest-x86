cmake_minimum_required(VERSION 3.20)
project(x86_asm_test_framework VERSION 1.0.0 LANGUAGES CXX ASM)

# Force C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force GCC 13 if available
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
        find_program(GCC13_COMPILER g++-13)
        if(GCC13_COMPILER)
            set(CMAKE_CXX_COMPILER ${GCC13_COMPILER})
            message(STATUS "Using GCC 13: ${CMAKE_CXX_COMPILER}")
        else()
            message(WARNING "GCC 13 not found, using ${CMAKE_CXX_COMPILER}")
        endif()
    endif()
endif()

# Enable Assembly language
enable_language(ASM)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Assembly flags
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} --64")

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(src)

# Add GoogleTest subdirectory (assuming it's cloned locally)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
    add_subdirectory(googletest)
    set(GTEST_FOUND TRUE)
else()
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(FATAL_ERROR "Google Test not found. Please run: git clone https://github.com/google/googletest.git")
    endif()
    set(GTEST_FOUND TRUE)
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

if(GTEST_FOUND)
    if(TARGET gtest)
        target_link_libraries(x86_asm_test_lib PUBLIC gtest)
    else()
        target_link_libraries(x86_asm_test_lib PUBLIC GTest::gtest)
    endif()
    target_link_libraries(x86_asm_test_lib PUBLIC Threads::Threads)
endif()

# Assembly test programs
set(ASM_PROGRAMS
    calc
    string_processor
)

foreach(PROGRAM ${ASM_PROGRAMS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_programs/${PROGRAM}.s")
        # Create object file
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}.o
            COMMAND ${CMAKE_ASM_COMPILER} ${CMAKE_ASM_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}.o ${CMAKE_CURRENT_SOURCE_DIR}/test_programs/${PROGRAM}.s
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_programs/${PROGRAM}.s
            COMMENT "Assembling ${PROGRAM}.s"
        )
        
        # Create executable
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}
            COMMAND ${CMAKE_LINKER} -o ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM} ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}.o
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}.o
            COMMENT "Linking ${PROGRAM}"
        )
        
        # Create a custom target for the program
        add_custom_target(${PROGRAM}_target ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}
        )
        
        # Make the executable available as a target
        add_executable(${PROGRAM} IMPORTED)
        set_target_properties(${PROGRAM} PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}
        )
        add_dependencies(${PROGRAM} ${PROGRAM}_target)
    endif()
endforeach()

# Example usage executable
add_executable(asm_test_examples
    src/example_usage.cpp
)

target_link_libraries(asm_test_examples PRIVATE
    x86_asm_test_lib
)

if(GTEST_FOUND)
    if(TARGET gtest_main)
        target_link_libraries(asm_test_examples PRIVATE gtest_main)
    else()
        target_link_libraries(asm_test_examples PRIVATE GTest::gtest_main)
    endif()
endif()

target_compile_features(asm_test_examples PRIVATE cxx_std_20)

# Copy assembly programs to build directory for testing
add_custom_target(copy_asm_programs ALL
    COMMENT "Copying assembly programs to build directory"
)

foreach(PROGRAM ${ASM_PROGRAMS})
    add_custom_command(TARGET copy_asm_programs POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}
        ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}
        COMMENT "Copying ${PROGRAM} to build directory"
    )
endforeach()

# Make sure assembly programs are built before the test executable
add_dependencies(asm_test_examples copy_asm_programs)
foreach(PROGRAM ${ASM_PROGRAMS})
    if(TARGET ${PROGRAM}_target)
        add_dependencies(copy_asm_programs ${PROGRAM}_target)
    endif()
endforeach()

# Enable testing
enable_testing()

# Add test that runs the example
add_test(NAME x86_asm_framework_tests 
         COMMAND asm_test_examples
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Set test environment
set_tests_properties(x86_asm_framework_tests PROPERTIES
    ENVIRONMENT "PATH=${CMAKE_CURRENT_BINARY_DIR}:$ENV{PATH}"
)

# Install targets (optional)
install(TARGETS x86_asm_test_lib asm_test_examples
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES src/x86_asm_test.h
    DESTINATION include
)

foreach(PROGRAM ${ASM_PROGRAMS})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM}
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endforeach()

# Documentation with Doxygen (optional)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    if(EXISTS ${DOXYGEN_IN})
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "x86 ASM Test Framework Configuration Summary:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Assembly Programs: ${ASM_PROGRAMS}")
message(STATUS "  Google Test Found: ${GTEST_FOUND}")
if(DOXYGEN_FOUND)
    message(STATUS "  Doxygen Found: YES")
else()
    message(STATUS "  Doxygen Found: NO")
endif()
message(STATUS "")
