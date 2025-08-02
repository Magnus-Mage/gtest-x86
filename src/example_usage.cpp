// Example usage of the AsmTestFramework
// This demonstrates how to test x86 assembly executables

#include "x86_asm_test.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace x86_asm_test;
// Simple format replacement for older compilers
template<typename... Args>
std::string simple_format(const std::string& format_str, Args... args) {
    std::ostringstream oss;
    oss << format_str;
    ((oss << args), ...);
    return oss.str();
}

// Example 1: Basic test fixture for a simple calculator assembly program
class CalculatorAsmTest : public AsmTestFixture {
protected:
    void SetUp() override {
        // The program takes two integers and an operation as command line args
        // Usage: ./calc <num1> <num2> <operation>
        // Where operation is: add, sub, mul, div
        
        TestConfig config;
        config.timeout = std::chrono::milliseconds(3000); // 3 second timeout
        config.use_strace = false; // Disable strace for basic tests
        
        create_runner("./calc", AsmSyntax::Intel, config);
    }
};

// Test basic addition operation
TEST_F(CalculatorAsmTest, TestAddition) {
    auto input = make_input()
        .add_arg(10)        // First number
        .add_arg(5)         // Second number  
        .add_arg("add");    // Operation
    
    auto expected = expect_success()
        .stdout_equals("15\n");  // Expected result with newline
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Test multiplication with different input types
TEST_F(CalculatorAsmTest, TestMultiplication) {
    // Using array - single add_args call handles any range
    std::array<int, 2> numbers = {7, 8};
    
    auto input = make_input()
        .add_args(numbers)  // Ranges concept handles arrays, vectors, spans, etc.
        .add_arg("mul");
    
    auto expected = expect_success()
        .stdout_contains("56");  // More flexible - just check if result is present
    
    ASM_EXPECT_OUTPUT(get_runner(), input, expected);
}

// Test subtraction
TEST_F(CalculatorAsmTest, TestSubtraction) {
    auto input = make_input()
        .add_arg(10)
        .add_arg(3)
        .add_arg("sub");
    
    auto expected = expect_success()
        .stdout_equals("7\n");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Test division
TEST_F(CalculatorAsmTest, TestDivision) {
    auto input = make_input()
        .add_arg(20)
        .add_arg(4)
        .add_arg("div");
    
    auto expected = expect_success()
        .stdout_equals("5\n");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Test division by zero error handling
TEST_F(CalculatorAsmTest, TestDivisionByZero) {
    auto input = make_input()
        .add_arg(10)
        .add_arg(0)
        .add_arg("div");
    
    // Expect failure with specific exit code and error message
    auto expected = expect_failure(1)
        .stderr_contains("division by zero");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Test invalid arguments
TEST_F(CalculatorAsmTest, TestInvalidArguments) {
    auto input = make_input()
        .add_arg(10)
        .add_arg(5);  // Missing operation
    
    auto expected = expect_failure(1)
        .stderr_contains("Usage:");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Example 2: Testing a program that reads from stdin
class StringProcessorTest : public AsmTestFixture {
protected:
    void SetUp() override {
        // Assembly program that processes strings from stdin
        // Converts input to uppercase and outputs result
        TestConfig config;
        config.capture_stderr = true;
        
        create_runner("./string_processor", AsmSyntax::Intel, config);
    }
};

TEST_F(StringProcessorTest, TestUppercaseConversion) {
    auto input = make_input()
        .set_stdin("hello world\n");
    
    auto expected = expect_success()
        .stdout_contains("HELLO WORLD");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Test with simple string
TEST_F(StringProcessorTest, TestSimpleString) {
    auto input = make_input()
        .set_stdin("test");
    
    auto expected = expect_success()
        .stdout_contains("TEST");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Example 3: Parameterized tests for comprehensive input testing
class ParameterizedCalcTest : public CalculatorAsmTest, 
                            public ::testing::WithParamInterface<std::tuple<int, int, std::string, int>> {
};

TEST_P(ParameterizedCalcTest, TestOperations) {
    auto [num1, num2, operation, expected_result] = GetParam();
    
    auto input = make_input()
        .add_arg(num1)
        .add_arg(num2)
        .add_arg(operation);
    
    auto expected = expect_success()
        .stdout_equals(std::format("{}\n", expected_result));
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Test data for parameterized tests
INSTANTIATE_TEST_SUITE_P(
    BasicOperations,
    ParameterizedCalcTest,
    ::testing::Values(
        std::make_tuple(10, 5, "add", 15),
        std::make_tuple(10, 5, "sub", 5),
        std::make_tuple(10, 5, "mul", 50),
        std::make_tuple(10, 5, "div", 2),
        std::make_tuple(-5, 3, "add", -2),
        std::make_tuple(0, 100, "mul", 0),
        std::make_tuple(15, 3, "div", 5)
    )
);

// Example 4: Testing with container inputs
TEST_F(CalculatorAsmTest, TestWithContainerInputs) {
    // Using vector of arguments
    std::vector<std::string> operations = {"add", "sub", "mul"};
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    
    // Test multiple operations in sequence
    for (const auto& op : operations) {
        for (size_t i = 0; i < numbers.size() - 1; ++i) {
            auto input = make_input()
                .add_arg(numbers[i])
                .add_arg(numbers[i + 1])
                .add_arg(op);
            
            // Just verify it doesn't crash - flexible testing
            auto result = get_runner()->run_test(input);
            EXPECT_TRUE(result.succeeded() || result.exit_code == 1) 
                << std::format("Operation {} {} {} should either succeed or gracefully fail", 
                              numbers[i], op, numbers[i + 1]);
        }
    }
}

// Test negative numbers
TEST_F(CalculatorAsmTest, TestNegativeNumbers) {
    auto input = make_input()
        .add_arg(-10)
        .add_arg(5)
        .add_arg("add");
    
    auto expected = expect_success()
        .stdout_equals("-5\n");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Example main function for running tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // You can add custom test environment setup here
    std::cout << "Running Assembly Test Framework Examples\n";
    std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";
    
    return RUN_ALL_TESTS();
}
