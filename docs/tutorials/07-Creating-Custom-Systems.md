# Tutorial 7: Creating Custom Systems

In this tutorial, you'll learn how to create your own custom systems in Bestow to extend the engine with new functionality. This is an advanced topic that requires understanding Bestow's contract-based architecture.

## Understanding Contract-Based Architecture

Bestow follows a strict contract-based architecture where **systems ONLY depend on interfaces, never implementations**. This is the most fundamental principle in Bestow.

### Why Contract-Based?

1. **Complete Interchangeability** - Clients can swap any system implementation
2. **Clean Boundaries** - Clear separation between interface and implementation
3. **Testing** - Easy to mock interfaces for unit tests
4. **Flexibility** - Multiple implementations can coexist (e.g., OpenGL vs Vulkan)

### The Contract Rule

**ALL systems MUST ONLY depend on `bestow-contract` interfaces. NEVER depend on other system implementations.**

```cpp
// ✅ CORRECT - System depends only on contract interfaces
class MySystem : public IMySystem {
    IAssetSystem* assets_;     // Interface dependency - GOOD
    IEntitySystem* entities_;  // Interface dependency - GOOD
};

// ❌ WRONG - System depends on implementation
class MySystem : public IMySystem {
    AssetSystem* assets_;      // Implementation dependency - FORBIDDEN
    EntityManager* entities_;  // Implementation dependency - FORBIDDEN
};
```

## Step 1: Design Your System Interface

First, decide what your system will do. For this tutorial, we'll create a **Quest System** that tracks player objectives.

Think about:
- What operations does the system need to support?
- What data does it manage?
- What other systems does it need to interact with?

## Step 2: Define the Contract Interface

Create your interface in your game code (not in bestow-contract, since this is game-specific):

```cpp
// src/IQuestSystem.h
#pragma once

import std;

namespace mygame {

struct Quest {
    std::string id;
    std::string title;
    std::string description;
    bool isComplete = false;
    int progress = 0;
    int goal = 1;
};

class IQuestSystem {
public:
    virtual ~IQuestSystem() = default;

    // Quest management
    virtual void addQuest(const Quest& quest) = 0;
    virtual void removeQuest(const std::string& questId) = 0;
    virtual bool hasQuest(const std::string& questId) const = 0;

    // Quest progress
    virtual void updateProgress(const std::string& questId, int progress) = 0;
    virtual void completeQuest(const std::string& questId) = 0;

    // Query
    virtual std::vector<Quest> getActiveQuests() const = 0;
    virtual std::vector<Quest> getCompletedQuests() const = 0;
    virtual const Quest* getQuest(const std::string& questId) const = 0;
};

} // namespace mygame
```

## Step 3: Using the BESTOW_SYSTEM Macro

The `BESTOW_SYSTEM` macro combines class declaration, service definition, constructor, and member variables into one declaration:

```cpp
// src/QuestSystem.h
#pragma once

// Global module fragment - include third-party headers
module;

#include <kangaru/kangaru.hpp>
#include <bestow/kangaru_macros.hpp>

export module mygame.quest;

import std;
import bestow;
import bestow.events;

export namespace mygame {

// Forward declare the interface
class IQuestSystem;

// BESTOW_SYSTEM generates:
// - class QuestSystem : public IQuestSystem {
// - nested QuestSystem::Service struct for Kangaru registration
// - constructor: explicit QuestSystem(IEventSystem* pIEventSystem = nullptr)
// - private member: IEventSystem* pIEventSystem_ = nullptr;

BESTOW_SYSTEM(QuestSystem, IQuestSystem, bestow::IEventSystem) {
public:
    ~QuestSystem() override = default;

    // Interface implementation
    void addQuest(const Quest& quest) override {
        quests_[quest.id] = quest;

        // Use injected dependency
        if (pIEventSystem_) {
            pIEventSystem_->publish(Events::QuestAdded, quest.id);
        }
    }

    void removeQuest(const std::string& questId) override {
        quests_.erase(questId);
    }

    bool hasQuest(const std::string& questId) const override {
        return quests_.contains(questId);
    }

    void updateProgress(const std::string& questId, int progress) override {
        if (auto it = quests_.find(questId); it != quests_.end()) {
            it->second.progress = progress;

            // Auto-complete if goal reached
            if (progress >= it->second.goal) {
                completeQuest(questId);
            }
        }
    }

    void completeQuest(const std::string& questId) override {
        if (auto it = quests_.find(questId); it != quests_.end()) {
            it->second.isComplete = true;

            if (pIEventSystem_) {
                pIEventSystem_->publish(Events::QuestCompleted, questId);
            }
        }
    }

    std::vector<Quest> getActiveQuests() const override {
        std::vector<Quest> result;
        for (const auto& [id, quest] : quests_) {
            if (!quest.isComplete) {
                result.push_back(quest);
            }
        }
        return result;
    }

    std::vector<Quest> getCompletedQuests() const override {
        std::vector<Quest> result;
        for (const auto& [id, quest] : quests_) {
            if (quest.isComplete) {
                result.push_back(quest);
            }
        }
        return result;
    }

    const Quest* getQuest(const std::string& questId) const override {
        if (auto it = quests_.find(questId); it != quests_.end()) {
            return &it->second;
        }
        return nullptr;
    }

private:
    std::unordered_map<std::string, Quest> quests_;

    // pIEventSystem_ is automatically generated by BESTOW_SYSTEM macro
};

} // namespace mygame
```

