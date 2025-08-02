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
#include <iomanip>

namespace x86_asm_test {

// Concepts for type safety and better error messages
template<typename T>
concept StringLike = std::convertible_to<T, std::string_view>;

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

// Assembly syntax format enumeration
enum class AsmSyntax : uint8_t {
    Intel,
    ATT
};

// Process execution result with comprehensive information
struct ExecutionResult {
    int exit_code{0};
    std::string stdout_output;
    std::string stderr_output;
    std::chrono::milliseconds execution_time{0};
    bool timed_out{false};
    
    [[nodiscard]] constexpr bool succeeded() const noexcept {
        return exit_code == 0 && !timed_out;
    }
    
    [[nodiscard]] bool has_output() const noexcept {
        return !stdout_output.empty();
    }
};

// Configuration for test execution
struct TestConfig {
    std::chrono::milliseconds timeout{5000}; // 5 second default timeout
    bool capture_stderr{true};
    bool use_strace{false}; // Enable strace debugging
    std::vector<std::string> strace_options{"-e", "trace=write,read,exit_group"};
    std::filesystem::path working_directory{std::filesystem::current_path()};
};

// Input wrapper that can handle various types
class TestInput {
private:
    std::vector<std::string> args_;
    std::optional<std::string> stdin_data_;

public:
    TestInput() = default;
    
    // Add single argument - perfect forwarding for efficiency
    template<StringLike T>
    TestInput& add_arg(T&& arg) {
        args_.emplace_back(std::forward<T>(arg));
        return *this;
    }
    
    // Add arithmetic argument with automatic conversion using stringstream
    template<Arithmetic T>
    TestInput& add_arg(T value) {
        // Use stringstream for all arithmetic types - compatible with older compilers
        std::ostringstream oss;
        oss << value;
        args_.emplace_back(oss.str());
        return *this;
    }
    
    // Add multiple arguments from any range (C++20 ranges concept)
    template<std::ranges::input_range R>
    TestInput& add_args(R&& range) {
        // Using ranges eliminates need for separate container/span overloads
        for (const auto& item : range) {
            add_arg(item);
        }
        return *this;
    }
    
    // Set stdin data for the process
    template<StringLike T>
    TestInput& set_stdin(T&& data) {
        stdin_data_ = std::forward<T>(data);
        return *this;
    }
    
    // Return args as span - more efficient, no copying, works with algorithms
    [[nodiscard]] std::span<const std::string> args() const noexcept { return args_; }
    [[nodiscard]] const std::optional<std::string>& stdin_data() const noexcept { return stdin_data_; }
    
    // Fluent interface support
    [[nodiscard]] bool empty() const noexcept { return args_.empty(); }
    [[nodiscard]] size_t size() const noexcept { return args_.size(); }
};

// Expected output matcher with flexible comparison
class ExpectedOutput {
private:
    std::optional<std::string> exact_stdout_;
    std::optional<std::string> exact_stderr_;
    std::vector<std::string> stdout_contains_;
    std::vector<std::string> stderr_contains_;
    std::optional<int> expected_exit_code_;
    
public:
    ExpectedOutput() = default;
    
    // Exact output matching
    template<StringLike T>
    ExpectedOutput& stdout_equals(T&& expected) {
        exact_stdout_ = std::forward<T>(expected);
        return *this;
    }
    
    template<StringLike T>
    ExpectedOutput& stderr_equals(T&& expected) {
        exact_stderr_ = std::forward<T>(expected);
        return *this;
    }
    
    // Partial matching - more flexible for asm output
    template<StringLike T>
    ExpectedOutput& stdout_contains(T&& pattern) {
        stdout_contains_.emplace_back(std::forward<T>(pattern));
        return *this;
    }
    
    template<StringLike T>
    ExpectedOutput& stderr_contains(T&& pattern) {
        stderr_contains_.emplace_back(std::forward<T>(pattern));
        return *this;
    }
    
    ExpectedOutput& exit_code(int code) noexcept {
        expected_exit_code_ = code;
        return *this;
    }
    
