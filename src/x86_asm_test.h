#pragma once

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <concepts>
#include <memory>
#include <chrono>
#include <optional>
#include <span>
#include <cstdint>
#include <type_traits>
#include <format>
#include <ranges>

namespace x86_asm_test 
{
	

	/**
	 * @brief Forward Declarations; to get the pointer to the class  (DECREPTED)
	 */
	class ExecutableRunner;
	class ProcessResult;
	class InputProvider;

	/**
	 * @brief Type safety and better error messages
	 */
	template<typename T>
        concept StringLike = std::convertible_to<T, std::string_view>;
	
	template<typename T>
	concept Arithmetic = std::is_arithmetic_v<T>;

	template<typename T>					// (Decrepted)
	concept Container = requires(T t) 
	{
		t.begin();
		t.end();
		typename T::value_type;
	};

	/**
	 * @brief Enum for assembly syntax types
	 */
	enum class AssemblySyntax : uint8_t
	{
		INTEL,
		ATT
	};

	/**
	 * @brief  Process execution result with log info
	 */
       	struct ExecutionResult 
	{
		int 				exit_code	{0};
		std::string 			stdout_output;
		std::string 			stderr_output;
		std::chrono::milliseconds 	execution_time	{0};
		bool 				timed_out	{false};

		[[nodiscard]] constexpr bool succeeded() const noexcept 
		{
			return exit_code == 0 && !timed_out;
		}

		[[nodiscard]] bool has_output() const noexcept 
		{
			return !stdout_output.empty();
		}
	};	

	/**
	 * @brief Settings for test execution 
	 */
	struct TestConfig
	{
		std::chrono::milliseconds timeout           {5000};      		// 5 seconds cd
		bool 			  capture_stderr    {true};
		bool 			  use_strace        {false};			// Enable strace debugging
		std::vector<std::string>  strace_options    {"-e", "trace=write, read, exit_group"};
		std::filesystem::path     working_directory {std::filesystem::current_path()};
	};

	
	/**
	 * @brief Input Wrapper that can handles diff types
	 */
	class TestInput
	{
	private:
		std::vector<std::string>   args_;
		std::optional<std::string> stdin_data_;
	
	public:
		TestInput() = default;
		
		/**
		 * @brief Add single argument
		 * @param expect a string 
		 */
		template<StringLike T>
		TestInput& add_arg(T&& arg)
		{
			args_.emplace_back(std::forward<T>(arg));
			return *this;
		}

		/**
		 * @brief Add arithmetic argument with auto conversion
		 * @param Expect a Integer
		 */
		template<Arithmetic T>
		TestInput& add_arg(T value)
		{	
			args_.emplace_back(std::format("{}", value));	
			return *this;
		}

		/**
		 * @brief Add multiple arguments from container
		 * @param Expects any container; no need for seperate container overloads
		 */
		template<std::ranges::input_range R>
		TestInput& add_args(R&& range)
		{
			for (const auto& item : range)
			{
				add_arg(item);
			}
			return *this;
		}

		/**
		 * @brief set stdin for reading data
		 * @param Data for testing
		 */
		template<StringLike T>
		TestInput& set_stdin(T&& data)
		{
			stdin_data_ = std::forward<T>(data);
			return *this;
		}

		[[nodiscard]] const std::span<const std::string>& args() const noexcept { return args_; }
		[[nodiscard]] const std::optional<std::string>& stdin_data() const noexcept { return stdin_data_; }
		

		/**
		 * @brief Some utils for better support
		 */
		[[nodiscard]] bool empty() const noexcept { return args_.empty(); }
		[[nodiscard]] size_t size() const noexcept { return args_.size(); }

	};
	
	/**
	 * @brief Expected output matcher
	 */
	class ExpectedOuput 
	{
	private:
		std::optional<std::string> 	exact_stdout_;
		std::optional<std::string>	exact_stderr_;
		std::vector<std::string>	stdout_contains_;
		std::vector<std::string>	stderr_contains_;
		std::optional<int>		expected_exit_code_;

