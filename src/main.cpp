#include "asm_test_helper.h"
#include <iostream>

// Implementation of AsmTestHelper methods
std::string AsmTestHelper::getLibraryName() {
    return "gtest-x86";
}

bool AsmTestHelper::isLibraryWorking() {
    return true;
}

int AsmTestHelper::addTwoNumbers(int a, int b) {
    return a + b;
}

void AsmTestHelper::logTestInfo(const std::string& message) {
    std::cout << "[gtest-x86] " << message << std::endl;
}

// C function implementation
extern "C" {
    int test_library_function() {
        return 42;
    }
}
