#ifndef ASM_TEST_HELPER_H
#define ASM_TEST_HELPER_H

#include <string>

class AsmTestHelper {
public:
    static std::string getLibraryName();
    static bool isLibraryWorking();
    static int addTwoNumbers(int a, int b);
    static void logTestInfo(const std::string& message);
};

// C function declaration
extern "C" {
    int test_library_function();
}

#endif // ASM_TEST_HELPER_H