### Understanding Member Variable Naming

The `BESTOW_SYSTEM` macro generates member variables with this naming convention:
- Injected dependencies become `p<TypeName>_`
- The 'p' prefix indicates pointer
- Use the full type name (including 'I' for interfaces)

Example:
```cpp
BESTOW_SYSTEM(QuestSystem, IQuestSystem, bestow::IEventSystem, bestow::IAssetSystem)
// Generates:
//   bestow::IEventSystem* pIEventSystem_ = nullptr;
//   bestow::IAssetSystem* pIAssetSystem_ = nullptr;
```

## Step 4: Multiple Dependencies

Your system can depend on multiple interfaces. Let's expand our QuestSystem to save quest progress:

```cpp
// Depends on both EventSystem and SaveSystem
BESTOW_SYSTEM(QuestSystem, IQuestSystem, bestow::IEventSystem, bestow::ISaveSystem) {
public:
    ~QuestSystem() override = default;

    void addQuest(const Quest& quest) override {
        quests_[quest.id] = quest;

        // Use injected event system
        if (pIEventSystem_) {
            pIEventSystem_->publish(Events::QuestAdded, quest.id);
        }

        // Save progress
        saveQuestProgress();
    }

    void loadQuestProgress() {
        // Use injected save system
        if (!pISaveSystem_) return;

        auto saveData = pISaveSystem_->loadSlot(0);
        if (saveData) {
            // Deserialize quests from save data
            // Implementation details omitted for brevity
        }
    }

private:
    void saveQuestProgress() {
        if (!pISaveSystem_) return;

        // Serialize quests to save data
        // Implementation details omitted for brevity
    }

    std::unordered_map<std::string, Quest> quests_;

    // Auto-generated by macro:
    // bestow::IEventSystem* pIEventSystem_ = nullptr;
    // bestow::ISaveSystem* pISaveSystem_ = nullptr;
};
```

## Step 5: Register with Dependency Injection

Now register your system with the Kangaru container. This is typically done in your game's initialization code:

```cpp
// src/main.cpp
import std;
import bestow;
import bestow.core;
import mygame.quest;

int main(int argc, char* argv[]) {
    // Create Kangaru container
    kgr::container container;

    // Register Bestow systems
    container.service<bestow::IEventSystem::Service>();
    container.service<bestow::ISaveSystem::Service>();

    // Register your custom system
    container.service<mygame::QuestSystem::Service>();

    // Kangaru will automatically inject dependencies!
    auto questSystem = container.service<mygame::IQuestSystem>();

    // Use the system
    questSystem->addQuest(Quest{
        .id = "tutorial_quest",
        .title = "Learn the Basics",
        .description = "Complete the tutorial",
        .goal = 5
    });

    return 0;
}
```

## Step 6: Access in Your Application

Inject your custom system into your game application:

