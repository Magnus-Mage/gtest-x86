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
	 * @brief Forward Declarations; to get the pointer to the class
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
		int exit_code{0};
		std::string stdout_output;
		std::string stderr_output;
		std::chrono::milliseconds execution_time{0};
		bool timed_out{false};

		[[nodiscard]] constexpr bool succeeded() const noexcept 
		{
			return exit_code == 0 && !timed_out;
		}

		[[nodiscard]] bool has_output() const noexcept 
		{
			return !stdout_output.empty();
		}
	};	






}
