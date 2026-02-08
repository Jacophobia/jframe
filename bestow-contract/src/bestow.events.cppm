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
    inline constexpr const char* ScenePushed = "scene_pushed";
    inline constexpr const char* ScenePopped = "scene_popped";
    inline constexpr const char* SceneReplaced = "scene_replaced";
    inline constexpr const char* SceneReloaded = "scene_reloaded";
    inline constexpr const char* EntityDamaged = "entity_damaged";
    inline constexpr const char* EntityDied = "entity_died";
    inline constexpr const char* ItemCollected = "item_collected";
    inline constexpr const char* Checkpoint = "checkpoint";
    inline constexpr const char* PlayerDeath = "player_death";
    inline constexpr const char* GameSaved = "game_saved";
    inline constexpr const char* GameLoaded = "game_loaded";

    // Physics 3D events
    inline constexpr const char* Collision3D = "collision_3d";
    inline constexpr const char* TriggerEnter3D = "trigger_enter_3d";
    inline constexpr const char* TriggerExit3D = "trigger_exit_3d";

    // Asset System events
    inline constexpr const char* AssetLoaded = "asset_loaded";
    inline constexpr const char* AssetUnloaded = "asset_unloaded";
    inline constexpr const char* AssetReloaded = "asset_reloaded";
    inline constexpr const char* AssetError = "asset_error";

    // Config System events
    inline constexpr const char* ConfigChanged = "config_changed";

    // Shader System events
    inline constexpr const char* ShaderReloaded = "shader_reloaded";
    inline constexpr const char* MaterialReloaded = "material_reloaded";

    // Game State events
    inline constexpr const char* StateChanged = "state_changed";
    inline constexpr const char* StatePushed = "state_pushed";
    inline constexpr const char* StatePopped = "state_popped";

    // Dev tools events
    inline constexpr const char* FileChanged = "file_changed";

    // Input Action events (event-driven input system)
    inline constexpr const char* ActionTriggered = "action_triggered";

    // Phase events (input phase system)
    inline constexpr const char* PhaseChanged = "phase_changed";
    inline constexpr const char* PhasePushed = "phase_pushed";
    inline constexpr const char* PhasePopped = "phase_popped";
}

}  // namespace bestow
