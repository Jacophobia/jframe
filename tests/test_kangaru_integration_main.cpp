// Main entry point for Kangaru + modules integration test
// Include kangaru header to ensure all symbols are available (MSVC workaround)
#include <kangaru/kangaru.hpp>

import test.kangaru_integration;

int main() {
    return kangaru_test::runTest();
}
