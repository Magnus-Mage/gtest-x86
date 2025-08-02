/**
 * @file example_usage.cpp
 * @brief Example usage of the x86 ASM Test Framework
 * 
 * This file demonstrates how to test x86 assembly executables using
 * the framework with Google Test integration.
 */

#include "x86_asm_test.h"
#include <gtest/gtest.h>
#include <format>

using namespace x86_asm_test;

/**
 * @class CalculatorAsmTest
 * @brief Test fixture for a simple calculator assembly program
 * 
 * Tests a calculator program that takes two integers and an operation
 * as command line arguments: ./calc <num1> <num2> <operation>
 * Operations supported: add, sub, mul, div
 */
class CalculatorAsmTest : public AsmTestFixture {
protected:
    void SetUp() override {
        TestConfig config;
        config.timeout = std::chrono::milliseconds(3000);
        config.use_strace = false;
        
        create_runner("./calc", AsmSyntax::Intel, config);
    }
};

TEST_F(CalculatorAsmTest, TestAddition) {
    auto input = make_input()
        .add_arg(10)
        .add_arg(5)
        .add_arg("add");
    
    auto expected = expect_success()
        .stdout_equals("15\n");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

TEST_F(CalculatorAsmTest, TestMultiplication) {
    std::array<int, 2> numbers = {7, 8};
    
    auto input = make_input()
        .add_args(numbers)
        .add_arg("mul");
    
    auto expected = expect_success()
        .stdout_contains("56");
    
    ASM_EXPECT_OUTPUT(get_runner(), input, expected);
}

TEST_F(CalculatorAsmTest, TestSubtraction) {
    auto input = make_input()
        .add_arg(10)
        .add_arg(3)
        .add_arg("sub");
    
    auto expected = expect_success()
        .stdout_equals("7\n");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

TEST_F(CalculatorAsmTest, TestDivision) {
    auto input = make_input()
        .add_arg(20)
        .add_arg(4)
        .add_arg("div");
    
    auto expected = expect_success()
        .stdout_equals("5\n");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

TEST_F(CalculatorAsmTest, TestDivisionByZero) {
    auto input = make_input()
        .add_arg(10)
        .add_arg(0)
        .add_arg("div");
    
    auto expected = expect_failure(1)
        .stderr_contains("division by zero");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

TEST_F(CalculatorAsmTest, TestInvalidArguments) {
    auto input = make_input()
        .add_arg(10)
        .add_arg(5);  // Missing operation
    
    auto expected = expect_failure(1)
        .stderr_contains("Usage:");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

/**
 * @class StringProcessorTest
 * @brief Test fixture for a string processing assembly program
 * 
 * Tests a program that reads from stdin and converts input to uppercase
 */
class StringProcessorTest : public AsmTestFixture {
protected:
    void SetUp() override {
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

TEST_F(StringProcessorTest, TestSimpleString) {
    auto input = make_input()
        .set_stdin("test");
    
    auto expected = expect_success()
        .stdout_contains("TEST");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

/**
 * @class ParameterizedCalcTest
 * @brief Parameterized tests for comprehensive calculator testing
 */
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

TEST_F(CalculatorAsmTest, TestWithContainerInputs) {
    std::vector<std::string> operations = {"add", "sub", "mul"};
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    
    // Test multiple operations in sequence
    for (const auto& op : operations) {
        for (size_t i = 0; i < numbers.size() - 1; ++i) {
            auto input = make_input()
                .add_arg(numbers[i])
                .add_arg(numbers[i + 1])
                .add_arg(op);
            
            auto result = get_runner()->run_test(input);
            EXPECT_TRUE(result.succeeded() || result.exit_code == 1) 
                << std::format("Operation {} {} {} should either succeed or gracefully fail", 
                              numbers[i], op, numbers[i + 1]);
        }
    }
}

TEST_F(CalculatorAsmTest, TestNegativeNumbers) {
    auto input = make_input()
        .add_arg(-10)
        .add_arg(5)
        .add_arg("add");
    
    auto expected = expect_success()
        .stdout_equals("-5\n");
    
    ASM_ASSERT_OUTPUT(get_runner(), input, expected);
}

/**
 * @brief Main function for running the test suite
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "Running x86 Assembly Test Framework Examples\n";
    std::cout << std::format("Current working directory: {}\n", std::filesystem::current_path().string());
    
    return RUN_ALL_TESTS();
}
