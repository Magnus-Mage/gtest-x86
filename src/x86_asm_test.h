/**
 * @file x86_asm_test.h
 * @brief A C++20 framework for testing x86 assembly programs with Google Test integration
 * @author Magnus-Mage
 * @version 1.0.0
 */

#pragma once

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <concepts>
#include <type_traits>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <chrono>
#include <ranges>
#include <sstream>

/**
 * @namespace x86_asm_test
 * @brief Main namespace for the x86 assembly testing framework
 */
namespace x86_asm_test {

/**
 * @concept StringLike
 * @brief Concept for types that can be converted to string_view
 */
template<typename T>
concept StringLike = std::convertible_to<T, std::string_view>;

/**
 * @concept Arithmetic
 * @brief Concept for arithmetic types (int, float, etc.)
 */
template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

/**
 * @enum AsmSyntax
 * @brief Assembly syntax format enumeration
 */
enum class AsmSyntax : uint8_t {
    Intel,  ///< Intel assembly syntax
    ATT     ///< AT&T assembly syntax
};

/**
 * @struct ExecutionResult
 * @brief Contains the results of executing an assembly program
 */
struct ExecutionResult {
    int exit_code{0};                                    ///< Process exit code
    std::string stdout_output;                           ///< Standard output content
    std::string stderr_output;                           ///< Standard error content
    std::chrono::milliseconds execution_time{0};         ///< Execution duration
    bool timed_out{false};                               ///< Whether execution timed out
    
    /**
     * @brief Check if the execution succeeded (exit code 0 and no timeout)
     * @return true if successful, false otherwise
     */
    [[nodiscard]] constexpr bool succeeded() const noexcept {
        return exit_code == 0 && !timed_out;
    }
    
    /**
     * @brief Check if the program produced any output
     * @return true if stdout is not empty
     */
    [[nodiscard]] bool has_output() const noexcept {
        return !stdout_output.empty();
    }
};

/**
 * @struct TestConfig
 * @brief Configuration options for test execution
 */
struct TestConfig {
    std::chrono::milliseconds timeout{5000};                                    ///< Execution timeout (default: 5s)
    bool capture_stderr{true};                                                   ///< Whether to capture stderr
    bool use_strace{false};                                                      ///< Enable strace debugging
    std::vector<std::string> strace_options{"-e", "trace=write,read,exit_group"}; ///< Strace command options
    std::filesystem::path working_directory{std::filesystem::current_path()};    ///< Working directory for execution
};

/**
 * @class TestInput
 * @brief Wrapper for test input data including arguments and stdin
 * 
 * This class provides a fluent interface for building test input with
 * command-line arguments and stdin data.
 */
class TestInput {
private:
    std::vector<std::string> args_;
    std::optional<std::string> stdin_data_;

public:
    TestInput() = default;
    
    /**
     * @brief Add a single string-like argument
     * @tparam T String-like type
     * @param arg The argument to add
     * @return Reference to this object for chaining
     */
    template<StringLike T>
    TestInput& add_arg(T&& arg) {
        args_.emplace_back(std::forward<T>(arg));
        return *this;
    }
    
    /**
     * @brief Add an arithmetic argument (automatically converted to string)
     * @tparam T Arithmetic type
     * @param value The numeric value to add
     * @return Reference to this object for chaining
     */
    template<Arithmetic T>
    TestInput& add_arg(T value) {
        std::ostringstream oss;
        oss << value;
        args_.emplace_back(oss.str());
        return *this;
    }
    
    /**
     * @brief Add multiple arguments from a range
     * @tparam R Range type
     * @param range Range of arguments to add
     * @return Reference to this object for chaining
     */
    template<std::ranges::input_range R>
    TestInput& add_args(R&& range) {
        for (const auto& item : range) {
            add_arg(item);
        }
        return *this;
    }
    
    /**
     * @brief Set stdin data for the process
     * @tparam T String-like type
     * @param data The data to send to stdin
     * @return Reference to this object for chaining
     */
    template<StringLike T>
    TestInput& set_stdin(T&& data) {
        stdin_data_ = std::forward<T>(data);
        return *this;
    }
    
    /**
     * @brief Get arguments as a span
     * @return Span view of the arguments
     */
    [[nodiscard]] std::span<const std::string> args() const noexcept { return args_; }
    
    /**
     * @brief Get stdin data
     * @return Optional stdin data
     */
    [[nodiscard]] const std::optional<std::string>& stdin_data() const noexcept { return stdin_data_; }
    
    /**
     * @brief Check if no arguments have been added
     * @return true if empty
     */
    [[nodiscard]] bool empty() const noexcept { return args_.empty(); }
    
