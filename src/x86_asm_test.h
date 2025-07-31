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

	template<typename T>
	concept Arithmetic = std::is_arithmeric_v<T>;

	template<typename T>
	concept Container = requires(T t) 
	{
		t.begin();
		t.end();
		typename T::value_type;
	};

	/**
	 * @brief Enum for assembly syntax types
	 */
	enum class AssemblySyntax 
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
			if constexpr (std::is_floating_point_v<T>)
			{
				args_.emplace_back(std::format("{}", value));
			}
			else
			{
				args_.emplace_back(std::to_string(value));
			}
			return *this;
		}

		/**
		 * @brief Add multiple arguments from container
		 * @param Expects an array or container
		 */
		template<Container C>
		TestInput& add_args(const C& container)
		{
			for (const auto& item : container)
			{
				add_arg(item);
			}
			return *this;
		}

		/**
		 * @brief Add arguments from span
		 * @param Expects a std::span from C++ 20
		 */
		template<typename T>
		TestInput& add_args(std::span<const T> items)
		{
			for ( const auto& item : items)
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

		[[nodiscard]] const std::vector<std::string>& args() const noexcept { return args_; }
		[[nodiscard]] const std::optional<std::string>& stdin_data() const noexcept { return stdin_data_; }
		

		/**
		 * @brief Some utils for better support
		 */
		[[nodiscard]] bool empty() const noexcept { return args_.empty(); }
		[[nodiscard]] size_t size() const noexcept { return args_.size(); }

	};

}
