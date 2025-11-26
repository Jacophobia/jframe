// tests/unit/EventSystemTests.cpp
// Event system unit tests

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest.h>

import jframe.events;
import jframe.events.impl;
import jframe.types;

namespace jframe::tests {

class EventSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventSystem_ = createEventSystem();
    }

    std::unique_ptr<IEventSystem> eventSystem_;
};

TEST_F(EventSystemTest, SubscribeAndPublish) {
    bool called = false;
    auto id = eventSystem_->subscribe("test_event", [&](const EventData& data) {
        called = true;
    });

    EXPECT_FALSE(called);

    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_TRUE(called);

    eventSystem_->unsubscribe(id);
}

TEST_F(EventSystemTest, QueueAndProcess) {
    int callCount = 0;
    eventSystem_->subscribe("queued_event", [&](const EventData& data) {
        callCount++;
    });

    eventSystem_->queue("queued_event", EntityEventData{});
    eventSystem_->queue("queued_event", EntityEventData{});
    eventSystem_->queue("queued_event", EntityEventData{});

    EXPECT_EQ(callCount, 0);
    EXPECT_EQ(eventSystem_->queueSize(), 3);

    eventSystem_->processQueue();

    EXPECT_EQ(callCount, 3);
    EXPECT_EQ(eventSystem_->queueSize(), 0);
}

TEST_F(EventSystemTest, Unsubscribe) {
    int callCount = 0;
    auto id = eventSystem_->subscribe("test_event", [&](const EventData& data) {
        callCount++;
    });

    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(callCount, 1);

    eventSystem_->unsubscribe(id);

    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(callCount, 1);
}

TEST_F(EventSystemTest, MultipleSubscribers) {
    int count1 = 0, count2 = 0;

    eventSystem_->subscribe("multi_event", [&](const EventData&) { count1++; });
    eventSystem_->subscribe("multi_event", [&](const EventData&) { count2++; });

    eventSystem_->publish("multi_event", EntityEventData{});

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}

TEST_F(EventSystemTest, ClearQueue) {
    eventSystem_->queue("event", EntityEventData{});
    eventSystem_->queue("event", EntityEventData{});

    EXPECT_EQ(eventSystem_->queueSize(), 2);

    eventSystem_->clearQueue();

    EXPECT_EQ(eventSystem_->queueSize(), 0);
}

}  // namespace jframe::tests
