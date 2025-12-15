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

    /// Register a service type with the DI container.
    /// The service definition handles interface-to-implementation mapping.
    ///
    /// Example:
    /// ```cpp
    /// engine.registerSystem<EventSystemService>();
    /// engine.registerSystem<EntitySystemService>();
    /// engine.registerSystem<vulkan::VulkanGraphics3DSystemService>();
    /// ```
    template<typename ServiceType>
    void registerSystem() {
        container_.service<ServiceType>();
    }

    /// Run the application. The App type must:
    /// 1. Inherit from IApplication
    /// 2. Have a nested `Service` type (Kangaru service definition)
    /// 3. Implement the run() method
    ///
    /// Example:
    /// ```cpp
    /// class MyGame : public IApplication {
    /// public:
    ///     MyGame(IGraphicsSystem* graphics, IInputSystem* input)
    ///         : graphics_(graphics), input_(input) {}
    ///
    ///     void run() override { /* Game loop */ }
    ///
    ///     // Kangaru service definition
    ///     struct Service : kgr::single_service<MyGame> { ... };
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
        auto& app = container_.service<typename App::Service>();

        // Run the application
        app.run();
    }

    /// Get the Kangaru container for advanced usage.
    kgr::container& container() { return container_; }
    const kgr::container& container() const { return container_; }

private:
    kgr::container container_;
};

}  // namespace bestow::core