	public:
		ExpectedOuput() = default;

		// Exact String Matcher
		template<StringLike T>
		ExpectedOutput& stdout_equals(T&& expected)
		{
			exact_stdout_ = std::forward<T>(expected);
			return *this;
		}

		template<StringLike T>
		ExpectedOuput& stderr_equals(T&& expected)
		{
			exact_stderr_ = std::forward<T>(expected);
			return *this;
		}

		// Partial matching for asm flexibility
		template<StringLike T>
		ExpectedOutput& stdout_contains(T&& pattern)
		{
			stdout_contains_.emplace_back(std::forward<T>(pattern));
			return *this;
		}

		template<StringLike T>
		ExpectedOutput& stderr_contains(T&& pattern)
		{
			stderr_contains_.emplace_back(std::forward<T>(pattern));
			return *this;
		}

		ExpectedOutput& exit_code(int code) noexcept
		{
			expected_exit_code_ = code;
			return *this;
		}

		// Internal validation method
		[[nodiscard]] bool matches(const ExecutionResult& result) const noexcept;
		[[nodiscard]] std::string get_mismatch_description(const ExecutionResult& result) const;
	};

	/**
	 * @brief Main testing class with RAII design pattern for proper resource management
	 */
	
	class AsmTestRunner
	{
	private:
		std::filesystem::path executable_path_;
		AsmSyntax syntax_;
		TestConfig config_;
		
		// Private helper for process execution
		[[nodiscard]] ExecutionResult execute_process(
			const std::vector<std::string>& args,
			const std::optional<std::string>& stdin_data = std::nullopt
			) const;

		// Strace integration for debugging
		[[nodiscard]] ExecutionResult execute_with_strace(
				const std::vector<std::string>& args,
				const std::optional<std::string>& stdin_data = std::nullopt
				) const;
		
	public:
		// constructor for path verification
		explicit AsmTestRunner(
				std::filesystem::path executable_path,
				AsmSyntax syntax = AsmSyntax::Intel,
				TestConfig config = {}
				);

		// Delete copy operations to prevent copying big objects
		AsmTestRunner(const AsmTestRunner&) = delete;
		AsmTestRunner& operator=(const AsmTestRunner&) = delete;

		// Move constructors for efficiency
		AsmTestRunner(AsmTestRunner&&) noexcept = default;i
		AsmTestRunner& operator=(AsmTestRunner&&) noexcept = default;

		~AsmTestRunner() = default;

		// Main test execution method
		[[nodiscard]] ExectutionResult run_test(const TestInput& input) const;
		

		// Convience method that combines execution and assertion
		void assert_output(const TestInput& input, const ExpectedOutput& expected) const;

		// Config getters/setters
		[[nodiscard]] const TestConfig& config() const noexcept { return config_; }
		void set_config(TestConfig new_config) noexcept { config_ = std::move(new_config); }

		[[nodiscard]] AsmSyntax syntax() const noexcept { return syntax_; }
		void set_syntax(AsmSyntax new_syntax) noexcept { syntax_ = new_syntax; }

		[[nodiscard]] const std::filesystem::path& executable_path() const noexcept { return executable_path_; }
		
		// Utility Mehtods
		[[nodiscard]] bool executable_exists() const noexcept;
		[[nodiscard]] std::string get_syntax_string() const noexcept;

	};

	/**
	 * @brief RAII test fixture for Google test integration
	 */
	class AsmTestFixture : public ::testing::Test
	{
	protected:
		std::unique_ptr<AsmTestRunner> runner_;

		void SetUp() override
		{
			// Override in derived classes to set up a runner
		}

		void TearDown() override
		{
			// Cleanup
			runner_.reset();
		}

	public:
		// Factory method for creating runners - just testing something i saw
		template<typename... Args>
		void create_runner(Args&&... args)
		{
			runner_ = std::make_unique<AsmTestRunner>(std::forward<Args>(args)...);
		}

		[[nodiscard]] AsmTestRunner* get_runner() const noexcept
		{
			return runner_.get();
		}

	};


}