```cpp
// src/Game.h
import bestow.core;
import mygame.quest;

class MyGame : public bestow::core::Application {
public:
    MyGame(
        bestow::core::Engine& engine,
        mygame::IQuestSystem* questSystem
    ) : engine_(&engine), questSystem_(questSystem) {}

    bool initialize() override {
        // Load quests from Lua
        loadQuestsFromLua();
        return true;
    }

    void updateFixed(bestow::DeltaTime dt) override {
        // Game logic that might complete quests
        checkQuestObjectives();
    }

private:
    void checkQuestObjectives() {
        // Example: Update quest progress based on game state
        if (hasCollectedCoin()) {
            questSystem_->updateProgress("collect_coins", coinsCollected_);
        }
    }

    void loadQuestsFromLua() {
        // Load quest definitions from Lua files
        // See Tutorial 6 for Lua integration details
    }

    bestow::core::Engine* engine_;
    mygame::IQuestSystem* questSystem_;
    int coinsCollected_ = 0;
};
```

## Step 7: Make It Data-Driven with Lua

Define your quests in Lua instead of C++:

```lua
-- data/quests/tutorial.lua
return {
    id = "tutorial_quest",
    title = "Learn the Basics",
    description = "Complete the tutorial by collecting 5 coins",
    goal = 5,

    objectives = {
        { type = "collect", item = "coin", count = 5 }
    },

    rewards = {
        gold = 100,
        experience = 50
    }
}
```

Load quests from Lua in your system:

```cpp
void QuestSystem::loadQuestFromLua(const std::filesystem::path& path) {
    if (!pIAssetSystem_) return;

    // Register and load the Lua file as a Data asset
    auto handle = pIAssetSystem_->registerAsset(
        bestow::AssetType::Data,
        path
    );
    pIAssetSystem_->loadAsset(handle);

    // Get the loaded data
    auto* dataAsset = pIAssetSystem_->getAsset<bestow::DataAsset>(handle);
    if (!dataAsset) return;

    // Parse quest from Lua data
    Quest quest;
    quest.id = getJsonValue<std::string>(*dataAsset, "id", "");
    quest.title = getJsonValue<std::string>(*dataAsset, "title", "");
    quest.description = getJsonValue<std::string>(*dataAsset, "description", "");
    quest.goal = getJsonValue<int>(*dataAsset, "goal", 1);

    addQuest(quest);
}
```

## Step 8: Event-Based Communication

Use EventSystem for decoupled communication between systems:

```cpp
// In your game initialization
void Game::setupQuestEventHandlers() {
    auto& events = engine_->systems().events;

    // Subscribe to quest completion events
    questCompleteSubId_ = events->subscribe(
        Events::QuestCompleted,
        [this](const bestow::EventData& data) {
            auto questId = std::get<std::string>(data);
            onQuestCompleted(questId);
        }
    );

    // Subscribe to collision events to track quest objectives
    collisionSubId_ = events->subscribe(
        Events::Collision,
        [this](const bestow::EventData& data) {
            auto& collision = std::get<bestow::CollisionEvent>(data);
            checkQuestObjectiveCollision(collision);
        }
    );
}

void Game::onQuestCompleted(const std::string& questId) {
    auto* quest = questSystem_->getQuest(questId);
    if (!quest) return;

    // Show completion UI, give rewards, etc.
    showQuestCompleteUI(*quest);
    giveQuestRewards(*quest);
}

void Game::checkQuestObjectiveCollision(const bestow::CollisionEvent& collision) {
    // Example: Collecting a coin completes quest objective
    if (isPlayer(collision.entityA) && isCoin(collision.entityB)) {
        questSystem_->updateProgress("collect_coins", ++coinsCollected_);
    }
}
```

## Advanced: Hot Reload Support

Enable hot reload for your quest definitions in debug builds:

```cpp
#if defined(BESTOW_DEV_TOOLS)
void QuestSystem::enableHotReload() {
    if (!pIAssetSystem_) return;

    pIAssetSystem_->enableHotReload(true);

    // Subscribe to asset changes
    assetChangeSubId_ = pIAssetSystem_->subscribeToType(
        bestow::AssetType::Data,
        [this](bestow::AssetHandle handle, bestow::AssetType type) {
            auto metadata = pIAssetSystem_->getAssetMetadata(handle);

            // Check if this is a quest file
            if (metadata.path.string().find("quests/") != std::string::npos) {
                reloadQuestFromAsset(handle);
            }
        }
    );
}
#endif
```

Now you can edit `data/quests/tutorial.lua` and see changes instantly without recompiling!

