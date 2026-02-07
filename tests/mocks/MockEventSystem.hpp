// tests/mocks/MockEventSystem.hpp
// Shared mock event system for testing

#pragma once

#include <kangaru/kangaru.hpp>

import std;
import bestow;
import bestow.types;

namespace bestow::tests {

class MockEventSystem : public IEventSystem {
public:
    // Tracking
    int publishCount = 0;

    // Delegates
    std::function<void(const EventType&, const EventData&)> onPublish =
        [this](const EventType&, const EventData&) { publishCount++; };
    std::function<void(const EventType&, const EventData&)> onQueue =
        [](const EventType&, const EventData&) {};
    std::function<SubscriptionId(const EventType&, EventCallback)> onSubscribe =
        [this](const EventType&, EventCallback) { return nextSubId_++; };
    std::function<void(SubscriptionId)> onUnsubscribe = [](SubscriptionId) {};
    std::function<void(const EventType&)> onUnsubscribeAll = [](const EventType&) {};
    std::function<void()> onProcessQueue = [] {};
    std::function<void()> onClearQueue = [] {};
    std::function<std::size_t()> onQueueSize = [] { return std::size_t{0}; };

    // IEventSystem overrides
    void publish(const EventType& type, const EventData& data) override {
        onPublish(type, data);
    }
    void queue(const EventType& type, const EventData& data) override {
        onQueue(type, data);
    }
    SubscriptionId subscribe(const EventType& type, EventCallback callback) override {
        return onSubscribe(type, callback);
    }
    void unsubscribe(SubscriptionId id) override { onUnsubscribe(id); }
    void unsubscribeAll(const EventType& type) override { onUnsubscribeAll(type); }
    void processQueue() override { onProcessQueue(); }
    void clearQueue() override { onClearQueue(); }
    std::size_t queueSize() const override { return onQueueSize(); }

private:
    SubscriptionId nextSubId_ = 1;
};

}  // namespace bestow::tests
