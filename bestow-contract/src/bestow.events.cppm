// bestow-contract/src/bestow.events.cppm
// Event system interface

module;

#include <cstddef>

export module bestow.events;

import bestow.types;

export namespace bestow {

class IEventSystem {
public:
    virtual ~IEventSystem() = default;

    //======================================================================
    // Publishing
    //======================================================================

    // Immediate dispatch
    virtual void publish(const EventType& type, const EventData& data) = 0;

    // Deferred dispatch (processed during processQueue)
    virtual void queue(const EventType& type, const EventData& data) = 0;

    //======================================================================
    // Subscribing
    //======================================================================

    virtual SubscriptionId subscribe(const EventType& type,
                                     EventCallback callback) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
    virtual void unsubscribeAll(const EventType& type) = 0;

    //======================================================================
    // Processing
    //======================================================================

    virtual void processQueue() = 0;
    virtual void clearQueue() = 0;
    virtual std::size_t queueSize() const = 0;
};

// Common event type constants
namespace Events {
    inline constexpr const char* Collision = "collision";
    inline constexpr const char* TriggerEnter = "trigger_enter";
    inline constexpr const char* TriggerExit = "trigger_exit";
    inline constexpr const char* LevelLoaded = "level_loaded";
    inline constexpr const char* LevelUnloaded = "level_unloaded";
    inline constexpr const char* EntityDamaged = "entity_damaged";
    inline constexpr const char* EntityDied = "entity_died";
    inline constexpr const char* ItemCollected = "item_collected";
    inline constexpr const char* Checkpoint = "checkpoint";
    inline constexpr const char* PlayerDeath = "player_death";
    inline constexpr const char* GameSaved = "game_saved";
    inline constexpr const char* GameLoaded = "game_loaded";
}

}  // namespace bestow
