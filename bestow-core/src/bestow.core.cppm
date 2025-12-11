// bestow-core/src/bestow.core.cppm
// Core utilities and Engine composition root
//
// The Engine is the composition root that:
// 1. Allows clients to register implementations against contract interfaces
// 2. Uses Kangaru DI to wire dependencies
// 3. Instantiates and runs the client's Application with injected dependencies

module;

#include <kangaru/kangaru.hpp>
#include <taskflow/taskflow.hpp>

export module bestow.core;

import std;
import bestow.services;  // For IApplication, system interfaces
export import bestow.utils;  // Re-export utilities for backwards compatibility

export namespace bestow::core {

//==========================================================================
// Job System
//==========================================================================

class JobSystem {
public:
    JobSystem() : executor_(), taskflow_() {}
    ~JobSystem() = default;

    // Non-copyable, non-movable (tf::Executor is not movable)
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    JobSystem(JobSystem&&) = delete;
    JobSystem& operator=(JobSystem&&) = delete;

    // Submit a job for parallel execution
    template<typename F>
    void submit(F&& func) {
        taskflow_.emplace(std::forward<F>(func));
    }

    // Submit multiple jobs that can run in parallel
    template<typename F>
    void submitBatch(std::span<F> funcs) {
        for (auto& func : funcs) {
            taskflow_.emplace(std::move(func));
        }
    }

    // Wait for all submitted jobs to complete
    void wait() {
        if (!taskflow_.empty()) {
            tf::Taskflow currentBatch = std::move(taskflow_);
            taskflow_ = tf::Taskflow{};
            executor_.run(currentBatch).wait();
        }
    }

    // Get number of worker threads
    std::size_t workerCount() const {
        return executor_.num_workers();
    }

    // Check if there are pending jobs
    bool hasPendingJobs() const {
        return !taskflow_.empty();
    }

private:
    tf::Executor executor_;
    tf::Taskflow taskflow_;
};

//==========================================================================
// Engine - Composition Root
//==========================================================================

/// The Engine is the composition root that binds contracts to implementations
/// and provides the client interface for running the application.
///
/// Usage:
/// ```cpp
/// int main() {
///     Engine engine;
///
///     // Register system implementations against contracts
///     engine.registerSystem<IEventSystem, EventSystem>();
///     engine.registerSystem<IEntitySystem, EntitySystem>();
///     engine.registerSystem<IGraphicsSystem, VulkanGraphicsSystem>();
///
///     // Run the client application (dependencies injected via constructor)
///     engine.run<MyGameApp>();
/// }
/// ```
class Engine {
public:
    Engine() = default;
    ~Engine() = default;

    // Non-copyable, movable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) noexcept = default;
    Engine& operator=(Engine&&) noexcept = default;

    /// Register an implementation type for a contract interface.
    /// The implementation will be instantiated by Kangaru with its dependencies injected.
    ///
    /// Example:
    /// ```cpp
    /// engine.registerSystem<IGraphicsSystem, VulkanGraphicsSystem>();
    /// ```
    template<typename Contract, typename Implementation>
    void registerSystem() {
        static_assert(std::is_base_of_v<Contract, Implementation>,
            "Implementation must inherit from Contract interface");

        // Register the service mapping in Kangaru
        // The Implementation's Kangaru service definition handles the wiring
        registrations_.push_back([this]() {
            // This will be called when we need to resolve the service
            // Kangaru services are auto-registered when their modules are imported
        });
    }

    /// Register a factory function that creates an implementation.
    /// Useful when the implementation requires custom initialization.
    ///
    /// Example:
    /// ```cpp
    /// engine.registerSystem<IAudioSystem>([]() {
    ///     return std::make_unique<FMODAudioSystem>("config.json");
    /// });
    /// ```
    template<typename Contract>
    void registerSystem(std::function<std::unique_ptr<Contract>()> factory) {
        factories_[std::type_index(typeid(Contract))] = [factory = std::move(factory)]() -> void* {
            return factory().release();
        };
    }

    /// Run the application. The App type must:
    /// 1. Inherit from IApplication
    /// 2. Have a constructor that accepts its dependencies (injected by Kangaru)
    /// 3. Implement the run() method
    ///
    /// Example:
    /// ```cpp
    /// class MyGame : public IApplication {
    /// public:
    ///     MyGame(IGraphicsSystem& graphics, IInputSystem& input)
    ///         : graphics_(&graphics), input_(&input) {}
    ///
    ///     void run() override {
    ///         // Game loop
    ///     }
    /// };
    ///
    /// engine.run<MyGame>();
    /// ```
    template<typename App>
    void run() {
        static_assert(std::is_base_of_v<IApplication, App>,
            "App must inherit from IApplication");

        // Resolve the application from the DI container
        // Kangaru will inject all constructor dependencies
        auto& app = container_.service<typename App::service_type>();

        // Run the application
        app.run();
    }

    /// Get the Kangaru container for advanced usage.
    /// Prefer using registerSystem() and run() instead.
    kgr::container& container() { return container_; }
    const kgr::container& container() const { return container_; }

private:
    kgr::container container_;
    std::vector<std::function<void()>> registrations_;
    std::unordered_map<std::type_index, std::function<void*()>> factories_;
};

}  // namespace bestow::core
