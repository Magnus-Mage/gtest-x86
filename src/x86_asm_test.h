#ifndef X86_ASM_TEST_H_
#define X86_ASM_TEST_H_

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <type_traits>
#include <functional>
#include <random>
#include <limits>

namespace x86_asm_test 
{
	
	/**
	 * @brief Forward Declarations; to get the pointer to the class
	 */
	class ExecutableRunner;
	class ProcessResult;
	class InputProvider;

	/**
	 * @brief Enum for assembly syntax types
	 */
	enum class AssemblySyntax 
	{
		INTEL,
		ATT
	};

	/**
	 * @brief ENUM for input/output datatypes (deprecated)
	 */
	enum class DataType 
	{
		INT8,
		INT16,
		INT32,
		INT64,
		UINT8,
		UINT16,
		UINT32,
		UINT64,
		FLOAT,
		DOUBLE,
		STRING,
		BINARY
	};

	/**
	 * @brief Type traits for compile-time information
	 */

	template<typename T>
	struct TypeInfo {
		static constexpr const char* name() 
		{
			if      constexpr (std::is_same_v<T, int8_t>)      return "int8" ;
			else if constexpr (std::is_same_v<T, int8_t>)      return "int8" ;
			else if constexpr (std::is_same_v<T, int16_t>)     return "int16";
			else if constexpr (std::is_same_v<T, int32_t>)     return "int32";
			else if constexpr (std::is_same_v<T, int64_t>)     return "int64";
			else if constexpr (std::is_same_v<T, uint8_t>)     return "uint8";
			else if constexpr (std::is_same_v<T, uint16_t>)    return "uint16";
			else if constexpr (std::is_same_v<T, uint32_t>)    return "uint32";
			else if constexpr (std::is_same_v<T, uint64_t>)    return "uint64";
			else if constexpr (std::is_same_v<T, float>)       return "float" ;
			else if constexpr (std::is_same_v<T, double>)      return "double";
			else if constexpr (std::is_same_v<T, std::string>) return "string";
			else						   return "binary";
		}

		static constexpr size_t size() 
	};

	struct TestInput {
		Datatype type;
		std::vector<uint8_t> data;
		std::string description;

		TestInput(DataType t, const std::vector<uint_8>& d, const std::string& desc = "")
			: type(t), data(d), description(desc) {}

		/**
		 * @brief constructors for different types
		 */ 
		static TestInput fromInt32(int_32_t value, const std::string& desc = "");
	};


}
