# x86 Assembly Test Framework

[![CI](https://github.com/Magnus-Mage/gtest-x86/workflows/CI/badge.svg)](https://github.com/Magnus-Mage/gtest-x86/actions)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg)](https://cmake.org/)

A modern C++20 testing framework designed specifically for testing x86 assembly programs with Google Test integration. This framework provides a comprehensive solution for automated testing of assembly executables with features like timeout handling, stdin/stdout capture, system call tracing, and flexible output matching.

## Features

- **Modern C++20 Design**: Leverages concepts, ranges, and other C++20 features
- **Google Test Integration**: Seamless integration with the Google Test framework
- **Flexible Input/Output Testing**: Support for command-line arguments and stdin data
- **Multiple Output Matching**: Exact matches, substring containment, and pattern matching
- **System Call Tracing**: Built-in strace support for debugging assembly programs
- **Timeout Protection**: Configurable execution timeouts to prevent hanging tests
- **Cross-Platform Support**: Works on Linux x86-64 systems
- **Intel & AT&T Syntax**: Support for both assembly syntax formats
- **Comprehensive Documentation**: Full Doxygen documentation with examples

## Requirements

### System Requirements
- **Operating System**: Linux x86-64
- **Compiler**: GCC 10+ (GCC 13+ recommended for full C++20 support)
- **CMake**: Version 3.20 or higher
- **Assembly Tools**: GNU Assembler (as) and Linker (ld)

### Dependencies
- **Google Test**: v1.14.0 (automatically cloned)
- **Standard Libraries**: C++20 standard library with filesystem, format, and ranges support

## Installation

### 1. Install System Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    gcc-13 \
    g++-13 \
    binutils \
    git
```

**CentOS/RHEL/Fedora:**
```bash
# For newer versions with dnf
sudo dnf install cmake gcc gcc-c++ binutils git

# For older versions with yum
sudo yum install cmake gcc gcc-c++ binutils git
```

### 2. Install Development Tools (Optional but Recommended)

**System Call Tracing:**
```bash
# Install system call tracing
sudo apt-get install strace       # Ubuntu/Debian
sudo dnf install strace           # Fedora
sudo yum install strace           # CentOS/RHEL
```

**Documentation Tools:**
```bash
# For generating documentation
sudo apt-get install doxygen graphviz    # Ubuntu/Debian
sudo dnf install doxygen graphviz        # Fedora
```

### 3. Clone and Build

```bash
# Clone the repository
git clone https://github.com/Magnus-Mage/gtest-x86.git
cd gtest-x86

# Clone Google Test dependency
git clone https://github.com/google/googletest.git
cd googletest && git checkout v1.14.0 && cd ..

# Create build directory and configure
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the framework and test programs
make -j$(nproc)

# Run the example tests
./asm_test_examples
```

## Quick Start

### Basic Usage Example

```cpp
#include "x86_asm_test.h"
#include <gtest/gtest.h>

using namespace x86_asm_test;

class CalculatorTest : public AsmTestFixture {
protected:
    void SetUp() override {
        TestConfig config;
        config.timeout = std::chrono::milliseconds(3000);
        create_runner("./calc", AsmSyntax::Intel, config);
    }
};

TEST_F(CalculatorTest, TestAddition) {
    auto input = make_input()
        .add_arg(10)
        .add_arg(5)
        .add_arg("add");
    
    auto expected = expect_success()
        .stdout_equals("15\n");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}
```

### Testing Assembly Programs

1. **Create your assembly program** (e.g., `calculator.s`):
```assembly
.intel_syntax noprefix
.global _start

.section .text
_start:
    # Your assembly code here
    mov rax, 60     # sys_exit
    mov rdi, 0      # exit code
    syscall
```

2. **Assemble and link**:
```bash
as --64 -o calculator.o calculator.s
ld -o calculator calculator.o
```

3. **Write tests**:
```cpp
TEST_F(YourTestFixture, TestYourProgram) {
    auto input = make_input().add_arg("test_argument");
    auto expected = expect_success().stdout_contains("expected_output");
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}
```

## Project Structure

```
gtest-x86/
├── .github/workflows/          # CI/CD pipeline
├── src/                        # Framework source code
│   ├── x86_asm_test.h         # Main header file
│   ├── x86_asm_test.cpp       # Implementation
│   └── example_usage.cpp      # Usage examples
├── test_programs/              # Sample assembly programs
│   ├── calc.s                 # Calculator example
│   └── string_processor.s     # String processing example
├── build/                      # Build directory (generated)
├── CMakeLists.txt             # Build configuration
├── Doxyfile                   # Documentation configuration
└── README.md                  # This file
```

## Configuration Options

### TestConfig Options

```cpp
TestConfig config;
config.timeout = std::chrono::milliseconds(5000);      // Execution timeout
config.capture_stderr = true;                          // Capture stderr output
config.use_strace = false;                             // Enable system call tracing
config.strace_options = {"-e", "trace=write,read"};    // Strace options
config.working_directory = "/path/to/workdir";         // Working directory
```

### Assembly Syntax Support

```cpp
// Intel syntax (default)
create_runner("./program", AsmSyntax::Intel, config);

// AT&T syntax
create_runner("./program", AsmSyntax::ATT, config);
```

## Testing Features

### Input Methods

```cpp
auto input = make_input()
    .add_arg("string_argument")     // String arguments
    .add_arg(42)                    // Numeric arguments
    .add_args(vector_of_args)       // Multiple arguments from container
    .set_stdin("input data");       // Stdin data
```

### Output Matching

```cpp
auto expected = expect_success()
    .stdout_equals("exact_match")           // Exact stdout match
    .stdout_contains("substring")           // Substring containment
    .stderr_contains("error_pattern")      // Stderr pattern matching
    .exit_code(0);                         // Expected exit code

// For failure cases
auto expected_fail = expect_failure(1)
    .stderr_contains("Error:");
```

### Advanced Testing

```cpp
// Parameterized tests
INSTANTIATE_TEST_SUITE_P(
    Operations,
    CalculatorTest,
    ::testing::Values(
        std::make_tuple(10, 5, "add", 15),
        std::make_tuple(10, 5, "sub", 5)
    )
);

// System call tracing
config.use_strace = true;
config.strace_options = {"-e", "trace=write,read,exit_group"};
```

## Documentation

### Generate Documentation

```bash
cd build
make docs
# Documentation will be available in build/docs/html/index.html
```

### View Documentation

```bash
# Open in default browser
xdg-open build/docs/html/index.html

# Or serve locally
cd build/docs/html
python3 -m http.server 8000
# Visit http://localhost:8000
```

## Debugging Assembly Programs

### Using Strace for System Call Debugging

```cpp
// Enable strace in your tests
TestConfig config;
config.use_strace = true;
config.strace_options = {"-e", "trace=write,read,exit_group", "-v"};
```

### Common Debugging Commands

```bash
# View assembly with objdump
objdump -d -M intel program

# Check program dependencies
ldd program

# Examine ELF structure
readelf -h program

# Monitor system calls
strace -e trace=write,read ./program
```

## CI/CD Integration

The framework includes GitHub Actions CI/CD pipeline that:

- Tests on multiple build configurations (Debug/Release)
- Runs static analysis (cppcheck, clang-tidy)
- Generates test coverage reports
- Builds documentation automatically
- Validates assembly program execution

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines

- Follow C++20 best practices
- Add comprehensive tests for new features
- Update documentation for API changes
- Ensure CI pipeline passes
- Use modern C++ features appropriately

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Google Test](https://github.com/google/googletest) - Testing framework
- [CMake](https://cmake.org/) - Build system
- [Doxygen](https://www.doxygen.nl/) - Documentation generation
- The GNU toolchain for assembly development
