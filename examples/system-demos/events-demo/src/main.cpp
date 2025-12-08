// Events System Demo
// Comprehensive demonstration of the IEventSystem interface
//
// This demo exercises ALL methods from bestow.events:
// - publish() - Immediate event dispatch
// - queue() - Deferred event dispatch
// - subscribe() - Register event listeners
// - unsubscribe() - Remove specific listener
// - unsubscribeAll() - Remove all listeners for a type
// - processQueue() - Process deferred events
// - clearQueue() - Discard queued events
// - queueSize() - Get number of queued events
//
// Event types demonstrated:
// - Events::Collision, Events::TriggerEnter, Events::TriggerExit
// - Events::LevelLoaded, Events::LevelUnloaded
// - Events::EntityDamaged, Events::EntityDied
// - Events::ItemCollected, Events::Checkpoint
// - Custom event types
//
// EventData variants demonstrated:
// - EntityEventData
// - DamageEventData
// - LevelEventData
// - CollisionEvent
// - TriggerEvent
// - std::any for custom data

#include <kangaru/kangaru.hpp>

import std;
import bestow.events;
import bestow.events.impl;
import bestow.types;

using namespace bestow;

// Custom event data structure
struct PlayerLevelUpData {
    int newLevel;
    int experiencePoints;
    std::string className;
};

// Helper function to print section headers
void printSection(const std::string& title) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(80, '=') << "\n\n";
}

// Helper function to print subsection headers
void printSubsection(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n\n";
}

