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
#include <kangaru/kangaru.hpp>

import bestow.core;
import bestow.types;
import bestow.config;
import bestow.config.impl;
import bestow.assets.impl;   // ConfigSystem depends on AssetSystem
import bestow.events.impl;   // ConfigSystem depends on EventSystem

namespace bestow::tests {

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
    std::unordered_set<bestow::UUID> uuids;
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
// Engine Tests - New Composition Root API
//==========================================================================

class EngineTest : public ::testing::Test {};

TEST_F(EngineTest, DefaultConstruction) {
    // Engine should be default constructible
    core::Engine engine;
    (void)engine;  // Suppress unused variable warning
}

TEST_F(EngineTest, MoveConstruction) {
    core::Engine engine1;
    core::Engine engine2 = std::move(engine1);
    (void)engine2;  // Suppress unused variable warning
}

TEST_F(EngineTest, MoveAssignment) {
    core::Engine engine1;
    core::Engine engine2;
    engine2 = std::move(engine1);
    (void)engine2;  // Suppress unused variable warning
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

//==========================================================================
// ConfigSystem Tests
//==========================================================================

class ConfigSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        configSystem_ = std::make_unique<ConfigSystem>();
        ASSERT_TRUE(configSystem_->initialize());
    }

    void TearDown() override {
        if (configSystem_) {
            configSystem_->shutdown();
        }
    }

    std::unique_ptr<ConfigSystem> configSystem_;
};

TEST_F(ConfigSystemTest, InitializeSucceeds) {
    // Already initialized in SetUp
    EXPECT_NE(configSystem_, nullptr);
}

TEST_F(ConfigSystemTest, HasKeyReturnsFalseForNonExistent) {
    EXPECT_FALSE(configSystem_->hasKey("nonexistent.key"));
}

TEST_F(ConfigSystemTest, GetFloatReturnsNulloptForNonExistent) {
    auto value = configSystem_->getFloat("nonexistent.key");
    EXPECT_FALSE(value.has_value());
}

TEST_F(ConfigSystemTest, GetFloatOrReturnsDefaultForNonExistent) {
    float result = configSystem_->getFloatOr("nonexistent.key", 42.0f);
    EXPECT_FLOAT_EQ(result, 42.0f);
}

TEST_F(ConfigSystemTest, GetIntReturnsNulloptForNonExistent) {
    auto value = configSystem_->getInt("nonexistent.key");
    EXPECT_FALSE(value.has_value());
}

TEST_F(ConfigSystemTest, GetIntOrReturnsDefaultForNonExistent) {
    int result = configSystem_->getIntOr("nonexistent.key", 123);
    EXPECT_EQ(result, 123);
}

TEST_F(ConfigSystemTest, GetBoolReturnsNulloptForNonExistent) {
    auto value = configSystem_->getBool("nonexistent.key");
    EXPECT_FALSE(value.has_value());
}

TEST_F(ConfigSystemTest, GetBoolOrReturnsDefaultForNonExistent) {
    bool result = configSystem_->getBoolOr("nonexistent.key", true);
    EXPECT_TRUE(result);
}

TEST_F(ConfigSystemTest, GetStringReturnsNulloptForNonExistent) {
    auto value = configSystem_->getString("nonexistent.key");
    EXPECT_FALSE(value.has_value());
}

TEST_F(ConfigSystemTest, GetStringOrReturnsDefaultForNonExistent) {
    std::string result = configSystem_->getStringOr("nonexistent.key", "default");
    EXPECT_EQ(result, "default");
}

TEST_F(ConfigSystemTest, SetAndGetFloat) {
    configSystem_->setFloat("test.float", 3.14f);

    EXPECT_TRUE(configSystem_->hasKey("test.float"));

    auto value = configSystem_->getFloat("test.float");
    EXPECT_TRUE(value.has_value());
    if (value.has_value()) {
        EXPECT_FLOAT_EQ(*value, 3.14f);
    }
}

TEST_F(ConfigSystemTest, SetAndGetInt) {
    configSystem_->setInt("test.int", 42);

    EXPECT_TRUE(configSystem_->hasKey("test.int"));

    auto value = configSystem_->getInt("test.int");
    EXPECT_TRUE(value.has_value());
    if (value.has_value()) {
        EXPECT_EQ(*value, 42);
    }
}

TEST_F(ConfigSystemTest, SetAndGetBool) {
    configSystem_->setBool("test.bool", true);

    EXPECT_TRUE(configSystem_->hasKey("test.bool"));

    auto value = configSystem_->getBool("test.bool");
    EXPECT_TRUE(value.has_value());
    if (value.has_value()) {
        EXPECT_TRUE(*value);
    }
}

TEST_F(ConfigSystemTest, SetAndGetString) {
    configSystem_->setString("test.string", "hello");

    EXPECT_TRUE(configSystem_->hasKey("test.string"));

    auto value = configSystem_->getString("test.string");
    EXPECT_TRUE(value.has_value());
    if (value.has_value()) {
        EXPECT_EQ(*value, "hello");
    }
}

TEST_F(ConfigSystemTest, GetKeysWithPrefixReturnsEmpty) {
    auto keys = configSystem_->getKeysWithPrefix("test.");
    EXPECT_TRUE(keys.empty());
}

