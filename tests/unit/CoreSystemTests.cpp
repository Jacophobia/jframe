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

}  // namespace jframe::tests