int main() {
    std::cout << "Bestow Events System - Comprehensive API Demo\n";
    std::cout << "==============================================\n";

    // Create the event system using DI container
    kgr::container container;
    auto& events = container.service<EventSystemService>();

    // Track event counts for demonstration
    int collisionCount = 0;
    int damageCount = 0;
    int levelLoadCount = 0;
    int triggerCount = 0;
    int customEventCount = 0;

    //==========================================================================
    // SECTION 1: Basic Event Publishing and Subscribing
    //==========================================================================
    printSection("SECTION 1: Basic Event Publishing and Subscribing");

    printSubsection("1.1 Subscribe to Events::Collision");

    auto collisionSubId = events.subscribe(Events::Collision,
        [&](const EventData& data) {
            collisionCount++;
            if (std::holds_alternative<CollisionEvent>(data)) {
                const auto& collision = std::get<CollisionEvent>(data);
                std::cout << "  [CALLBACK] Collision detected!\n";
                std::cout << "    Entity A: " << static_cast<int>(collision.entityA) << "\n";
                std::cout << "    Entity B: " << static_cast<int>(collision.entityB) << "\n";
                std::cout << "    Contact Point: (" << collision.contactPoint.x
                         << ", " << collision.contactPoint.y << ")\n";
                std::cout << "    Normal: (" << collision.normal.x
                         << ", " << collision.normal.y << ")\n";
                std::cout << "    Impulse: " << collision.impulse << "\n";
            }
        }
    );

    std::cout << "Subscription ID: " << collisionSubId << "\n";

    printSubsection("1.2 Publish Immediate Collision Event");

    CollisionEvent collision{
        .entityA = Entity{42},
        .entityB = Entity{17},
        .contactPoint = Vec2{100.0f, 200.0f},
        .normal = Vec2{0.0f, -1.0f},
        .impulse = 150.5f
    };

    events.publish(Events::Collision, collision);
    std::cout << "Total collision events handled: " << collisionCount << "\n";

    //==========================================================================
    // SECTION 2: Multiple Subscribers
    //==========================================================================
    printSection("SECTION 2: Multiple Subscribers to Same Event");

    printSubsection("2.1 Add Second Collision Subscriber");

    auto collisionSubId2 = events.subscribe(Events::Collision,
        [&](const EventData& data) {
            std::cout << "  [CALLBACK 2] Second handler also received collision!\n";
        }
    );

    std::cout << "Second subscription ID: " << collisionSubId2 << "\n";

    printSubsection("2.2 Publish - Both Subscribers Receive Event");

    CollisionEvent collision2{
        .entityA = Entity{99},
        .entityB = Entity{88},
        .contactPoint = Vec2{50.0f, 75.0f},
        .normal = Vec2{1.0f, 0.0f},
        .impulse = 75.2f
    };

    events.publish(Events::Collision, collision2);
    std::cout << "Total collision events handled: " << collisionCount << "\n";

    //==========================================================================
    // SECTION 3: Entity and Damage Events
    //==========================================================================
    printSection("SECTION 3: EntityEventData and DamageEventData");

    printSubsection("3.1 Subscribe to Entity Damage Events");

    auto damageSubId = events.subscribe(Events::EntityDamaged,
        [&](const EventData& data) {
            damageCount++;
            if (std::holds_alternative<DamageEventData>(data)) {
                const auto& dmg = std::get<DamageEventData>(data);
                std::cout << "  [CALLBACK] Entity Damaged!\n";
                std::cout << "    Target: " << static_cast<int>(dmg.target) << "\n";
                std::cout << "    Source: " << static_cast<int>(dmg.source) << "\n";
                std::cout << "    Damage Amount: " << dmg.amount << "\n";
                std::cout << "    Knockback: (" << dmg.knockback.x
                         << ", " << dmg.knockback.y << ")\n";
            }
        }
    );

    printSubsection("3.2 Publish Damage Event");

    DamageEventData damage{
        .target = Entity{5},
        .source = Entity{10},
        .amount = 25,
        .knockback = Vec2{50.0f, -100.0f}
    };

    events.publish(Events::EntityDamaged, damage);

    printSubsection("3.3 Subscribe to Entity Death Events");

    auto deathSubId = events.subscribe(Events::EntityDied,
        [&](const EventData& data) {
            if (std::holds_alternative<EntityEventData>(data)) {
                const auto& entity = std::get<EntityEventData>(data);
                std::cout << "  [CALLBACK] Entity Died!\n";
                std::cout << "    Entity: " << static_cast<int>(entity.entity) << "\n";
                if (entity.otherEntity) {
                    std::cout << "    Killer: " << static_cast<int>(*entity.otherEntity) << "\n";
                }
            }
        }
    );

    printSubsection("3.4 Publish Death Event");

    EntityEventData death{
        .entity = Entity{5},
        .otherEntity = Entity{10}
    };

    events.publish(Events::EntityDied, death);
    std::cout << "Total damage events handled: " << damageCount << "\n";

    //==========================================================================
    // SECTION 4: Trigger Events
    //==========================================================================
    printSection("SECTION 4: Trigger Events (TriggerEnter/TriggerExit)");

    printSubsection("4.1 Subscribe to Trigger Events");

    auto triggerEnterSubId = events.subscribe(Events::TriggerEnter,
        [&](const EventData& data) {
            triggerCount++;
            if (std::holds_alternative<TriggerEvent>(data)) {
                const auto& trigger = std::get<TriggerEvent>(data);
                std::cout << "  [CALLBACK] Trigger Enter!\n";
                std::cout << "    Entity A: " << static_cast<int>(trigger.entityA) << "\n";
                std::cout << "    Entity B: " << static_cast<int>(trigger.entityB) << "\n";
                std::cout << "    Contact Point: (" << trigger.contactPoint.x
                         << ", " << trigger.contactPoint.y << ")\n";
            }
        }
    );

    auto triggerExitSubId = events.subscribe(Events::TriggerExit,
        [&](const EventData& data) {
            if (std::holds_alternative<TriggerEvent>(data)) {
                const auto& trigger = std::get<TriggerEvent>(data);
                std::cout << "  [CALLBACK] Trigger Exit!\n";
                std::cout << "    Entity A: " << static_cast<int>(trigger.entityA) << "\n";
                std::cout << "    Entity B: " << static_cast<int>(trigger.entityB) << "\n";
            }
        }
    );

    printSubsection("4.2 Publish Trigger Events");

    TriggerEvent enterTrigger{
        .entityA = Entity{1}, // Player
        .entityB = Entity{20}, // Checkpoint trigger
        .contactPoint = Vec2{300.0f, 400.0f}
    };

    events.publish(Events::TriggerEnter, enterTrigger);

    TriggerEvent exitTrigger{
        .entityA = Entity{1},
        .entityB = Entity{20}
    };

    events.publish(Events::TriggerExit, exitTrigger);
    std::cout << "Total trigger events handled: " << triggerCount << "\n";

    //==========================================================================
    // SECTION 5: Level Events
    //==========================================================================
    printSection("SECTION 5: Level Events (LevelLoaded/LevelUnloaded)");

    printSubsection("5.1 Subscribe to Level Events");

    auto levelLoadedSubId = events.subscribe(Events::LevelLoaded,
        [&](const EventData& data) {
            levelLoadCount++;
            if (std::holds_alternative<LevelEventData>(data)) {
                const auto& level = std::get<LevelEventData>(data);
                std::cout << "  [CALLBACK] Level Loaded!\n";
                std::cout << "    Level ID: " << level.levelId << "\n";
                std::cout << "    Event Type: "
                         << static_cast<int>(level.event) << "\n";
            }
        }
    );

    auto levelUnloadedSubId = events.subscribe(Events::LevelUnloaded,
        [&](const EventData& data) {
            if (std::holds_alternative<LevelEventData>(data)) {
                const auto& level = std::get<LevelEventData>(data);
                std::cout << "  [CALLBACK] Level Unloaded!\n";
                std::cout << "    Level ID: " << level.levelId << "\n";
            }
        }
    );

    printSubsection("5.2 Publish Level Events");

    LevelEventData levelLoad{
        .levelId = 12345,
        .event = LevelEvent::LoadCompleted
    };

    events.publish(Events::LevelLoaded, levelLoad);

    LevelEventData levelUnload{
        .levelId = 12345,
        .event = LevelEvent::UnloadCompleted
    };

    events.publish(Events::LevelUnloaded, levelUnload);
    std::cout << "Total level load events handled: " << levelLoadCount << "\n";

    //==========================================================================
    // SECTION 6: Custom Events with std::any
    //==========================================================================
    printSection("SECTION 6: Custom Events with std::any");

    printSubsection("6.1 Define Custom Event Type");
    std::cout << "Custom event type: \"player_level_up\"\n";
    std::cout << "Custom data structure: PlayerLevelUpData\n";

    printSubsection("6.2 Subscribe to Custom Event");

    const EventType PLAYER_LEVEL_UP = "player_level_up";

    auto levelUpSubId = events.subscribe(PLAYER_LEVEL_UP,
        [&](const EventData& data) {
            customEventCount++;
            if (std::holds_alternative<std::any>(data)) {
                const auto& anyData = std::get<std::any>(data);
                if (anyData.type() == typeid(PlayerLevelUpData)) {
                    const auto& levelUp = std::any_cast<const PlayerLevelUpData&>(anyData);
                    std::cout << "  [CALLBACK] Player Leveled Up!\n";
                    std::cout << "    New Level: " << levelUp.newLevel << "\n";
                    std::cout << "    Experience Points: " << levelUp.experiencePoints << "\n";
                    std::cout << "    Class: " << levelUp.className << "\n";
                }
            }
        }
    );

    printSubsection("6.3 Publish Custom Event");

    PlayerLevelUpData levelUpData{
        .newLevel = 5,
        .experiencePoints = 1250,
        .className = "Warrior"
    };

    events.publish(PLAYER_LEVEL_UP, std::any(levelUpData));

    //==========================================================================
    // SECTION 7: Deferred Events (Queue/Process)
    //==========================================================================
    printSection("SECTION 7: Deferred Events (Queue and Process)");

    printSubsection("7.1 Check Initial Queue Size");
    std::cout << "Queue size before queuing: " << events.queueSize() << "\n";

    printSubsection("7.2 Queue Multiple Events (Not Processed Yet)");

    std::cout << "Queuing 3 collision events...\n";
    for (int i = 0; i < 3; ++i) {
        CollisionEvent queuedCollision{
            .entityA = Entity{static_cast<std::uint32_t>(100 + i)},
            .entityB = Entity{static_cast<std::uint32_t>(200 + i)},
            .contactPoint = Vec2{static_cast<float>(i * 10), static_cast<float>(i * 20)},
            .normal = Vec2{0.0f, -1.0f},
            .impulse = static_cast<float>(i * 5.5f)
        };
        events.queue(Events::Collision, queuedCollision);
    }

    std::cout << "Queue size after queuing: " << events.queueSize() << "\n";
    std::cout << "Note: Events are queued but not yet processed.\n";
    std::cout << "Collision count (should be unchanged): " << collisionCount << "\n";

    printSubsection("7.3 Process Queued Events");

    std::cout << "Processing queue...\n\n";
    events.processQueue();

    std::cout << "\nQueue size after processing: " << events.queueSize() << "\n";
    std::cout << "Collision count (should be +3): " << collisionCount << "\n";

    printSubsection("7.4 Queue Events Then Clear Without Processing");

    std::cout << "Queuing 2 damage events...\n";
    for (int i = 0; i < 2; ++i) {
        DamageEventData queuedDamage{
            .target = Entity{static_cast<std::uint32_t>(50 + i)},
            .source = Entity{static_cast<std::uint32_t>(60 + i)},
            .amount = 10 + i * 5,
            .knockback = Vec2{10.0f, -20.0f}
        };
        events.queue(Events::EntityDamaged, queuedDamage);
    }

    std::cout << "Queue size before clear: " << events.queueSize() << "\n";
    std::cout << "Damage count before clear: " << damageCount << "\n";

    std::cout << "\nClearing queue without processing...\n";
    events.clearQueue();

    std::cout << "Queue size after clear: " << events.queueSize() << "\n";
    std::cout << "Damage count after clear (unchanged): " << damageCount << "\n";

    //==========================================================================
    // SECTION 8: Unsubscribing
    //==========================================================================
    printSection("SECTION 8: Unsubscribing from Events");

    printSubsection("8.1 Unsubscribe Single Listener");

    std::cout << "Unsubscribing second collision handler (ID: "
              << collisionSubId2 << ")...\n";
    events.unsubscribe(collisionSubId2);

    std::cout << "\nPublishing collision event...\n";
    CollisionEvent testCollision{
        .entityA = Entity{777},
        .entityB = Entity{888},
        .contactPoint = Vec2{0.0f, 0.0f},
        .normal = Vec2{0.0f, 1.0f},
        .impulse = 25.0f
    };

    events.publish(Events::Collision, testCollision);
    std::cout << "Note: Only first handler should have fired.\n";

    printSubsection("8.2 Subscribe Multiple, Then Unsubscribe All");

    std::cout << "Adding 3 new subscribers to ItemCollected event...\n";

    auto itemSub1 = events.subscribe(Events::ItemCollected,
        [](const EventData&) { std::cout << "  [CALLBACK 1] Item collected!\n"; });

    auto itemSub2 = events.subscribe(Events::ItemCollected,
        [](const EventData&) { std::cout << "  [CALLBACK 2] Item collected!\n"; });

    auto itemSub3 = events.subscribe(Events::ItemCollected,
        [](const EventData&) { std::cout << "  [CALLBACK 3] Item collected!\n"; });

    std::cout << "Subscription IDs: " << itemSub1 << ", "
              << itemSub2 << ", " << itemSub3 << "\n";

    std::cout << "\nPublishing ItemCollected event...\n";
    EntityEventData itemData{.entity = Entity{15}};
    events.publish(Events::ItemCollected, itemData);

    std::cout << "\nUnsubscribing all ItemCollected listeners...\n";
    events.unsubscribeAll(Events::ItemCollected);

    std::cout << "\nPublishing ItemCollected event again...\n";
    events.publish(Events::ItemCollected, itemData);
    std::cout << "Note: No callbacks should have fired.\n";

    //==========================================================================
    // SECTION 9: Event Chaining (Events Publishing Other Events)
    //==========================================================================
    printSection("SECTION 9: Event Chaining (Events Triggering Other Events)");

    printSubsection("9.1 Set Up Chain: Checkpoint -> Save Game");

    int checkpointCount = 0;
    int gameSavedCount = 0;

    auto checkpointSubId = events.subscribe(Events::Checkpoint,
        [&](const EventData& data) {
            checkpointCount++;
            std::cout << "  [CALLBACK] Checkpoint reached!\n";

            // Trigger a save game event
            std::cout << "  Triggering game save...\n";
            EntityEventData saveData{.entity = Entity{0}};
            events.publish(Events::GameSaved, saveData);
        }
    );

    auto gameSavedSubId = events.subscribe(Events::GameSaved,
        [&](const EventData&) {
            gameSavedCount++;
            std::cout << "    [CALLBACK] Game saved successfully!\n";
        }
    );

    printSubsection("9.2 Trigger Checkpoint Event");

    EntityEventData checkpointData{.entity = Entity{1}};
    events.publish(Events::Checkpoint, checkpointData);

    std::cout << "\nCheckpoint events: " << checkpointCount << "\n";
    std::cout << "Game saved events: " << gameSavedCount << "\n";
    std::cout << "Note: One checkpoint triggered one save.\n";

    //==========================================================================
    // SECTION 10: Summary and Statistics
    //==========================================================================
    printSection("SECTION 10: Summary and Event Statistics");

    std::cout << "Event Type                | Count\n";
    std::cout << "--------------------------|-------\n";
    std::cout << std::format("Collision                 | {}\n", collisionCount);
    std::cout << std::format("Entity Damage             | {}\n", damageCount);
    std::cout << std::format("Level Load                | {}\n", levelLoadCount);
    std::cout << std::format("Trigger Events            | {}\n", triggerCount);
    std::cout << std::format("Custom Events             | {}\n", customEventCount);
    std::cout << std::format("Checkpoint                | {}\n", checkpointCount);
    std::cout << std::format("Game Saved                | {}\n", gameSavedCount);

    printSubsection("API Methods Demonstrated");
    std::cout << "[✓] publish() - Immediate event dispatch\n";
    std::cout << "[✓] queue() - Deferred event dispatch\n";
    std::cout << "[✓] subscribe() - Register event listeners\n";
    std::cout << "[✓] unsubscribe() - Remove specific listener\n";
    std::cout << "[✓] unsubscribeAll() - Remove all listeners for type\n";
    std::cout << "[✓] processQueue() - Process deferred events\n";
    std::cout << "[✓] clearQueue() - Discard queued events\n";
    std::cout << "[✓] queueSize() - Get queue size\n";

    printSubsection("Event Types Demonstrated");
    std::cout << "[✓] Events::Collision\n";
    std::cout << "[✓] Events::TriggerEnter\n";
    std::cout << "[✓] Events::TriggerExit\n";
    std::cout << "[✓] Events::LevelLoaded\n";
    std::cout << "[✓] Events::LevelUnloaded\n";
    std::cout << "[✓] Events::EntityDamaged\n";
    std::cout << "[✓] Events::EntityDied\n";
    std::cout << "[✓] Events::ItemCollected\n";
    std::cout << "[✓] Events::Checkpoint\n";
    std::cout << "[✓] Events::GameSaved\n";
    std::cout << "[✓] Custom event types (player_level_up)\n";

    printSubsection("EventData Variants Demonstrated");
    std::cout << "[✓] CollisionEvent\n";
    std::cout << "[✓] TriggerEvent\n";
    std::cout << "[✓] EntityEventData\n";
    std::cout << "[✓] DamageEventData\n";
    std::cout << "[✓] LevelEventData\n";
    std::cout << "[✓] std::any (for custom data)\n";

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Events System Demo Complete!\n";
    std::cout << "All IEventSystem API methods have been demonstrated.\n";
    std::cout << std::string(80, '=') << "\n\n";

    return 0;
}
