// Example file only
#include <gtest/gtest.h>
#include <cstdint>

// Forward declaration from main.cpp
class AsmTestHelper {
public:
    static void logTestInfo(const std::string& message);
};

// Simple inline assembly test (x86-64)
TEST(AssemblyTest, BasicInlineAssembly) {
    int input = 10;
    int output = 0;
    
    // Simple assembly: move input to output
    #ifdef __x86_64__
    asm volatile (
        "movl %1, %0"
        : "=r" (output)
        : "r" (input)
        : 
    );
    #else
    output = input; // Fallback for non-x86 platforms
    #endif
    
    EXPECT_EQ(output, input);
    AsmTestHelper::logTestInfo("Basic inline assembly test passed");
}

// Test assembly arithmetic
TEST(AssemblyTest, AssemblyAddition) {
    int a = 15;
    int b = 25;
    int result = 0;
    
    #ifdef __x86_64__
    asm volatile (
        "addl %2, %1\n\t"
        "movl %1, %0"
        : "=r" (result)
        : "r" (a), "r" (b)
        :
    );
    #else
    result = a + b; // Fallback
    #endif
    
    EXPECT_EQ(result, 40);
    AsmTestHelper::logTestInfo("Assembly addition test passed");
}

// Test register operations
TEST(AssemblyTest, RegisterOperations) {
    uint32_t value = 0x12345678;
    uint32_t result = 0;
    
    #ifdef __x86_64__
    asm volatile (
        "movl %1, %%eax\n\t"
        "movl %%eax, %0"
        : "=m" (result)
        : "r" (value)
        : "eax"
    );
    #else
    result = value; // Fallback
    #endif
    
    EXPECT_EQ(result, value);
    AsmTestHelper::logTestInfo("Register operations test passed");
}

// Test flags and conditions
TEST(AssemblyTest, FlagsAndConditions) {
    int a = 10;
    int b = 5;
    bool greater = false;
    
    #ifdef __x86_64__
    asm volatile (
        "cmpl %2, %1\n\t"
        "setg %0"
        : "=r" (greater)
        : "r" (a), "r" (b)
        : "cc"
    );
    #else
    greater = (a > b); // Fallback
    #endif
    
    EXPECT_TRUE(greater);
    AsmTestHelper::logTestInfo("Flags and conditions test passed");
}
