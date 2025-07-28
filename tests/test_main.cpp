#include <gtest/gtest.h>
#include "asm_test_helper.h"
#include <iostream>

// Basic test to verify gtest is working
TEST(BasicTest, GoogleTestIsWorking) {
    EXPECT_TRUE(true);
    EXPECT_EQ(1, 1);
}

// Test your library functions
TEST(LibraryTest, LibraryNameTest) {
    std::string name = AsmTestHelper::getLibraryName();
    EXPECT_EQ(name, "gtest-x86");
}

TEST(LibraryTest, LibraryStatusTest) {
    EXPECT_TRUE(AsmTestHelper::isLibraryWorking());
}

TEST(LibraryTest, BasicMathTest) {
    int result = AsmTestHelper::addTwoNumbers(5, 3);
    EXPECT_EQ(result, 8);
}

TEST(LibraryTest, CFunctionTest) {
    int result = test_library_function();
    EXPECT_EQ(result, 42);
}

// Test logging functionality
TEST(LibraryTest, LoggingTest) {
    // This will print to console - useful for verifying output
    testing::internal::CaptureStdout();
    AsmTestHelper::logTestInfo("Test message");
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(output.find("Test message") != std::string::npos);
}
