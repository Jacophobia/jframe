// tests/unit/EventSystemTests.cpp
// Event system unit tests

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>

import bestow;
import bestow.types;
import bestow.events.impl;  // For EventSystemService

namespace bestow::tests {

// Mock Event System for interface testing
class MockEventSystem : public IEventSystem {
public:
    SubscriptionId subscribe(const EventType& eventType, EventCallback callback) override {
        callbacks_[eventType].push_back({++nextId_, callback});
        return nextId_;
    }

    void unsubscribe(SubscriptionId id) override {
        for (auto& [type, subs] : callbacks_) {
            subs.erase(std::remove_if(subs.begin(), subs.end(),
                [id](const auto& sub) { return sub.first == id; }), subs.end());
        }
    }

    void unsubscribeAll(const EventType& eventType) override {
        callbacks_.erase(eventType);
    }

    void publish(const EventType& eventType, const EventData& data) override {
        if (auto it = callbacks_.find(eventType); it != callbacks_.end()) {
            for (const auto& [id, callback] : it->second) {
                callback(data);
            }
        }
    }

    void queue(const EventType& eventType, const EventData& data) override {
        eventQueue_.push_back({eventType, data});
    }

    void processQueue() override {
        // Process all queued events
        auto queueCopy = std::move(eventQueue_);
        eventQueue_.clear();
        for (const auto& [type, data] : queueCopy) {
            publish(type, data);
        }
    }

    void clearQueue() override {
        eventQueue_.clear();
    }

    std::size_t queueSize() const override {
        return eventQueue_.size();
    }

private:
    SubscriptionId nextId_ = 0;
    std::unordered_map<std::string, std::vector<std::pair<SubscriptionId, EventCallback>>> callbacks_;
    std::vector<std::pair<std::string, EventData>> eventQueue_;
};

class EventSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventSystem_ = std::make_unique<MockEventSystem>();
    }

    std::unique_ptr<MockEventSystem> eventSystem_;
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

//==============================================================================
// NEW TESTS: Enhanced Coverage
//==============================================================================

TEST_F(EventSystemTest, UnsubscribeAll) {
    int count1 = 0, count2 = 0, count3 = 0;

    // Subscribe multiple callbacks to same event type
    eventSystem_->subscribe("test_event", [&](const EventData&) { count1++; });
    eventSystem_->subscribe("test_event", [&](const EventData&) { count2++; });

    // Subscribe to different event type (should not be affected)
    eventSystem_->subscribe("other_event", [&](const EventData&) { count3++; });

    // Verify all subscribers work
    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 0);

    // Unsubscribe all from test_event
    eventSystem_->unsubscribeAll("test_event");

    // Verify test_event subscribers are gone but other_event still works
    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(count1, 1);  // Should not have increased
    EXPECT_EQ(count2, 1);  // Should not have increased

    eventSystem_->publish("other_event", EntityEventData{});
    EXPECT_EQ(count3, 1);  // Should work
}

TEST_F(EventSystemTest, UnsubscribeAllNonExistentType) {
    // Should not crash when unsubscribing from non-existent type
    EXPECT_NO_THROW(eventSystem_->unsubscribeAll("nonexistent_event"));
}

TEST_F(EventSystemTest, UnsubscribeInvalidId) {
    // Should not crash when unsubscribing with invalid ID
    EXPECT_NO_THROW(eventSystem_->unsubscribe(999999));
}

TEST_F(EventSystemTest, UnsubscribeTwice) {
    int callCount = 0;
    auto id = eventSystem_->subscribe("test_event", [&](const EventData&) {
        callCount++;
    });

    eventSystem_->unsubscribe(id);

    // Unsubscribing twice should not crash
    EXPECT_NO_THROW(eventSystem_->unsubscribe(id));

    // Verify callback is not called
    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(callCount, 0);
}

TEST_F(EventSystemTest, PublishWithNoSubscribers) {
    // Should not crash when publishing to event with no subscribers
    EXPECT_NO_THROW(eventSystem_->publish("nonexistent_event", EntityEventData{}));
}

TEST_F(EventSystemTest, QueueWithNoSubscribers) {
    // Should not crash when queuing event with no subscribers
    eventSystem_->queue("nonexistent_event", EntityEventData{});
    EXPECT_EQ(eventSystem_->queueSize(), 1);

    // Processing should not crash even with no subscribers
    EXPECT_NO_THROW(eventSystem_->processQueue());
    EXPECT_EQ(eventSystem_->queueSize(), 0);
}

