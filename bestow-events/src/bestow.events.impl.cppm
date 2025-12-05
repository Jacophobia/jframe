// bestow-events/src/bestow.events.impl.cppm
// Event system implementation

module;

export module bestow.events.impl;

import std;
import bestow.events;
import bestow.types;

export namespace bestow {

class EventSystem : public IEventSystem {
public:
    EventSystem() = default;
    ~EventSystem() override = default;

    //======================================================================
    // Publishing
    //======================================================================

    void publish(const EventType& type, const EventData& data) override {
        auto it = subscribers_.find(type);
        if (it != subscribers_.end()) {
            // Copy callbacks to avoid iterator invalidation if unsubscribe is called during callback execution
            std::vector<EventCallback> callbacks;
            callbacks.reserve(it->second.size());
            for (const auto& [id, callback] : it->second) {
                callbacks.push_back(callback);
            }

            // Execute callbacks from the copy
            for (const auto& callback : callbacks) {
                callback(data);
            }
        }
    }

    void queue(const EventType& type, const EventData& data) override {
        std::lock_guard lock(queueMutex_);
        eventQueue_.push({type, data});
    }

    //======================================================================
    // Subscribing
    //======================================================================

    SubscriptionId subscribe(const EventType& type, EventCallback callback) override {
        SubscriptionId id = nextSubscriptionId_++;
        subscribers_[type][id] = std::move(callback);
        subscriptionTypes_[id] = type;
        return id;
    }

    void unsubscribe(SubscriptionId id) override {
        auto typeIt = subscriptionTypes_.find(id);
        if (typeIt != subscriptionTypes_.end()) {
            auto& typeSubscribers = subscribers_[typeIt->second];
            typeSubscribers.erase(id);
            subscriptionTypes_.erase(typeIt);
        }
    }

    void unsubscribeAll(const EventType& type) override {
        auto it = subscribers_.find(type);
        if (it != subscribers_.end()) {
            for (const auto& [id, _] : it->second) {
                subscriptionTypes_.erase(id);
            }
            it->second.clear();
        }
    }

    //======================================================================
    // Processing
    //======================================================================

    void processQueue() override {
        std::queue<QueuedEvent> toProcess;

        {
            std::lock_guard lock(queueMutex_);
            std::swap(toProcess, eventQueue_);
        }

        while (!toProcess.empty()) {
            const auto& event = toProcess.front();
            publish(event.type, event.data);
            toProcess.pop();
        }
    }

    void clearQueue() override {
        std::lock_guard lock(queueMutex_);
        eventQueue_ = {};
    }

    std::size_t queueSize() const override {
        std::lock_guard lock(queueMutex_);
        return eventQueue_.size();
    }

private:
    struct QueuedEvent {
        EventType type;
        EventData data;
    };

    // Use std::map to maintain insertion order for callbacks
    std::unordered_map<EventType, std::map<SubscriptionId, EventCallback>> subscribers_;
    std::unordered_map<SubscriptionId, EventType> subscriptionTypes_;
    std::queue<QueuedEvent> eventQueue_;
    mutable std::mutex queueMutex_;
    SubscriptionId nextSubscriptionId_ = 1;
};

// Factory function (exported via namespace)
inline std::unique_ptr<IEventSystem> createEventSystem() {
    return std::make_unique<EventSystem>();
}

}  // namespace bestow