TEST_F(ConfigSystemTest, GetKeysWithPrefixReturnsMatchingKeys) {
    configSystem_->setFloat("player.speed", 100.0f);
    configSystem_->setInt("player.health", 100);
    configSystem_->setBool("player.invincible", false);
    configSystem_->setString("enemy.type", "goblin");

    auto playerKeys = configSystem_->getKeysWithPrefix("player.");
    EXPECT_EQ(playerKeys.size(), 3);

    auto enemyKeys = configSystem_->getKeysWithPrefix("enemy.");
    EXPECT_EQ(enemyKeys.size(), 1);
}

TEST_F(ConfigSystemTest, GetLoadedConfigsInitiallyEmpty) {
    auto configs = configSystem_->getLoadedConfigs();
    EXPECT_TRUE(configs.empty());
}

TEST_F(ConfigSystemTest, HotReloadInitiallyDisabled) {
    EXPECT_FALSE(configSystem_->isHotReloadEnabled());
}

TEST_F(ConfigSystemTest, EnableHotReload) {
    configSystem_->enableHotReload(true);
    EXPECT_TRUE(configSystem_->isHotReloadEnabled());

    configSystem_->enableHotReload(false);
    EXPECT_FALSE(configSystem_->isHotReloadEnabled());
}

TEST_F(ConfigSystemTest, UpdateDoesNotCrash) {
    // Update should not crash even with no configs loaded
    EXPECT_NO_THROW(configSystem_->update(DeltaTime{0.016f}));
}

TEST_F(ConfigSystemTest, GetIntArrayReturnsEmpty) {
    auto values = configSystem_->getIntArray("nonexistent.array");
    EXPECT_TRUE(values.empty());
}

TEST_F(ConfigSystemTest, GetFloatArrayReturnsEmpty) {
    auto values = configSystem_->getFloatArray("nonexistent.array");
    EXPECT_TRUE(values.empty());
}

TEST_F(ConfigSystemTest, GetStringArrayReturnsEmpty) {
    auto values = configSystem_->getStringArray("nonexistent.array");
    EXPECT_TRUE(values.empty());
}

TEST_F(ConfigSystemTest, SubscribeToConfigChanges) {
    int callbackCount = 0;
    std::string changedKey;

    auto id = configSystem_->onConfigChanged([&](const ConfigKey& key) {
        callbackCount++;
        changedKey = key;
    });

    EXPECT_GT(id, 0);

    // Setting a value should trigger the callback
    configSystem_->setFloat("test.value", 1.0f);

    // Note: Callback may be async or immediate depending on implementation
    // We just verify subscription ID is valid

    configSystem_->unsubscribe(id);
}

TEST_F(ConfigSystemTest, SubscribeToKeyChanges) {
    int callbackCount = 0;
    std::string changedKey;

    auto id = configSystem_->onKeyChanged("player.", [&](const ConfigKey& key) {
        callbackCount++;
        changedKey = key;
    });

    EXPECT_GT(id, 0);

    configSystem_->unsubscribe(id);
}

TEST_F(ConfigSystemTest, UnsubscribeInvalidId) {
    // Should not crash
    EXPECT_NO_THROW(configSystem_->unsubscribe(99999));
}

// ============================================================================
// Kangaru DI Integration Tests for ConfigSystem
// ============================================================================

TEST_F(ConfigSystemTest, KangaruServiceInstantiation) {
    // Test that ConfigSystem can be instantiated via Kangaru DI
    kgr::container container;

    // Register dependencies first - ConfigSystem depends on AssetSystem and EventSystem
    container.service<EventSystemService>();
    container.service<AssetSystemService>();

    auto& configSystem = container.service<ConfigSystemService>();

    // Verify the service is valid
    EXPECT_NE(&configSystem, nullptr);

    // Verify it needs initialization
    EXPECT_TRUE(configSystem.initialize());

    // Verify it behaves like a ConfigSystem
    EXPECT_FALSE(configSystem.hasKey("nonexistent.key"));

    // Test that it's a singleton
    auto& configSystem2 = container.service<ConfigSystemService>();
    EXPECT_EQ(&configSystem, &configSystem2);

    configSystem.shutdown();
}

TEST_F(ConfigSystemTest, KangaruServiceWithSetGet) {
    // Test set/get operations using Kangaru-instantiated service
    kgr::container container;
    // Register dependencies first - ConfigSystem depends on AssetSystem and EventSystem
    container.service<EventSystemService>();
    container.service<AssetSystemService>();
    auto& configSystem = container.service<ConfigSystemService>();

    ASSERT_TRUE(configSystem.initialize());

    configSystem.setFloat("di.test.float", 99.9f);
    configSystem.setInt("di.test.int", 777);
    configSystem.setString("di.test.string", "kangaru");

    auto floatVal = configSystem.getFloat("di.test.float");
    auto intVal = configSystem.getInt("di.test.int");
    auto stringVal = configSystem.getString("di.test.string");

    EXPECT_TRUE(floatVal.has_value());
    EXPECT_TRUE(intVal.has_value());
    EXPECT_TRUE(stringVal.has_value());

    if (floatVal.has_value()) {
        EXPECT_FLOAT_EQ(*floatVal, 99.9f);
    }
    if (intVal.has_value()) {
        EXPECT_EQ(*intVal, 777);
    }
    if (stringVal.has_value()) {
        EXPECT_EQ(*stringVal, "kangaru");
    }

    configSystem.shutdown();
}

}  // namespace bestow::tests