TEST_F(EventSystemTest, ProcessEmptyQueue) {
    EXPECT_EQ(eventSystem_->queueSize(), 0);

    // Processing empty queue should not crash
    EXPECT_NO_THROW(eventSystem_->processQueue());

    EXPECT_EQ(eventSystem_->queueSize(), 0);
}

TEST_F(EventSystemTest, ClearEmptyQueue) {
    EXPECT_EQ(eventSystem_->queueSize(), 0);

    // Clearing empty queue should not crash
    EXPECT_NO_THROW(eventSystem_->clearQueue());

    EXPECT_EQ(eventSystem_->queueSize(), 0);
}

TEST_F(EventSystemTest, QueueSizeAccuracy) {
    EXPECT_EQ(eventSystem_->queueSize(), 0);

    eventSystem_->queue("event1", EntityEventData{});
    EXPECT_EQ(eventSystem_->queueSize(), 1);

    eventSystem_->queue("event2", EntityEventData{});
    EXPECT_EQ(eventSystem_->queueSize(), 2);

    eventSystem_->queue("event3", EntityEventData{});
    EXPECT_EQ(eventSystem_->queueSize(), 3);

    eventSystem_->processQueue();
    EXPECT_EQ(eventSystem_->queueSize(), 0);
}

TEST_F(EventSystemTest, MixedQueueAndPublish) {
    int immediateCount = 0;
    int queuedCount = 0;

    eventSystem_->subscribe("immediate_event", [&](const EventData&) {
        immediateCount++;
    });

    eventSystem_->subscribe("queued_event", [&](const EventData&) {
        queuedCount++;
    });

    // Immediate publish should trigger immediately
    eventSystem_->publish("immediate_event", EntityEventData{});
    EXPECT_EQ(immediateCount, 1);
    EXPECT_EQ(queuedCount, 0);

    // Queued event should not trigger yet
    eventSystem_->queue("queued_event", EntityEventData{});
    EXPECT_EQ(immediateCount, 1);
    EXPECT_EQ(queuedCount, 0);
    EXPECT_EQ(eventSystem_->queueSize(), 1);

    // Process queue should trigger queued event
    eventSystem_->processQueue();
    EXPECT_EQ(immediateCount, 1);
    EXPECT_EQ(queuedCount, 1);
    EXPECT_EQ(eventSystem_->queueSize(), 0);
}

TEST_F(EventSystemTest, MultipleEventTypes) {
    int count1 = 0, count2 = 0, count3 = 0;

    eventSystem_->subscribe("event_type_1", [&](const EventData&) { count1++; });
    eventSystem_->subscribe("event_type_2", [&](const EventData&) { count2++; });
    eventSystem_->subscribe("event_type_3", [&](const EventData&) { count3++; });

    eventSystem_->publish("event_type_1", EntityEventData{});
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 0);
    EXPECT_EQ(count3, 0);

    eventSystem_->publish("event_type_2", EntityEventData{});
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 0);

    eventSystem_->publish("event_type_3", EntityEventData{});
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 1);
}

TEST_F(EventSystemTest, EventDataPreservedThroughQueue) {
    Entity receivedEntity = static_cast<Entity>(0);

    eventSystem_->subscribe("entity_event", [&](const EventData& data) {
        auto entityData = std::get<EntityEventData>(data);
        receivedEntity = entityData.entity;
    });

    Entity testEntity = static_cast<Entity>(42);
    EntityEventData testData{testEntity};

    eventSystem_->queue("entity_event", testData);
    eventSystem_->processQueue();

    EXPECT_EQ(receivedEntity, testEntity);
}

TEST_F(EventSystemTest, SubscriptionIdUniqueness) {
    auto id1 = eventSystem_->subscribe("event", [](const EventData&) {});
    auto id2 = eventSystem_->subscribe("event", [](const EventData&) {});
    auto id3 = eventSystem_->subscribe("other_event", [](const EventData&) {});

    // All subscription IDs should be unique
    EXPECT_NE(id1, id2);
    EXPECT_NE(id1, id3);
    EXPECT_NE(id2, id3);
}

