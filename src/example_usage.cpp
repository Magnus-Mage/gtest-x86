// Example usage of the AsmTestFramework
// This demonstrates how to test x86 assembly executables

#include "x86_asm_test.h"
#include <gtest/gtest.h>

using namespace x86_asm_test;

// Example 1: Basic test fixture for a simple calculator assembly program
class CalculatorAsmTest : public AsmTestFixture {
protected:
    void SetUp() override {
        // Assume we have a compiled assembly program that does basic arithmetic
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
    // Using array and ranges - single add_args call handles any range
    std::array<int, 2> numbers = {7, 8};
    
    auto input = make_input()
        .add_args(numbers)  // Ranges concept handles arrays, vectors, spans, etc.
        .add_arg("mul");
    
    auto expected = expect_success()
        .stdout_contains("56");  // More flexible - just check if result is present
    
    ASM_EXPECT_OUTPUT(get_runner(), input, expected);
}

// Test division by zero error handling
TEST_F(CalculatorAsmTest, TestDivisionByZero) {
    auto input = make_input()
        .add_arg(10)
        .add_arg(0)
        .add_arg("div");
    
    // Expect failure with specific exit code and error message
    auto expected = expect_failure(1)
        .stderr_contains("division by zero")
        .stderr_contains("error");
    
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
        config.working_directory = std::filesystem::current_path() / "bin";
        
        create_runner("string_processor", AsmSyntax::ATT, config);
    }
};

TEST_F(StringProcessorTest, TestUppercaseConversion) {
    auto input = make_input()
        .set_stdin("hello world\ntest string\n");
    
    auto expected = expect_success()
        .stdout_equals("HELLO WORLD\nTEST STRING\n");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

// Test with binary input data
TEST_F(StringProcessorTest, TestBinaryData) {
    std::string binary_input{'\x41', '\x42', '\x43', '\x00', '\x44'}; // ABC\0D
    
    auto input = make_input()
        .set_stdin(binary_input);
    
    // Check that it handles null bytes correctly
    auto expected = expect_success()
        .stdout_contains("ABC");
    
    ASM_EXPECT_OUTPUT(get_runner(), input, expected);
}

// Example 3: Performance testing with timing
class PerformanceTest : public AsmTestFixture {
protected:
    void SetUp() override {
        // Assembly program that performs intensive computation
        TestConfig config;
        config.timeout = std::chrono::milliseconds(10000); // 10 second timeout for perf tests
        
        create_runner("./performance_test", AsmSyntax::Intel, config);
    }
};

TEST_F(PerformanceTest, TestExecutionTime) {
    auto input = make_input()
        .add_arg(1000000); // Number of iterations
    
    auto result = get_runner()->run_test(input);
    
    // Custom assertions for performance
    EXPECT_TRUE(result.succeeded()) << "Performance test should complete successfully";
    EXPECT_LT(result.execution_time.count(), 5000) << "Should complete within 5 seconds";
    EXPECT_GT(result.execution_time.count(), 100) << "Should take at least 100ms for realistic test";
    
    // Check output format
    EXPECT_TRUE(result.has_output()) << "Should produce timing output";
}

// Example 4: Advanced testing with strace debugging
class DebuggingTest : public AsmTestFixture {
protected:
    void SetUp() override {
        TestConfig config;
        config.use_strace = true;  // Enable strace for debugging
        config.strace_options = {
            "-e", "trace=write,read,open,close,exit_group",
            "-f",  // Follow forks
            "-s", "1024"  // String limit
        };
        
        create_runner("./file_processor", AsmSyntax::Intel, config);
    }
};

TEST_F(DebuggingTest, TestFileOperations) {
    // Create a temporary test file
    std::filesystem::path temp_file = std::filesystem::temp_directory_path() / "test_input.txt";
    std::ofstream{temp_file} << "test content\n";
    
    auto input = make_input()
        .add_arg(temp_file.string());
    
    auto expected = expect_success()
        .stdout_contains("test content");
    
    // The strace output will be mixed with stderr, providing debugging info
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
    
    // Cleanup
    std::filesystem::remove(temp_file);
}

// Example 5: Testing different assembly syntaxes
class SyntaxComparisonTest : public ::testing::Test {
protected:
    std::unique_ptr<AsmTestRunner> intel_runner_;
    std::unique_ptr<AsmTestRunner> att_runner_;
    
    void SetUp() override {
        // Assume we have the same program compiled with different syntax preferences
        intel_runner_ = std::make_unique<AsmTestRunner>("./program_intel", AsmSyntax::Intel);
        att_runner_ = std::make_unique<AsmTestRunner>("./program_att", AsmSyntax::ATT);
    }
};

TEST_F(SyntaxComparisonTest, TestBothSyntaxesProduceSameResult) {
    auto input = make_input()
        .add_arg(42)
        .add_arg("process");
    
    auto expected = expect_success()
        .stdout_contains("result: 42");
    
    // Both versions should produce identical results
    if (intel_runner_->executable_exists()) {
        ASM_EXPECT_OUTPUT(intel_runner_.get(), input, expected);
    }
    
    if (att_runner_->executable_exists()) {
        ASM_EXPECT_OUTPUT(att_runner_.get(), input, expected);
    }
}

// Example 6: Parameterized tests for comprehensive input testing
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
        std::make_tuple(0, 100, "mul", 0)
    )
);

// Example 7: Testing with container inputs
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

// Example main function for running tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // You can add custom test environment setup here
    std::cout << "Running Assembly Test Framework Examples\n";
    std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";
    
    return RUN_ALL_TESTS();
}
