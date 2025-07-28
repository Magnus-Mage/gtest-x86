// Example file only

#include <iostream>
#include <string>

// Simple utility function for your gtest-x86 library
class AsmTestHelper {
public:
    static std::string getLibraryName() {
        return "gtest-x86";
    }
    
    static bool isLibraryWorking() {
        return true;
    }
    
    // Example function that could be used for assembly testing
    static int addTwoNumbers(int a, int b) {
        return a + b;
    }
    
    // Placeholder for future assembly test utilities
    static void logTestInfo(const std::string& message) {
        std::cout << "[gtest-x86] " << message << std::endl;
    }
};

// Function that can be called from tests
extern "C" {
    int test_library_function() {
        return 42;
    }
}