TEST_F(EventSystemTest, CallbackExecutionOrder) {
    std::vector<int> executionOrder;

    eventSystem_->subscribe("test_event", [&](const EventData&) {
        executionOrder.push_back(1);
    });

    eventSystem_->subscribe("test_event", [&](const EventData&) {
        executionOrder.push_back(2);
    });

    eventSystem_->subscribe("test_event", [&](const EventData&) {
        executionOrder.push_back(3);
    });

    eventSystem_->publish("test_event", EntityEventData{});

    // All callbacks should execute
    EXPECT_EQ(executionOrder.size(), 3);
    EXPECT_EQ(executionOrder[0], 1);
    EXPECT_EQ(executionOrder[1], 2);
    EXPECT_EQ(executionOrder[2], 3);
}

TEST_F(EventSystemTest, QueueProcessingOrder) {
    std::vector<std::string> executionOrder;

    eventSystem_->subscribe("event1", [&](const EventData&) {
        executionOrder.push_back("event1");
    });

    eventSystem_->subscribe("event2", [&](const EventData&) {
        executionOrder.push_back("event2");
    });

    eventSystem_->subscribe("event3", [&](const EventData&) {
        executionOrder.push_back("event3");
    });

    // Queue events in specific order
    eventSystem_->queue("event1", EntityEventData{});
    eventSystem_->queue("event2", EntityEventData{});
    eventSystem_->queue("event3", EntityEventData{});

    eventSystem_->processQueue();

    // Events should be processed in FIFO order
    ASSERT_EQ(executionOrder.size(), 3);
    EXPECT_EQ(executionOrder[0], "event1");
    EXPECT_EQ(executionOrder[1], "event2");
    EXPECT_EQ(executionOrder[2], "event3");
}

TEST_F(EventSystemTest, ClearQueueDoesNotTriggerCallbacks) {
    int callCount = 0;

    eventSystem_->subscribe("test_event", [&](const EventData&) {
        callCount++;
    });

    eventSystem_->queue("test_event", EntityEventData{});
    eventSystem_->queue("test_event", EntityEventData{});
    eventSystem_->queue("test_event", EntityEventData{});

    EXPECT_EQ(callCount, 0);
    EXPECT_EQ(eventSystem_->queueSize(), 3);

    eventSystem_->clearQueue();

    EXPECT_EQ(callCount, 0);  // Callbacks should NOT have been called
    EXPECT_EQ(eventSystem_->queueSize(), 0);
}

TEST_F(EventSystemTest, UnsubscribeDuringCallback) {
    int callCount1 = 0;
    int callCount2 = 0;
    SubscriptionId id1, id2;

    // First callback unsubscribes itself
    id1 = eventSystem_->subscribe("test_event", [&](const EventData&) {
        callCount1++;
        eventSystem_->unsubscribe(id1);
    });

    id2 = eventSystem_->subscribe("test_event", [&](const EventData&) {
        callCount2++;
    });

    // First publish - both should be called
    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);

    // Second publish - only second callback should be called
    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(callCount1, 1);  // Should not increase
    EXPECT_EQ(callCount2, 2);  // Should increase
}

TEST_F(EventSystemTest, ResubscribeAfterUnsubscribe) {
    int callCount = 0;

    auto id = eventSystem_->subscribe("test_event", [&](const EventData&) {
        callCount++;
    });

    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(callCount, 1);

    eventSystem_->unsubscribe(id);

    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(callCount, 1);  // Should not increase

    // Resubscribe with new ID
    auto newId = eventSystem_->subscribe("test_event", [&](const EventData&) {
        callCount++;
    });

    EXPECT_NE(id, newId);  // Should have different ID

    eventSystem_->publish("test_event", EntityEventData{});
    EXPECT_EQ(callCount, 2);  // Should increase again
}