    /**
     * @brief Get number of arguments
     * @return Number of arguments
     */
    [[nodiscard]] size_t size() const noexcept { return args_.size(); }
};

/**
 * @class ExpectedOutput
 * @brief Matcher for expected program output with flexible comparison options
 * 
 * This class allows you to specify expected output patterns using exact matches
 * or substring containment checks.
 */
class ExpectedOutput {
private:
    std::optional<std::string> exact_stdout_;
    std::optional<std::string> exact_stderr_;
    std::vector<std::string> stdout_contains_;
    std::vector<std::string> stderr_contains_;
    std::optional<int> expected_exit_code_;
    
public:
    ExpectedOutput() = default;
    
    /**
     * @brief Expect exact stdout match
     * @tparam T String-like type
     * @param expected Expected stdout content
     * @return Reference to this object for chaining
     */
    template<StringLike T>
    ExpectedOutput& stdout_equals(T&& expected) {
        exact_stdout_ = std::forward<T>(expected);
        return *this;
    }
    
    /**
     * @brief Expect exact stderr match
     * @tparam T String-like type
     * @param expected Expected stderr content
     * @return Reference to this object for chaining
     */
    template<StringLike T>
    ExpectedOutput& stderr_equals(T&& expected) {
        exact_stderr_ = std::forward<T>(expected);
        return *this;
    }
    
    /**
     * @brief Expect stdout to contain a pattern
     * @tparam T String-like type
     * @param pattern Pattern that should be present in stdout
     * @return Reference to this object for chaining
     */
    template<StringLike T>
    ExpectedOutput& stdout_contains(T&& pattern) {
        stdout_contains_.emplace_back(std::forward<T>(pattern));
        return *this;
    }
    
    /**
     * @brief Expect stderr to contain a pattern
     * @tparam T String-like type
     * @param pattern Pattern that should be present in stderr
     * @return Reference to this object for chaining
     */
    template<StringLike T>
    ExpectedOutput& stderr_contains(T&& pattern) {
        stderr_contains_.emplace_back(std::forward<T>(pattern));
        return *this;
    }
    
    /**
     * @brief Set expected exit code
     * @param code Expected exit code
     * @return Reference to this object for chaining
     */
    ExpectedOutput& exit_code(int code) noexcept {
        expected_exit_code_ = code;
        return *this;
    }
    
    /**
     * @brief Check if the actual result matches expectations
     * @param result The execution result to check
     * @return true if all expectations match
     */
    [[nodiscard]] bool matches(const ExecutionResult& result) const noexcept;
    
    /**
     * @brief Get a description of mismatches
     * @param result The execution result to compare against
     * @return String describing what didn't match
     */
    [[nodiscard]] std::string get_mismatch_description(const ExecutionResult& result) const;
};

/**
 * @class AsmTestRunner
 * @brief Main class for executing and testing assembly programs
 * 
 * This class handles the execution of assembly programs and provides
 * methods for testing their behavior. It uses RAII for resource management.
 */
class AsmTestRunner {
private:
    std::filesystem::path executable_path_;
    AsmSyntax syntax_;
    TestConfig config_;
    
    /**
     * @brief Execute process with regular system calls
     * @param args Command line arguments
     * @param stdin_data Optional stdin data
     * @return Execution result
     */
    [[nodiscard]] ExecutionResult execute_process(
        std::span<const std::string> args,
        const std::optional<std::string>& stdin_data = std::nullopt
    ) const;
    
    /**
     * @brief Execute process with strace for debugging
     * @param args Command line arguments
     * @param stdin_data Optional stdin data
     * @return Execution result
     */
    [[nodiscard]] ExecutionResult execute_with_strace(
        std::span<const std::string> args,
        const std::optional<std::string>& stdin_data = std::nullopt
    ) const;

public:
    /**
     * @brief Construct a test runner for an assembly executable
     * @param executable_path Path to the executable
     * @param syntax Assembly syntax used (default: Intel)
     * @param config Test configuration (default: empty config)
     * @throws std::runtime_error if executable doesn't exist or isn't executable
     */
    explicit AsmTestRunner(
        std::filesystem::path executable_path,
        AsmSyntax syntax = AsmSyntax::Intel,
        TestConfig config = {}
    );
    
    // Delete copy operations to prevent expensive copying
    AsmTestRunner(const AsmTestRunner&) = delete;
    AsmTestRunner& operator=(const AsmTestRunner&) = delete;
    
    // Move operations for efficient resource transfer
    AsmTestRunner(AsmTestRunner&&) noexcept = default;
    AsmTestRunner& operator=(AsmTestRunner&&) noexcept = default;
    
    ~AsmTestRunner() = default;
    
