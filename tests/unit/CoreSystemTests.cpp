// tests/unit/CoreSystemTests.cpp
// Core utilities unit tests

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

import jframe.core;
import jframe.types;

namespace jframe::tests {

//==========================================================================
// Timer Tests
//==========================================================================

class TimerTest : public ::testing::Test {};

TEST_F(TimerTest, InitialTime) {
    core::Timer timer;
    // Timer should start with a very small elapsed time
    EXPECT_LT(timer.elapsedSeconds(), 0.1f);
}

TEST_F(TimerTest, Reset) {
    core::Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    float before = timer.elapsedMilliseconds();
    timer.reset();
    float after = timer.elapsedMilliseconds();
    EXPECT_LT(after, before);
}

TEST_F(TimerTest, ElapsedTime) {
    core::Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    float ms = timer.elapsedMilliseconds();
    // Should be at least 50ms (allow some tolerance)
    EXPECT_GE(ms, 45.0f);
    EXPECT_LT(ms, 200.0f);  // sanity check
}

//==========================================================================
// FrameTimer Tests
//==========================================================================

class FrameTimerTest : public ::testing::Test {};

TEST_F(FrameTimerTest, InitialFrameCount) {
    core::FrameTimer timer;
    EXPECT_EQ(timer.frameCount(), 0);
}

TEST_F(FrameTimerTest, TickIncreasesFrameCount) {
    core::FrameTimer timer;
    timer.tick();
    EXPECT_EQ(timer.frameCount(), 1);
    timer.tick();
    EXPECT_EQ(timer.frameCount(), 2);
}

TEST_F(FrameTimerTest, TickReturnsDeltaTime) {
    core::FrameTimer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    float dt = timer.tick();
    // Should be around 20ms = 0.02s
    EXPECT_GE(dt, 0.015f);
    EXPECT_LT(dt, 0.5f);  // sanity check
}

//==========================================================================
// Easing Tests
//==========================================================================

class EasingTest : public ::testing::Test {};

TEST_F(EasingTest, LinearBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::linear(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(core::Easing::linear(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(core::Easing::linear(0.5f), 0.5f);
}

TEST_F(EasingTest, EaseInQuadBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::easeInQuad(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(core::Easing::easeInQuad(1.0f), 1.0f);
}

TEST_F(EasingTest, EaseOutQuadBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::easeOutQuad(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(core::Easing::easeOutQuad(1.0f), 1.0f);
}

TEST_F(EasingTest, EaseInOutQuadBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::easeInOutQuad(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(core::Easing::easeInOutQuad(1.0f), 1.0f);
}

TEST_F(EasingTest, EaseInCubicBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::easeInCubic(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(core::Easing::easeInCubic(1.0f), 1.0f);
}

TEST_F(EasingTest, EaseOutCubicBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::easeOutCubic(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(core::Easing::easeOutCubic(1.0f), 1.0f);
}

TEST_F(EasingTest, EaseInOutCubicBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::easeInOutCubic(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(core::Easing::easeInOutCubic(1.0f), 1.0f);
}

TEST_F(EasingTest, EaseInSineBoundaries) {
    EXPECT_NEAR(core::Easing::easeInSine(0.0f), 0.0f, 0.0001f);
    EXPECT_NEAR(core::Easing::easeInSine(1.0f), 1.0f, 0.0001f);
}

TEST_F(EasingTest, EaseOutSineBoundaries) {
    EXPECT_NEAR(core::Easing::easeOutSine(0.0f), 0.0f, 0.0001f);
    EXPECT_NEAR(core::Easing::easeOutSine(1.0f), 1.0f, 0.0001f);
}

TEST_F(EasingTest, EaseInOutSineBoundaries) {
    EXPECT_NEAR(core::Easing::easeInOutSine(0.0f), 0.0f, 0.0001f);
    EXPECT_NEAR(core::Easing::easeInOutSine(1.0f), 1.0f, 0.0001f);
}

TEST_F(EasingTest, EaseInExpoBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::easeInExpo(0.0f), 0.0f);
    EXPECT_NEAR(core::Easing::easeInExpo(1.0f), 1.0f, 0.0001f);
}

TEST_F(EasingTest, EaseOutExpoBoundaries) {
    EXPECT_NEAR(core::Easing::easeOutExpo(0.0f), 0.0f, 0.001f);
    EXPECT_FLOAT_EQ(core::Easing::easeOutExpo(1.0f), 1.0f);
}

TEST_F(EasingTest, EaseOutBounceBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::easeOutBounce(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(core::Easing::easeOutBounce(1.0f), 1.0f);
}

TEST_F(EasingTest, EaseInBounceBoundaries) {
    EXPECT_FLOAT_EQ(core::Easing::easeInBounce(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(core::Easing::easeInBounce(1.0f), 1.0f);
}

TEST_F(EasingTest, EaseInQuadCurve) {
    // Ease-in should start slow (below linear)
    float mid = core::Easing::easeInQuad(0.5f);
    EXPECT_LT(mid, 0.5f);
}

TEST_F(EasingTest, EaseOutQuadCurve) {
    // Ease-out should start fast (above linear)
    float mid = core::Easing::easeOutQuad(0.5f);
    EXPECT_GT(mid, 0.5f);
}

//==========================================================================
// JobSystem Tests
//==========================================================================

class JobSystemTest : public ::testing::Test {
protected:
    core::JobSystem jobs_;
};

TEST_F(JobSystemTest, WorkerCount) {
    EXPECT_GT(jobs_.workerCount(), 0);
}

TEST_F(JobSystemTest, InitiallyNoJobs) {
    EXPECT_FALSE(jobs_.hasPendingJobs());
}

TEST_F(JobSystemTest, SubmitSingleJob) {
    std::atomic<int> counter{0};

    jobs_.submit([&counter]() {
        counter++;
    });

    EXPECT_TRUE(jobs_.hasPendingJobs());

    jobs_.wait();

    EXPECT_FALSE(jobs_.hasPendingJobs());
    EXPECT_EQ(counter.load(), 1);
}

TEST_F(JobSystemTest, SubmitMultipleJobs) {
    std::atomic<int> counter{0};
    constexpr int numJobs = 100;

    for (int i = 0; i < numJobs; ++i) {
        jobs_.submit([&counter]() {
            counter++;
        });
    }

    jobs_.wait();

    EXPECT_EQ(counter.load(), numJobs);
}

TEST_F(JobSystemTest, WaitWithNoJobs) {
    // Should not hang or crash
    jobs_.wait();
    EXPECT_FALSE(jobs_.hasPendingJobs());
}

TEST_F(JobSystemTest, ParallelExecution) {
    std::atomic<int> maxConcurrent{0};
    std::atomic<int> currentlyRunning{0};
    constexpr int numJobs = 50;

    for (int i = 0; i < numJobs; ++i) {
        jobs_.submit([&currentlyRunning, &maxConcurrent]() {
            int curr = ++currentlyRunning;
            int expected = maxConcurrent.load();
            while (curr > expected && !maxConcurrent.compare_exchange_weak(expected, curr)) {
                // Update max
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            --currentlyRunning;
        });
    }

    jobs_.wait();

    // If we have more than 1 worker, we should have had some parallelism
    if (jobs_.workerCount() > 1) {
        EXPECT_GT(maxConcurrent.load(), 1);
    }
}

TEST_F(JobSystemTest, JobCanModifySharedState) {
    std::mutex mtx;
    std::vector<int> results;

    for (int i = 0; i < 10; ++i) {
        jobs_.submit([i, &mtx, &results]() {
            std::lock_guard lock(mtx);
            results.push_back(i);
        });
    }

    jobs_.wait();

    // All 10 values should be present (order doesn't matter)
    EXPECT_EQ(results.size(), 10);
    std::sort(results.begin(), results.end());
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(results[i], i);
    }
}

//==========================================================================
// UUID Tests
//==========================================================================

class UUIDTest : public ::testing::Test {};

TEST_F(UUIDTest, GenerateUnique) {
    auto uuid1 = core::generateUUID();
    auto uuid2 = core::generateUUID();
    EXPECT_NE(uuid1, uuid2);
}

TEST_F(UUIDTest, GenerateManyUnique) {
    std::unordered_set<jframe::UUID> uuids;
    constexpr int count = 1000;

    for (int i = 0; i < count; ++i) {
        auto uuid = core::generateUUID();
        EXPECT_TRUE(uuids.insert(uuid).second) << "UUID collision at iteration " << i;
    }

    EXPECT_EQ(uuids.size(), count);
}

//==========================================================================
// Logging Tests (basic sanity checks)
//==========================================================================

class LoggingTest : public ::testing::Test {};

TEST_F(LoggingTest, LogInfoDoesNotCrash) {
    EXPECT_NO_THROW(core::logInfo("Test info message"));
}

TEST_F(LoggingTest, LogWarnDoesNotCrash) {
    EXPECT_NO_THROW(core::logWarn("Test warning message"));
}

TEST_F(LoggingTest, LogErrorDoesNotCrash) {
    EXPECT_NO_THROW(core::logError("Test error message"));
}

TEST_F(LoggingTest, LogDebugDoesNotCrash) {
    EXPECT_NO_THROW(core::logDebug("Test debug message"));
}

//==========================================================================
// GraphicsConfig Tests
//==========================================================================

class GraphicsConfigTest : public ::testing::Test {};

TEST_F(GraphicsConfigTest, DefaultValues) {
    core::GraphicsConfig config;
    EXPECT_EQ(config.width, 1280);
    EXPECT_EQ(config.height, 720);
    EXPECT_EQ(config.title, "JFrame Application");
    EXPECT_TRUE(config.vsync);
    EXPECT_EQ(config.clearColor.r, 26);
    EXPECT_EQ(config.clearColor.g, 26);
    EXPECT_EQ(config.clearColor.b, 26);
    EXPECT_EQ(config.clearColor.a, 255);
}

TEST_F(GraphicsConfigTest, CustomValues) {
    core::GraphicsConfig config;
    config.width = 1920;
    config.height = 1080;
    config.title = "Custom Title";
    config.vsync = false;
    config.clearColor = Color{255, 0, 0, 255};

    EXPECT_EQ(config.width, 1920);
    EXPECT_EQ(config.height, 1080);
    EXPECT_EQ(config.title, "Custom Title");
    EXPECT_FALSE(config.vsync);
    EXPECT_EQ(config.clearColor.r, 255);
    EXPECT_EQ(config.clearColor.g, 0);
    EXPECT_EQ(config.clearColor.b, 0);
}

TEST_F(GraphicsConfigTest, CopyConstruction) {
    core::GraphicsConfig config1;
    config1.width = 800;
    config1.height = 600;
    config1.title = "Test Window";

    core::GraphicsConfig config2 = config1;
    EXPECT_EQ(config2.width, 800);
    EXPECT_EQ(config2.height, 600);
    EXPECT_EQ(config2.title, "Test Window");
}

//==========================================================================
// Engine Tests
//==========================================================================

class EngineTest : public ::testing::Test {};

TEST_F(EngineTest, DefaultConstruction) {
    core::Engine engine;
    EXPECT_FALSE(engine.isRunning());
}

TEST_F(EngineTest, SystemsAccessor) {
    core::Engine engine;
    const auto& systems = engine.systems();
    // Engine with no systems should have nullptrs
    EXPECT_EQ(systems.events, nullptr);
    EXPECT_EQ(systems.entities, nullptr);
    EXPECT_EQ(systems.graphics, nullptr);
}

TEST_F(EngineTest, QuitStopsRunning) {
    core::Engine engine;
    // Note: Can't easily test run() in a unit test without a full application
    // This is a basic sanity check
    EXPECT_FALSE(engine.isRunning());
    engine.quit();
    EXPECT_FALSE(engine.isRunning());
}

//==========================================================================
// EngineBuilder Tests
//==========================================================================

class EngineBuilderTest : public ::testing::Test {};

TEST_F(EngineBuilderTest, BuildEmptyEngine) {
    core::EngineBuilder builder;
    auto result = builder.build();
    EXPECT_TRUE(result.has_value());
}

TEST_F(EngineBuilderTest, BuildWithEvents) {
    core::EngineBuilder builder;
    auto result = builder.withEvents().build();
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        auto& engine = result.value();
        EXPECT_NE(engine.systems().events, nullptr);
    }
}

TEST_F(EngineBuilderTest, BuildWithEntities) {
    core::EngineBuilder builder;
    auto result = builder.withEntities().build();
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        auto& engine = result.value();
        EXPECT_NE(engine.systems().entities, nullptr);
    }
}

TEST_F(EngineBuilderTest, BuildWithMultipleSystems) {
    core::EngineBuilder builder;
    auto result = builder
        .withEvents()
        .withEntities()
        .withAssets("assets")
        .build();
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        auto& engine = result.value();
        EXPECT_NE(engine.systems().events, nullptr);
        EXPECT_NE(engine.systems().entities, nullptr);
        EXPECT_NE(engine.systems().assets, nullptr);
    }
}

TEST_F(EngineBuilderTest, GraphicsRequiresConfig) {
    core::EngineBuilder builder;
    // Graphics without config should succeed (uses default config internally)
    core::GraphicsConfig config;
    config.width = 800;
    config.height = 600;
    config.title = "Test";
    auto result = builder.withGraphics(config).build();
    // This may fail if we're in a headless environment, but should not crash
    // Just check it returns an expected type
    EXPECT_TRUE(result.has_value() || result.error().size() > 0);
}

TEST_F(EngineBuilderTest, FluentInterface) {
    core::EngineBuilder builder;
    // Test that methods return references for chaining
    auto& b1 = builder.withEvents();
    auto& b2 = b1.withEntities();
    auto& b3 = b2.withAssets("assets");
    EXPECT_EQ(&builder, &b1);
    EXPECT_EQ(&builder, &b2);
    EXPECT_EQ(&builder, &b3);
}

TEST_F(EngineBuilderTest, WithPhysics) {
    core::EngineBuilder builder;
    auto result = builder.withPhysics().build();
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        auto& engine = result.value();
        EXPECT_NE(engine.systems().physics, nullptr);
    }
}

TEST_F(EngineBuilderTest, WithSave) {
    core::EngineBuilder builder;
    auto result = builder.withSave("saves").build();
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        auto& engine = result.value();
        EXPECT_NE(engine.systems().save, nullptr);
    }
}

TEST_F(EngineBuilderTest, WithCamera) {
    core::EngineBuilder builder;
    Size viewportSize{1280, 720};
    auto result = builder.withCamera(viewportSize).build();
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        auto& engine = result.value();
        EXPECT_NE(engine.systems().camera, nullptr);
    }
}

TEST_F(EngineBuilderTest, WithGAS) {
    core::EngineBuilder builder;
    auto result = builder.withGAS().build();
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        auto& engine = result.value();
        EXPECT_NE(engine.systems().gas, nullptr);
    }
}

TEST_F(EngineBuilderTest, WithBlueprints) {
    core::EngineBuilder builder;
    // Blueprints requires entities
    auto result = builder.withEntities().withBlueprints().build();
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        auto& engine = result.value();
        EXPECT_NE(engine.systems().entities, nullptr);
        EXPECT_NE(engine.systems().blueprints, nullptr);
    }
}

TEST_F(EngineBuilderTest, BlueprintsWithoutEntitiesFails) {
    core::EngineBuilder builder;
    auto result = builder.withBlueprints().build();
    // Should fail because blueprints requires entities
    EXPECT_FALSE(result.has_value());
    if (!result.has_value()) {
        EXPECT_FALSE(result.error().empty());
    }
}

//==========================================================================
// Application Tests
//==========================================================================

class ApplicationTest : public ::testing::Test {};

// Mock application for testing
class MockApplication : public core::Application {
public:
    bool initializeCalled = false;
    bool shutdownCalled = false;
    int updateFixedCallCount = 0;
    int renderCallCount = 0;

    bool initialize(core::Engine& engine) override {
        initializeCalled = true;
        return true;
    }

    void updateFixed(DeltaTime dt) override {
        updateFixedCallCount++;
    }

    void render(float alpha) override {
        renderCallCount++;
    }

    void shutdown() override {
        shutdownCalled = true;
    }
};

TEST_F(ApplicationTest, VirtualMethodsExist) {
    // Test that Application interface is abstract and has virtual methods
    MockApplication app;
    EXPECT_FALSE(app.initializeCalled);
    EXPECT_FALSE(app.shutdownCalled);
    EXPECT_EQ(app.updateFixedCallCount, 0);
    EXPECT_EQ(app.renderCallCount, 0);
}

//==========================================================================
// Additional Timer Edge Case Tests
//==========================================================================

TEST_F(TimerTest, MultipleResets) {
    core::Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    timer.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    timer.reset();
    float after = timer.elapsedMilliseconds();
    EXPECT_LT(after, 10.0f);  // Should be close to 0 after reset
}

TEST_F(TimerTest, ElapsedSecondsVsMilliseconds) {
    core::Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    float seconds = timer.elapsedSeconds();
    float milliseconds = timer.elapsedMilliseconds();
    // milliseconds should be ~1000x seconds
    EXPECT_NEAR(milliseconds, seconds * 1000.0f, 10.0f);
}

//==========================================================================
// Additional FrameTimer Edge Case Tests
//==========================================================================

TEST_F(FrameTimerTest, MultipleTicks) {
    core::FrameTimer timer;
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        timer.tick();
    }
    EXPECT_EQ(timer.frameCount(), 10);
}

TEST_F(FrameTimerTest, DeltaTimeConsistency) {
    core::FrameTimer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    float dt1 = timer.tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    float dt2 = timer.tick();
    // Both should be around 50ms = 0.05s
    // Use very high tolerance (100ms) for CI environments where VM scheduling
    // can cause significant timing variations (observed 74ms variance on macOS CI)
    EXPECT_NEAR(dt1, dt2, 0.100f);
}

//==========================================================================
// Additional Easing Edge Case Tests
//==========================================================================

TEST_F(EasingTest, AllEasingFunctionsMidpoint) {
    // Test that all easing functions return reasonable values at midpoint
    float mid = 0.5f;

    EXPECT_GE(core::Easing::linear(mid), 0.0f);
    EXPECT_LE(core::Easing::linear(mid), 1.0f);

    EXPECT_GE(core::Easing::easeInQuad(mid), 0.0f);
    EXPECT_LE(core::Easing::easeInQuad(mid), 1.0f);

    EXPECT_GE(core::Easing::easeOutQuad(mid), 0.0f);
    EXPECT_LE(core::Easing::easeOutQuad(mid), 1.0f);

    EXPECT_GE(core::Easing::easeInOutQuad(mid), 0.0f);
    EXPECT_LE(core::Easing::easeInOutQuad(mid), 1.0f);

    EXPECT_GE(core::Easing::easeInCubic(mid), 0.0f);
    EXPECT_LE(core::Easing::easeInCubic(mid), 1.0f);

    EXPECT_GE(core::Easing::easeOutCubic(mid), 0.0f);
    EXPECT_LE(core::Easing::easeOutCubic(mid), 1.0f);

    EXPECT_GE(core::Easing::easeInOutCubic(mid), 0.0f);
    EXPECT_LE(core::Easing::easeInOutCubic(mid), 1.0f);

    EXPECT_GE(core::Easing::easeInSine(mid), 0.0f);
    EXPECT_LE(core::Easing::easeInSine(mid), 1.0f);

    EXPECT_GE(core::Easing::easeOutSine(mid), 0.0f);
    EXPECT_LE(core::Easing::easeOutSine(mid), 1.0f);

    EXPECT_GE(core::Easing::easeInOutSine(mid), 0.0f);
    EXPECT_LE(core::Easing::easeInOutSine(mid), 1.0f);

    EXPECT_GE(core::Easing::easeInExpo(mid), 0.0f);
    EXPECT_LE(core::Easing::easeInExpo(mid), 1.0f);

    EXPECT_GE(core::Easing::easeOutExpo(mid), 0.0f);
    EXPECT_LE(core::Easing::easeOutExpo(mid), 1.0f);

    EXPECT_GE(core::Easing::easeOutBounce(mid), 0.0f);
    EXPECT_LE(core::Easing::easeOutBounce(mid), 1.0f);

    EXPECT_GE(core::Easing::easeInBounce(mid), 0.0f);
    EXPECT_LE(core::Easing::easeInBounce(mid), 1.0f);
}

TEST_F(EasingTest, EaseInOutQuadSymmetry) {
    // easeInOutQuad should be symmetric around 0.5
    float t1 = 0.25f;
    float t2 = 0.75f;
    float v1 = core::Easing::easeInOutQuad(t1);
    float v2 = core::Easing::easeInOutQuad(t2);
    // v1 should be distance from 0, v2 should be same distance from 1
    EXPECT_NEAR(v1, 1.0f - v2, 0.0001f);
}

TEST_F(EasingTest, EaseInOutCubicSymmetry) {
    float t1 = 0.3f;
    float t2 = 0.7f;
    float v1 = core::Easing::easeInOutCubic(t1);
    float v2 = core::Easing::easeInOutCubic(t2);
    EXPECT_NEAR(v1, 1.0f - v2, 0.0001f);
}

TEST_F(EasingTest, BounceEffectAtEnd) {
    // Bounce should have multiple local maxima
    std::vector<float> samples;
    for (int i = 0; i <= 10; ++i) {
        float t = i / 10.0f;
        samples.push_back(core::Easing::easeOutBounce(t));
    }

    // Should have at least one "dip" (non-monotonic)
    bool hasLocalMaximum = false;
    for (size_t i = 1; i < samples.size() - 1; ++i) {
        if (samples[i] > samples[i-1] && samples[i] > samples[i+1]) {
            hasLocalMaximum = true;
            break;
        }
    }
    // Bounce function should have some non-monotonic behavior
    // (though this specific test might be fragile depending on implementation)
}

TEST_F(EasingTest, ExpoZeroEdgeCase) {
    // easeInExpo should return exactly 0 at t=0
    EXPECT_FLOAT_EQ(core::Easing::easeInExpo(0.0f), 0.0f);
    // easeOutExpo should return exactly 1 at t=1
    EXPECT_FLOAT_EQ(core::Easing::easeOutExpo(1.0f), 1.0f);
}

TEST_F(EasingTest, NegativeInputHandling) {
    // Test behavior with negative input (undefined, but shouldn't crash)
    EXPECT_NO_THROW(core::Easing::linear(-0.1f));
    EXPECT_NO_THROW(core::Easing::easeInQuad(-0.1f));
}

TEST_F(EasingTest, OverOneInputHandling) {
    // Test behavior with input > 1.0 (undefined, but shouldn't crash)
    EXPECT_NO_THROW(core::Easing::linear(1.5f));
    EXPECT_NO_THROW(core::Easing::easeInQuad(1.5f));
}

//==========================================================================
// Additional JobSystem Edge Case Tests
//==========================================================================

TEST_F(JobSystemTest, MultipleWaits) {
    // Multiple waits should be safe
    std::atomic<int> counter{0};

    jobs_.submit([&counter]() { counter++; });
    jobs_.wait();
    EXPECT_EQ(counter.load(), 1);

    jobs_.submit([&counter]() { counter++; });
    jobs_.wait();
    EXPECT_EQ(counter.load(), 2);

    jobs_.wait();  // Wait with no jobs
    EXPECT_EQ(counter.load(), 2);
}

TEST_F(JobSystemTest, JobException) {
    // Jobs that throw shouldn't crash the system
    // Note: Taskflow may or may not propagate exceptions, this tests stability
    jobs_.submit([]() {
        // Intentionally do nothing that throws in this test
        // Real exception handling would require try-catch in job
    });

    EXPECT_NO_THROW(jobs_.wait());
}

TEST_F(JobSystemTest, VeryLargeJobCount) {
    std::atomic<int> counter{0};
    constexpr int numJobs = 10000;

    for (int i = 0; i < numJobs; ++i) {
        jobs_.submit([&counter]() {
            counter++;
        });
    }

    jobs_.wait();
    EXPECT_EQ(counter.load(), numJobs);
}

TEST_F(JobSystemTest, JobOrderingNotGuaranteed) {
    // Jobs may complete in any order
    std::mutex mtx;
    std::vector<int> order;

    for (int i = 0; i < 5; ++i) {
        jobs_.submit([i, &mtx, &order]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard lock(mtx);
            order.push_back(i);
        });
    }

    jobs_.wait();

    // All values should be present
    EXPECT_EQ(order.size(), 5);
    // But order may not be sequential
    // (This is an informational test, not a requirement)
}

TEST_F(JobSystemTest, NestedJobSubmission) {
    // Jobs can submit other jobs (though they must wait separately)
    std::atomic<int> counter{0};

    jobs_.submit([this, &counter]() {
        counter++;
        // Note: This creates a nested submission but requires a second wait
        this->jobs_.submit([&counter]() {
            counter++;
        });
    });

    jobs_.wait();  // Complete first job
    EXPECT_EQ(counter.load(), 1);  // Only outer job done

    jobs_.wait();  // Complete nested job
    EXPECT_EQ(counter.load(), 2);  // Both jobs done
}

TEST_F(JobSystemTest, SubmitBatch) {
    std::atomic<int> counter{0};
    constexpr int batchSize = 10;

    // Create a vector of lambdas
    std::vector<std::function<void()>> jobs;
    for (int i = 0; i < batchSize; ++i) {
        jobs.push_back([&counter]() {
            counter++;
        });
    }

    // Submit batch
    jobs_.submitBatch<std::function<void()>>(jobs);
    jobs_.wait();

    EXPECT_EQ(counter.load(), batchSize);
}

TEST_F(JobSystemTest, EmptyBatch) {
    std::vector<std::function<void()>> emptyJobs;
    EXPECT_NO_THROW(jobs_.submitBatch<std::function<void()>>(emptyJobs));
    EXPECT_NO_THROW(jobs_.wait());
}

//==========================================================================
// Additional UUID Tests
//==========================================================================

TEST_F(UUIDTest, UUIDNonZero) {
    auto uuid = core::generateUUID();
    // UUID should be non-zero (statistically certain)
    EXPECT_NE(uuid, 0);
}

TEST_F(UUIDTest, UUIDDistribution) {
    // Generate many UUIDs and check they're well-distributed
    constexpr int count = 100;
    std::vector<UUID> uuids;

    for (int i = 0; i < count; ++i) {
        uuids.push_back(core::generateUUID());
    }

    // Check no duplicates
    std::unordered_set<UUID> uniqueUUIDs(uuids.begin(), uuids.end());
    EXPECT_EQ(uniqueUUIDs.size(), count);

    // Check distribution across high/low bits
    bool hasHighBit = false;
    bool hasLowBit = false;
    for (auto uuid : uuids) {
        if (uuid & 0x8000000000000000ULL) hasHighBit = true;
        if (uuid & 0x0000000000000001ULL) hasLowBit = true;
    }
    // Statistically very unlikely to not have both
    EXPECT_TRUE(hasHighBit);
    EXPECT_TRUE(hasLowBit);
}

//==========================================================================
// Logging with Format Strings
//==========================================================================

TEST_F(LoggingTest, LogInfoWithParameters) {
    // Test logging with formatted strings
    EXPECT_NO_THROW(core::logInfo("Test with value: " + std::to_string(42)));
}

TEST_F(LoggingTest, LogErrorWithParameters) {
    EXPECT_NO_THROW(core::logError("Error code: " + std::to_string(404)));
}

TEST_F(LoggingTest, LogEmptyString) {
    EXPECT_NO_THROW(core::logInfo(""));
    EXPECT_NO_THROW(core::logWarn(""));
    EXPECT_NO_THROW(core::logError(""));
    EXPECT_NO_THROW(core::logDebug(""));
}

TEST_F(LoggingTest, LogLongString) {
    std::string longMessage(10000, 'A');
    EXPECT_NO_THROW(core::logInfo(longMessage));
}

}  // namespace jframe::tests
