// bestow-core/src/bestow.core.cppm
// Core utilities and Engine composition root
//
// The Engine is the composition root that:
// 1. Allows clients to register implementations against contract interfaces
// 2. Uses bestow.di to wire dependencies automatically
// 3. Instantiates and runs the client's Application with injected dependencies

module;

#include <taskflow/taskflow.hpp>

export module bestow.core;

import std;
export import bestow.di;
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
/// Usage Option 1 - Application base class (RECOMMENDED):
/// ```cpp
/// class MyGame : public Application<MyGame, IGraphics3DSystem, IInputSystem> {
/// public:
///     MyGame(IGraphics3DSystem& g, IInputSystem& i)
///         : graphics_(&g), input_(&i) {}
///     void run() override { /* game loop */ }
/// private:
///     IGraphics3DSystem* graphics_;
///     IInputSystem* input_;
/// };
///
/// int main() {
///     Engine engine;
///     engine.use<IGraphics3DSystem, VulkanGraphics3DSystem>();
///     engine.use<IInputSystem, InputSystem>();
///     engine.run<MyGame>();  // Dependencies auto-detected!
/// }
/// ```
///
/// Usage Option 2 - Explicit deps in run<>():
/// ```cpp
/// engine.run<MyGame, IGraphics3DSystem, IInputSystem>();
/// ```
///
/// Usage Option 3 - Engine& pattern (dynamic access):
/// ```cpp
/// class MyGame : public IApplication {
///     MyGame(Engine& e) : graphics_(&e.get<IGraphics3DSystem>()) {}
/// };
/// engine.run<MyGame>();
/// ```
class Engine {
public:
    Engine() = default;
    ~Engine() = default;

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) noexcept = default;
    Engine& operator=(Engine&&) noexcept = default;

    /// Register a default-constructible implementation for a contract interface.
    /// Example: engine.use<IEventSystem, EventSystem>();
    template<typename Contract, typename Implementation>
        requires std::is_default_constructible_v<Implementation>
    void use() {
        static_assert(std::is_base_of_v<Contract, Implementation>,
            "Implementation must inherit from Contract");
        services_.addSingleton<Contract, Implementation>();
    }

    /// Register an implementation with a factory that resolves dependencies.
    /// Example: engine.use<IAssetSystem, AssetSystem>(
    ///     [](auto& sp) { return new AssetSystem(sp.template get<IEventSystem>()); });
    template<typename Contract, typename Implementation>
    void use(std::function<Implementation*(di::ServiceProvider&)> factory) {
        static_assert(std::is_base_of_v<Contract, Implementation>,
            "Implementation must inherit from Contract");
        services_.addSingleton<Contract, Implementation>(std::move(factory));
    }

    /// Check if a system has been registered for a contract interface.
    template<typename Contract>
    bool has() const {
        if (provider_) {
            return provider_->has<Contract>();
        }
        return false;
    }

    /// Get a system by its contract interface.
    /// Only available after build() (called automatically by run()).
    template<typename Contract>
    Contract& get() {
        ensureBuilt();
        return provider_->get<Contract>();
    }

    /// Get a system by its contract interface, or nullptr if not registered.
    template<typename Contract>
    Contract* tryGet() {
        ensureBuilt();
        return provider_->tryGet<Contract>();
    }

    /// Build the service provider (creates all singletons, calls initialize()).
    /// Called automatically by run(), but can be called explicitly for testing.
    void build() {
        provider_ = std::make_unique<di::ServiceProvider>(services_.build());
    }

    /// Run the application.
    /// Dependencies are auto-detected if App inherits from Application<App, Deps...>,
    /// or can be specified explicitly: engine.run<MyGame, IDep1, IDep2>()
    template<typename App, typename... Contracts>
    void run() {
        static_assert(std::is_base_of_v<IApplication, App>,
            "App must inherit from IApplication or Application<>");

        ensureBuilt();

        if constexpr (sizeof...(Contracts) > 0) {
            // Explicit contracts provided - use them
            App app(get<Contracts>()...);
            app.run();
        } else if constexpr (requires { typename App::Dependencies; }) {
            // App has Dependencies type (from Application<> base) - use it
            runWithDeps<App>(static_cast<typename App::Dependencies*>(nullptr));
        } else {
            // Fallback to Engine& constructor
            App app(*this);
            app.run();
        }
    }

private:
    void ensureBuilt() {
        if (!provider_) {
            build();
        }
    }

    /// Helper to unpack tuple and inject dependencies
    template<typename App, typename... Deps>
    void runWithDeps(std::tuple<Deps...>*) {
        App app(get<Deps>()...);
        app.run();
    }

    di::ServiceCollection services_;
    std::unique_ptr<di::ServiceProvider> provider_;
};

}  // namespace bestow::core
