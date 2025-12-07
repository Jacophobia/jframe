// Test: Verify Kangaru works with C++20 modules
// This is a compatibility test before integrating into bestow-engine

#include <kangaru/kangaru.hpp>
#include <iostream>
#include <string_view>

// Simple interface
struct ILogger {
    virtual ~ILogger() = default;
    virtual void log(std::string_view msg) = 0;
};

// Implementation
struct ConsoleLogger : ILogger {
    void log(std::string_view msg) override {
        std::cout << "[LOG] " << msg << "\n";
    }
};

// Service definitions for Kangaru
struct ConsoleLoggerService : kgr::single_service<ConsoleLogger> {};

// A class that depends on ILogger
struct Application {
    ILogger& logger;

    void run() {
        logger.log("Application running!");
    }
};

// Service definition with dependency
struct ApplicationService : kgr::service<Application, kgr::dependency<ConsoleLoggerService>> {};

int main() {
    std::cout << "Testing Kangaru with C++20...\n";

    kgr::container container;

    // Resolve services
    auto& logger = container.service<ConsoleLoggerService>();
    logger.log("Logger created successfully");

    auto app = container.service<ApplicationService>();
    app.run();

    // Verify same instance (singleton)
    auto& logger2 = container.service<ConsoleLoggerService>();
    if (&logger == &logger2) {
        std::cout << "SUCCESS: Singleton pattern works!\n";
    } else {
        std::cout << "FAILURE: Singleton returned different instances!\n";
        return 1;
    }

    std::cout << "All Kangaru tests passed!\n";
    return 0;
}