    /**
     * @brief Execute the assembly program with given input
     * @param input Test input containing arguments and stdin data
     * @return Execution result
     */
    [[nodiscard]] ExecutionResult run_test(const TestInput& input) const;
    
    /**
     * @brief Execute and assert that output matches expectations
     * @param input Test input
     * @param expected Expected output patterns
     * @throws Google Test assertion failure if expectations don't match
     */
    void assert_output(const TestInput& input, const ExpectedOutput& expected) const;
    
    /**
     * @brief Get current test configuration
     * @return Reference to current config
     */
    [[nodiscard]] const TestConfig& config() const noexcept { return config_; }
    
    /**
     * @brief Set new test configuration
     * @param new_config New configuration to use
     */
    void set_config(TestConfig new_config) noexcept { config_ = std::move(new_config); }
    
    /**
     * @brief Get current assembly syntax
     * @return Current syntax setting
     */
    [[nodiscard]] AsmSyntax syntax() const noexcept { return syntax_; }
    
    /**
     * @brief Set assembly syntax
     * @param new_syntax New syntax to use
     */
    void set_syntax(AsmSyntax new_syntax) noexcept { syntax_ = new_syntax; }
    
    /**
     * @brief Get path to the executable
     * @return Path to executable
     */
    [[nodiscard]] const std::filesystem::path& executable_path() const noexcept { 
        return executable_path_; 
    }
    
    /**
     * @brief Check if the executable file exists
     * @return true if executable exists and is a regular file
     */
    [[nodiscard]] bool executable_exists() const noexcept;
    
    /**
     * @brief Get string representation of current syntax
     * @return Syntax as string
     */
    [[nodiscard]] std::string get_syntax_string() const noexcept;
};

/**
 * @class AsmTestFixture
 * @brief Google Test fixture for assembly testing with RAII resource management
 * 
 * This class provides a convenient base for writing Google Test test cases
 * that test assembly programs. It handles runner lifecycle automatically.
 */
class AsmTestFixture : public ::testing::Test {
protected:
    std::unique_ptr<AsmTestRunner> runner_;
    
    /**
     * @brief Set up the test fixture (override in derived classes)
     */
    void SetUp() override {
        // Override in derived classes to set up the runner
    }
    
    /**
     * @brief Clean up the test fixture (automatic due to RAII)
     */
    void TearDown() override {
        runner_.reset();
    }
    
public:
    /**
     * @brief Create a new test runner with the given arguments
     * @tparam Args Argument types for AsmTestRunner constructor
     * @param args Arguments to forward to AsmTestRunner constructor
     */
    template<typename... Args>
    void create_runner(Args&&... args) {
        runner_ = std::make_unique<AsmTestRunner>(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Get pointer to the current test runner
     * @return Pointer to runner (may be null if not created)
     */
    [[nodiscard]] AsmTestRunner* get_runner() const noexcept {
        return runner_.get();
    }
};

/**
 * @def ASM_EXPECT_OUTPUT
 * @brief Google Test EXPECT macro for assembly output testing
 * @param runner Pointer to AsmTestRunner
 * @param input TestInput object
 * @param expected ExpectedOutput object
 */
#define ASM_EXPECT_OUTPUT(runner, input, expected) \
    do { \
        ASSERT_NE(runner, nullptr) << "Runner not initialized"; \
        EXPECT_NO_THROW((runner)->assert_output(input, expected)); \
    } while(0)

/**
 * @def ASM_ASSERT_OUTPUT
 * @brief Google Test ASSERT macro for assembly output testing
 * @param runner Pointer to AsmTestRunner
 * @param input TestInput object
 * @param expected ExpectedOutput object
 */
#define ASM_ASSERT_OUTPUT(runner, input, expected) \
    do { \
        ASSERT_NE(runner, nullptr) << "Runner not initialized"; \
        ASSERT_NO_THROW((runner)->assert_output(input, expected)); \
    } while(0)

/**
 * @brief Factory function to create an empty TestInput
 * @return New TestInput object
 */
[[nodiscard]] inline TestInput make_input() {
    return TestInput{};
}

/**
 * @brief Factory function to create ExpectedOutput for successful execution
 * @return ExpectedOutput configured for success (exit code 0)
 */
[[nodiscard]] inline ExpectedOutput expect_success() {
    return ExpectedOutput{}.exit_code(0);
}

/**
 * @brief Factory function to create ExpectedOutput for failed execution
 * @param code Expected exit code (default: 1)
 * @return ExpectedOutput configured for failure
 */
[[nodiscard]] inline ExpectedOutput expect_failure(int code = 1) {
    return ExpectedOutput{}.exit_code(code);
}

} // namespace x86_asm_test