## Best Practices

### 1. Program to Interfaces Only

Never include implementation headers from other systems:

```cpp
// ❌ WRONG
#include "bestow-assets/AssetSystem.h"
#include "bestow-entity/EntityManager.h"

// ✅ CORRECT
import bestow.assets;  // Only the interface
import bestow.entity;  // Only the interface
```

### 2. Use Dependency Injection

Never create system dependencies directly:

```cpp
// ❌ WRONG
class QuestSystem {
    EventSystem* events_ = new EventSystem();  // Direct creation!
};

// ✅ CORRECT
BESTOW_SYSTEM(QuestSystem, IQuestSystem, IEventSystem) {
    // IEventSystem* pIEventSystem_ injected by DI
};
```

### 3. Keep Systems Focused

Each system should have a single responsibility:

- **QuestSystem** - Manages quests only
- **InventorySystem** - Manages items only
- **DialogueSystem** - Manages conversations only

Don't create a "GameplaySystem" that does everything.

### 4. Make It Data-Driven

Put configuration in Lua, not C++:

```cpp
// ❌ WRONG - Hardcoded in C++
quest.goal = 5;
quest.title = "Collect Coins";

// ✅ CORRECT - Loaded from Lua
loadQuestFromLua("data/quests/tutorial.lua");
```

### 5. Use Events for Decoupling

Prefer EventSystem over direct callbacks:

```cpp
// ❌ WRONG - Tight coupling
class QuestSystem {
    std::function<void(Quest)> onQuestComplete_;
};

// ✅ CORRECT - Decoupled via events
void QuestSystem::completeQuest(const std::string& questId) {
    pIEventSystem_->publish(Events::QuestCompleted, questId);
}
```

## Common Patterns

### Singleton System (Most Common)

```cpp
// Single instance managed by DI container
BESTOW_SYSTEM(QuestSystem, IQuestSystem, IEventSystem) {
    // Kangaru ensures only one instance exists
};
```

### System with No Dependencies

```cpp
// System that doesn't need other systems
BESTOW_SYSTEM(MathUtilsSystem, IMathUtilsSystem) {
public:
    float lerp(float a, float b, float t) override {
        return a + (b - a) * t;
    }
};
```

### System with Many Dependencies

```cpp
// Supports up to 4 dependencies
BESTOW_SYSTEM(
    ComplexSystem,
    IComplexSystem,
    IEventSystem,
    IAssetSystem,
    IEntitySystem,
    ISaveSystem
) {
public:
    void doComplexWork() override {
        pIEventSystem_->publish(...);
        pIAssetSystem_->loadAsset(...);
        auto entity = pIEntitySystem_->createEntity();
        pISaveSystem_->saveSlot(0);
    }
};
```

## Troubleshooting

**Linker error: undefined reference to Service**
- Make sure you used `BESTOW_SYSTEM` macro correctly
- Check that you closed the class with `};`
- Verify you're exporting the class with `export namespace`

**Dependency is nullptr**
- Make sure you registered the dependency with Kangaru
- Check registration order - dependencies must be registered first
- Verify you're using interface types, not implementations

**Macro expansion errors**
- Include headers in global module fragment (`module;` section)
- Don't mix `#include` and `import` for the same types
- Use full type names including `bestow::` namespace

**Hot reload not working**
- Only available in debug builds with `BESTOW_DEV_TOOLS`
- Call `pIAssetSystem_->enableHotReload(true)`
- Subscribe to asset changes via `subscribeToType`

## Next Steps

You've learned how to create custom systems! Advanced topics:

- **Tutorial 8: Advanced Lua Integration** - Complex Lua bindings, coroutines
- **Tutorial 9: Multiplayer Systems** - Networking and replication
- **Tutorial 10: Performance Optimization** - Profiling, job system, caching

## Summary

Custom systems in Bestow follow these principles:

1. **Contract-based** - Depend only on interfaces, never implementations
2. **Dependency injection** - Use `BESTOW_SYSTEM` macro for automatic DI
3. **Event-driven** - Communicate via EventSystem for decoupling
4. **Data-driven** - Load configuration from Lua, not C++
5. **Hot-reload friendly** - Support live editing in debug builds

This architecture enables complete system interchangeability and makes your game highly moddable and maintainable.