TEST_F(EventSystemTest, CommonEventTypeConstants) {
    // Test that common event type constants are accessible and can be used
    int collisionCount = 0;
    int triggerEnterCount = 0;
    int levelLoadedCount = 0;

    eventSystem_->subscribe(Events::Collision, [&](const EventData&) {
        collisionCount++;
    });

    eventSystem_->subscribe(Events::TriggerEnter, [&](const EventData&) {
        triggerEnterCount++;
    });

    eventSystem_->subscribe(Events::LevelLoaded, [&](const EventData&) {
        levelLoadedCount++;
    });

    eventSystem_->publish(Events::Collision, EntityEventData{});
    eventSystem_->publish(Events::TriggerEnter, EntityEventData{});
    eventSystem_->publish(Events::LevelLoaded, EntityEventData{});

    EXPECT_EQ(collisionCount, 1);
    EXPECT_EQ(triggerEnterCount, 1);
    EXPECT_EQ(levelLoadedCount, 1);
}

TEST_F(EventSystemTest, LargeNumberOfSubscribers) {
    // Stress test with many subscribers
    constexpr int NUM_SUBSCRIBERS = 1000;
    int totalCalls = 0;

    for (int i = 0; i < NUM_SUBSCRIBERS; ++i) {
        eventSystem_->subscribe("stress_test", [&](const EventData&) {
            totalCalls++;
        });
    }

    eventSystem_->publish("stress_test", EntityEventData{});

    EXPECT_EQ(totalCalls, NUM_SUBSCRIBERS);
}

TEST_F(EventSystemTest, LargeQueueSize) {
    // Stress test with large queue
    constexpr int QUEUE_SIZE = 10000;
    int callCount = 0;

    eventSystem_->subscribe("queued_event", [&](const EventData&) {
        callCount++;
    });

    for (int i = 0; i < QUEUE_SIZE; ++i) {
        eventSystem_->queue("queued_event", EntityEventData{});
    }

    EXPECT_EQ(eventSystem_->queueSize(), QUEUE_SIZE);

    eventSystem_->processQueue();

    EXPECT_EQ(callCount, QUEUE_SIZE);
    EXPECT_EQ(eventSystem_->queueSize(), 0);
}

//==============================================================================
// KANGARU DI INTEGRATION TESTS
//==============================================================================

TEST(EventSystemKangaruTest, ServiceInstantiation) {
    // Test that EventSystemService can be instantiated via Kangaru
    kgr::container container;

    // Verify the service can be invoked
    auto& eventSystem = container.service<EventSystemService>();

    // Verify it's the same instance (singleton behavior)
    auto& eventSystem2 = container.service<EventSystemService>();
    EXPECT_EQ(&eventSystem, &eventSystem2);
}

TEST(EventSystemKangaruTest, ServiceFunctionality) {
    // Test that the service instance works correctly
    kgr::container container;
    auto& eventSystem = container.service<EventSystemService>();

    // Test basic subscribe/publish functionality
    bool called = false;
    auto id = eventSystem.subscribe("test_event", [&](const EventData& data) {
        called = true;
    });

    EXPECT_FALSE(called);

    eventSystem.publish("test_event", EntityEventData{});
    EXPECT_TRUE(called);

    eventSystem.unsubscribe(id);
}

TEST(EventSystemKangaruTest, ServiceQueueFunctionality) {
    // Test queue operations through DI
    kgr::container container;
    auto& eventSystem = container.service<EventSystemService>();

    int callCount = 0;
    eventSystem.subscribe("queued_event", [&](const EventData& data) {
        callCount++;
    });

    eventSystem.queue("queued_event", EntityEventData{});
    eventSystem.queue("queued_event", EntityEventData{});

    EXPECT_EQ(callCount, 0);
    EXPECT_EQ(eventSystem.queueSize(), 2);

    eventSystem.processQueue();

    EXPECT_EQ(callCount, 2);
    EXPECT_EQ(eventSystem.queueSize(), 0);
}

TEST(EventSystemKangaruTest, MultipleContainers) {
    // Test that different containers have different singleton instances
    kgr::container container1;
    kgr::container container2;

    auto& eventSystem1 = container1.service<EventSystemService>();
    auto& eventSystem2 = container2.service<EventSystemService>();

    // Different containers should have different instances
    EXPECT_NE(&eventSystem1, &eventSystem2);

    // Each should maintain separate state
    int count1 = 0;
    int count2 = 0;

    eventSystem1.subscribe("test", [&](const EventData&) { count1++; });
    eventSystem2.subscribe("test", [&](const EventData&) { count2++; });

    eventSystem1.publish("test", EntityEventData{});
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 0);

    eventSystem2.publish("test", EntityEventData{});
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}

}  // namespace bestow::tests