    // Internal validation method
    [[nodiscard]] bool matches(const ExecutionResult& result) const noexcept;
    [[nodiscard]] std::string get_mismatch_description(const ExecutionResult& result) const;
};

// Main testing class - RAII design with proper resource management
class AsmTestRunner {
private:
    std::filesystem::path executable_path_;
    AsmSyntax syntax_;
    TestConfig config_;
    
    // Private helper for process execution
    [[nodiscard]] ExecutionResult execute_process(
        std::span<const std::string> args,
        const std::optional<std::string>& stdin_data = std::nullopt
    ) const;
    
    // Strace integration for debugging
    [[nodiscard]] ExecutionResult execute_with_strace(
        std::span<const std::string> args,
        const std::optional<std::string>& stdin_data = std::nullopt
    ) const;

public:
    // Constructor with path validation
    explicit AsmTestRunner(
        std::filesystem::path executable_path,
        AsmSyntax syntax = AsmSyntax::Intel,
        TestConfig config = {}
    );
    
    // Deleted copy operations to prevent accidental copying of heavy objects
    AsmTestRunner(const AsmTestRunner&) = delete;
    AsmTestRunner& operator=(const AsmTestRunner&) = delete;
    
    // Move operations for efficient resource transfer
    AsmTestRunner(AsmTestRunner&&) noexcept = default;
    AsmTestRunner& operator=(AsmTestRunner&&) noexcept = default;
    
    ~AsmTestRunner() = default;
    
    // Main test execution method
    [[nodiscard]] ExecutionResult run_test(const TestInput& input) const;
    
    // Convenience method that combines execution and assertion
    void assert_output(const TestInput& input, const ExpectedOutput& expected) const;
    
    // Configuration getters/setters
    [[nodiscard]] const TestConfig& config() const noexcept { return config_; }
    void set_config(TestConfig new_config) noexcept { config_ = std::move(new_config); }
    
    [[nodiscard]] AsmSyntax syntax() const noexcept { return syntax_; }
    void set_syntax(AsmSyntax new_syntax) noexcept { syntax_ = new_syntax; }
    
    [[nodiscard]] const std::filesystem::path& executable_path() const noexcept { 
        return executable_path_; 
    }
    
    // Utility methods
    [[nodiscard]] bool executable_exists() const noexcept;
    [[nodiscard]] std::string get_syntax_string() const noexcept;
};

// RAII Test fixture for Google Test integration
class AsmTestFixture : public ::testing::Test {
protected:
    std::unique_ptr<AsmTestRunner> runner_;
    
    void SetUp() override {
        // Override in derived classes to set up the runner
    }
    
    void TearDown() override {
        // Cleanup is automatic due to RAII
        runner_.reset();
    }
    
public:
    // Factory method for creating runners - better than direct construction
    template<typename... Args>
    void create_runner(Args&&... args) {
        runner_ = std::make_unique<AsmTestRunner>(std::forward<Args>(args)...);
    }
    
    [[nodiscard]] AsmTestRunner* get_runner() const noexcept {
        return runner_.get();
    }
};

// Utility macros for common test patterns
#define ASM_EXPECT_OUTPUT(runner, input, expected) \
    do { \
        ASSERT_NE(runner, nullptr) << "Runner not initialized"; \
        EXPECT_NO_THROW((runner)->assert_output(input, expected)); \
    } while(0)

#define ASM_ASSERT_OUTPUT(runner, input, expected) \
    do { \
        ASSERT_NE(runner, nullptr) << "Runner not initialized"; \
        ASSERT_NO_THROW((runner)->assert_output(input, expected)); \
    } while(0)

// Factory functions for common use cases
[[nodiscard]] inline TestInput make_input() {
    return TestInput{};
}

[[nodiscard]] inline ExpectedOutput expect_success() {
    return ExpectedOutput{}.exit_code(0);
}

[[nodiscard]] inline ExpectedOutput expect_failure(int code = 1) {
    return ExpectedOutput{}.exit_code(code);
}

} // namespace x86_asm_test
