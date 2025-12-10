// bestow-core/src/bestow.core.cppm
// Core utilities and application framework
//
// STATUS: Implemented 2025-11-25
// COMPLETE: Timer, FrameTimer, Easing, JobSystem, UUID, Logging
// PARTIAL: Application (needs game loop integration with systems)
// BLOCKED: None
// TESTS: CoreSystemTests.cpp added (pending build verification)

module;

#include <taskflow/taskflow.hpp>

export module bestow.core;

import std;
import bestow;  // For BestowEngine and interface types
export import bestow.utils;  // Re-export utilities for backwards compatibility

export namespace bestow::core {

//==========================================================================
// Engine Configuration
//==========================================================================

struct GraphicsConfig {
    int width = 1280;
    int height = 720;
    std::string title = "Bestow Application";
    bool vsync = true;
    Color clearColor = Color{26, 26, 26, 255};  // Dark gray (0.1 * 255 ≈ 26)
};

//==========================================================================
// Forward Declarations
//==========================================================================

class Engine;

//==========================================================================
// Engine Builder
//==========================================================================

class EngineBuilder {
public:
    EngineBuilder();
    ~EngineBuilder();

    // System configuration (fluent interface)
    EngineBuilder& withEvents();
    EngineBuilder& withEntities();
    EngineBuilder& withPhysics();
    EngineBuilder& withPhysics3D();
    EngineBuilder& withGraphics(GraphicsConfig config);
    EngineBuilder& withGraphics3D(GraphicsConfig config);
    EngineBuilder& withAudio();
    EngineBuilder& withInput();
    EngineBuilder& withAssets(std::string_view basePath);
    EngineBuilder& withSave(std::string_view savePath);
    EngineBuilder& withLevel();
    EngineBuilder& withAI();
    EngineBuilder& withCamera(Size viewportSize);
    EngineBuilder& withGAS();
    EngineBuilder& withBlueprints();
    EngineBuilder& withUI();
    EngineBuilder& withGameStates();

    // Build the engine
    std::expected<Engine, std::string> build();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

//==========================================================================
// Engine
//==========================================================================

class Engine {
public:
    Engine();
    ~Engine();

    // Move-only
    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Get the engine aggregate (all systems)
    BestowEngine& systems();
    const BestowEngine& systems() const;

    // Game loop control
    void run(class Application& app);
    void quit();
    bool isRunning() const;

private:
    friend class EngineBuilder;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

//==========================================================================
// Application Base
//==========================================================================

class Application {
public:
    Application() = default;
    virtual ~Application() = default;

    // Non-copyable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Lifecycle hooks (called by Engine)
    virtual bool initialize(Engine& engine) = 0;
    virtual void updateFixed(DeltaTime dt) = 0;
    virtual void render(float alpha) = 0;
    virtual void shutdown() = 0;
};

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
    // Jobs are batched until wait() is called
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
    // Note: Jobs submitted during execution will remain in the taskflow
    // and require a subsequent wait() call
    void wait() {
        if (!taskflow_.empty()) {
            // Create a new taskflow for the current batch
            tf::Taskflow currentBatch = std::move(taskflow_);
            taskflow_ = tf::Taskflow{};

            // Run only the current batch
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

}  // namespace bestow::core
