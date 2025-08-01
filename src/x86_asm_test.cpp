// Function definations
#include "x86_asm_test.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <ranges>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

namespace x86_asm_test
{
	/**
	 * @brief ExpectedOutput Implementation
	 * @param Result of the execution - either succesful or denied
	 * @return bool to confirm the execution of the method 
	 */
	[[nodiscard]] bool ExpectedOutput::matches(const ExecutedResult& result) const noexcept 
	{
		// Check exit code if specified
		if (expected_exit_code_.has_value() && result.exit_code != expected_exit_code_.value())
		{
			return false;
		}

		// Check exact result for stdout match
		if (exact_stdout_.has_value() && result.stdout_output != exact_stdout_.value())
		{
			return false;
		}

		// Check exact stderr match
		if (exact_stderr_.has_value() && result.stderr_output != exact_stderr_.value())
		{
			return false;
		}

		// Check stdout contains patterns
		for (const auto& pattern : stdout_contains_)
		{
			if (result.stdout_output.find(pattern) == std::string::npos)
			{
				return false;
			}
		}

		// Check stderr contains patterns
		for (const auto& pattern : stderr_contains_)
		{
			if (result.stderr_output.find(pattern) == std::string::npos)
			{
				return false;
			}
		}

		return true;
	}

	/**
	 * @brief Get mismatch of strings as debug info
	 * @param Result of the execution
	 * @return string
	 */
	[[nodiscard]] std::string ExpectedOutput::get_mismatche_description(const ExecutionResult& result) const
	{
		std::ostringstream oss;

		if (expected_exit_code_.has_value() && result.exit_code != expected_exit_code_.value())
		{

		}
	}
}
