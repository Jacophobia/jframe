// Level System Demo - Comprehensive API Testing
// Demonstrates all ILevelSystem methods and types

// MSVC C++23 module compatibility for sol2 globals
#include <bestow/sol2_compat.hpp>

import std;
import bestow.types;
import bestow.level;

using namespace bestow;

// Demo helper functions
void printSectionHeader(const std::string& title) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(80, '=') << "\n\n";
}

void printSubsection(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n\n";
}

std::string levelStateToString(LevelState state) {
    switch (state) {
        case LevelState::Unloaded: return "Unloaded";
        case LevelState::Loading: return "Loading";
        case LevelState::Loaded: return "Loaded";
        case LevelState::Active: return "Active";
        case LevelState::Unloading: return "Unloading";
        default: return "Unknown";
    }
}

void printLevelMetadata(const LevelMetadata& meta) {
    std::cout << "  Level ID: " << meta.id << "\n";
    std::cout << "  Name: " << meta.levelName << "\n";
    std::cout << "  State: " << levelStateToString(meta.state) << "\n";
    std::cout << "  Dimensions: " << meta.width << "x" << meta.height << "\n";
    std::cout << "  Asset Handle: " << meta.assetHandle.uuid << "\n";
}

void printTransform(const Transform2D& transform) {
    std::format_to(std::ostream_iterator<char>(std::cout),
        "Position({:.1f}, {:.1f}), Rotation: {:.1f}, Scale({:.2f}, {:.2f})",
        transform.x, transform.y, transform.rotation, transform.scaleX, transform.scaleY);
}

void printEntityDef(const EntityDef& def) {
    std::cout << "  Type: " << def.type << "\n";
    std::cout << "    Transform: ";
    printTransform(def.transform);
    std::cout << "\n";
    std::cout << "    Properties (" << def.properties.size() << "):\n";

    // Print some common property types
    for (const auto& [key, value] : def.properties) {
        std::cout << "      - " << key << ": ";

        // Try to cast to common types
        try {
            if (value.type() == typeid(std::string)) {
                std::cout << std::any_cast<std::string>(value);
            } else if (value.type() == typeid(const char*)) {
                std::cout << std::any_cast<const char*>(value);
            } else if (value.type() == typeid(double)) {
                std::cout << std::any_cast<double>(value);
            } else if (value.type() == typeid(float)) {
                std::cout << std::any_cast<float>(value);
            } else if (value.type() == typeid(int)) {
                std::cout << std::any_cast<int>(value);
            } else if (value.type() == typeid(bool)) {
                std::cout << (std::any_cast<bool>(value) ? "true" : "false");
            } else {
                std::cout << "<" << value.type().name() << ">";
            }
        } catch (const std::bad_any_cast&) {
            std::cout << "<unknown type>";
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << R"(
╔════════════════════════════════════════════════════════════════════════════╗
║                      Bestow Level System Demo                              ║
║                                                                            ║
║  This demo exercises the entire ILevelSystem API:                         ║
║    - Level loading and unloading                                          ║
║    - Level state management                                               ║
║    - Level transitions                                                    ║
║    - Spawn point queries                                                  ║
║    - Entity definition queries                                            ║
║    - Level metadata inspection                                            ║
╚════════════════════════════════════════════════════════════════════════════╝
)" << "\n";

    std::cout << "NOTE: This is a demonstration of the Level System API.\n";
    std::cout << "      A real implementation would be injected via Fruit DI.\n";
    std::cout << "      This demo shows how to USE the system, not implement it.\n";

    // ========================================================================
    // Setup: Create Dependencies
    // ========================================================================
    printSectionHeader("1. SETUP: Initializing Dependencies");

    std::cout << "In a real game, you would get systems from dependency injection:\n\n";
    std::cout << "  // Get systems from Fruit DI\n";
    std::cout << "  auto assetSystem = injector.get<IAssetSystem*>();\n";
    std::cout << "  auto entitySystem = injector.get<IEntitySystem*>();\n";
    std::cout << "  auto eventSystem = injector.get<IEventSystem*>();\n";
    std::cout << "  auto levelSystem = injector.get<ILevelSystem*>();\n\n";

    std::cout << "Dependencies required by Level System:\n";
    std::cout << "  - IAssetSystem (for level file loading)\n";
    std::cout << "  - IEntitySystem (for level entity management)\n";
    std::cout << "  - IEventSystem (for level events)\n";

    // ========================================================================
    // Level Management: Register and Load Levels
    // ========================================================================
    printSectionHeader("2. LEVEL MANAGEMENT: Registering and Loading Levels");

    printSubsection("2.1 Register Level Assets");

    std::cout << "First, register level files as assets:\n\n";
    std::cout << "  AssetHandle level1Asset = assetSystem->registerAsset(\n";
    std::cout << "    AssetType::Level,\n";
    std::cout << "    \"data/level1.lua\"\n";
    std::cout << "  );\n\n";
    std::cout << "  AssetHandle level2Asset = assetSystem->registerAsset(\n";
    std::cout << "    AssetType::Level,\n";
    std::cout << "    \"data/level2.lua\"\n";
    std::cout << "  );\n\n";

    std::cout << "This tells the asset system about the Lua level files.\n";

    printSubsection("2.2 Load Levels");

    std::cout << "Load a level from its asset handle:\n\n";
    std::cout << "  Result<LevelId, std::error_code> result = levelSystem->loadLevel(level1Asset);\n";
    std::cout << "  if (result.has_value()) {\n";
    std::cout << "    LevelId level1Id = result.value();\n";
    std::cout << "    std::cout << \"Level loaded with ID: \" << level1Id << \"\\n\";\n";
    std::cout << "  } else {\n";
    std::cout << "    std::error_code error = result.error();\n";
    std::cout << "    std::cerr << \"Failed to load level: \" << error.message() << \"\\n\";\n";
    std::cout << "  }\n\n";

    std::cout << "Expected behavior:\n";
    std::cout << "  1. Asset system loads the Lua file\n";
    std::cout << "  2. Lua is parsed and sandboxed\n";
    std::cout << "  3. Level metadata is extracted (name, dimensions)\n";
    std::cout << "  4. Spawn points are stored\n";
    std::cout << "  5. Entity definitions are stored\n";
    std::cout << "  6. Level state transitions: Unloaded -> Loading -> Loaded\n";
    std::cout << "  7. LevelEvent::LoadStarted and LoadCompleted events are emitted\n";
    std::cout << "  8. Unique LevelId (UUID) is returned\n";

    // ========================================================================
    // State Queries: Inspect Level State
    // ========================================================================
    printSectionHeader("3. STATE QUERIES: Inspecting Level State");

    printSubsection("3.1 Get Active Level");

    std::cout << "Check which level is currently active:\n\n";
    std::cout << "  std::optional<LevelId> activeLevel = levelSystem->getActiveLevel();\n";
    std::cout << "  if (activeLevel.has_value()) {\n";
    std::cout << "    std::cout << \"Active level: \" << activeLevel.value() << \"\\n\";\n";
    std::cout << "  } else {\n";
    std::cout << "    std::cout << \"No active level\\n\";\n";
    std::cout << "  }\n\n";

    std::cout << "Expected: std::nullopt (no level set as active yet)\n";

    printSubsection("3.2 Get Level State");

    std::cout << "Query the state of a specific level:\n\n";
    std::cout << "  LevelState state = levelSystem->getLevelState(level1Id);\n";
    std::cout << "  std::cout << \"Level state: \" << levelStateToString(state) << \"\\n\";\n\n";

    std::cout << "Available states:\n";
    std::cout << "  - LevelState::Unloaded  (not in memory)\n";
    std::cout << "  - LevelState::Loading   (currently loading)\n";
    std::cout << "  - LevelState::Loaded    (in memory, not active)\n";
    std::cout << "  - LevelState::Active    (current level, entities spawned)\n";
    std::cout << "  - LevelState::Unloading (currently unloading)\n\n";

    std::cout << "Expected: LevelState::Loaded (after successful loadLevel call)\n";

    printSubsection("3.3 Get Level Metadata");

    std::cout << "Get detailed information about a level:\n\n";
    std::cout << "  LevelMetadata meta = levelSystem->getLevelMetadata(level1Id);\n";
    std::cout << "  std::cout << \"Name: \" << meta.levelName << \"\\n\";\n";
    std::cout << "  std::cout << \"Dimensions: \" << meta.width << \"x\" << meta.height << \"\\n\";\n";
    std::cout << "  std::cout << \"State: \" << levelStateToString(meta.state) << \"\\n\";\n\n";

    std::cout << "LevelMetadata structure:\n";
    std::cout << "  struct LevelMetadata {\n";
    std::cout << "    LevelId id;              // UUID of the level\n";
    std::cout << "    AssetHandle assetHandle; // Source asset\n";
    std::cout << "    std::string levelName;   // Display name from Lua\n";
    std::cout << "    LevelState state;        // Current state\n";
    std::cout << "    float width;             // Level width (from metadata.width)\n";
    std::cout << "    float height;            // Level height (from metadata.height)\n";
    std::cout << "  };\n\n";

    std::cout << "Expected metadata for level1.lua:\n";
    std::cout << "  Name: \"Tutorial Village\"\n";
    std::cout << "  Dimensions: 1920.0 x 1080.0\n";
    std::cout << "  State: Loaded\n";

    printSubsection("3.4 Get All Loaded Levels");

    std::cout << "Get metadata for all loaded levels:\n\n";
    std::cout << "  std::vector<LevelMetadata> loadedLevels = levelSystem->getLoadedLevels();\n";
    std::cout << "  for (const auto& meta : loadedLevels) {\n";
    std::cout << "    std::cout << meta.levelName << \" (\" << meta.width << \"x\" << meta.height << \")\\n\";\n";
    std::cout << "  }\n\n";

    std::cout << "Expected: Vector containing metadata for all loaded levels\n";
    std::cout << "  - Level 1: Tutorial Village (1920x1080)\n";
    std::cout << "  - Level 2: Dungeon Depths (2560x1440)\n";

    // ========================================================================
    // Level Activation: Set Active Level
    // ========================================================================
    printSectionHeader("4. LEVEL ACTIVATION: Setting Active Level");

    printSubsection("4.1 Activate Level");

    std::cout << "Set a loaded level as the active level:\n\n";
    std::cout << "  levelSystem->setActiveLevel(level1Id);\n\n";

    std::cout << "Expected behavior:\n";
    std::cout << "  1. Level state transitions: Loaded -> Active\n";
    std::cout << "  2. LevelEvent::Activated event is emitted\n";
    std::cout << "  3. getActiveLevel() now returns level1Id\n";
    std::cout << "  4. Entity definitions are ready for spawning\n";
    std::cout << "  5. Spawn points are available for player placement\n\n";

    std::cout << "Note: Only one level can be active at a time.\n";
    std::cout << "      Setting a new active level deactivates the previous one.\n";

    // ========================================================================
    // Spawn Points: Query Spawn Locations
    // ========================================================================
    printSectionHeader("5. SPAWN POINTS: Querying Spawn Locations");

    printSubsection("5.1 Get Spawn Point Names");

    std::cout << "Get all spawn point names for a level:\n\n";
    std::cout << "  std::vector<std::string> spawnNames = levelSystem->getSpawnPointNames(level1Id);\n";
    std::cout << "  for (const auto& name : spawnNames) {\n";
    std::cout << "    std::cout << \"Spawn point: \" << name << \"\\n\";\n";
    std::cout << "  }\n\n";

    std::cout << "Expected spawn points in level1.lua:\n";
    std::cout << "  - \"default\" (center of level)\n";
    std::cout << "  - \"fromLeft\" (left entrance)\n";
    std::cout << "  - \"fromRight\" (right entrance)\n";
    std::cout << "  - \"checkpoint1\" (mid-level checkpoint)\n";

    printSubsection("5.2 Get Specific Spawn Point");

    std::cout << "Get the transform for a named spawn point:\n\n";
    std::cout << "  std::optional<Transform2D> spawn = levelSystem->getSpawnPoint(level1Id, \"default\");\n";
    std::cout << "  if (spawn.has_value()) {\n";
    std::cout << "    Transform2D transform = spawn.value();\n";
    std::cout << "    // Spawn player at transform.x, transform.y\n";
    std::cout << "    Entity player = entitySystem->createEntity();\n";
    std::cout << "    entitySystem->emplace<Transform2D>(player, transform);\n";
    std::cout << "  }\n\n";

    std::cout << "Expected transform for \"default\" spawn:\n";
    std::cout << "  Position: (960.0, 540.0)\n";
    std::cout << "  Rotation: 0.0\n";
    std::cout << "  Scale: (1.0, 1.0)\n\n";

    std::cout << "Example: Print all spawn points:\n";
    std::cout << "  for (const auto& name : spawnNames) {\n";
    std::cout << "    auto spawn = levelSystem->getSpawnPoint(level1Id, name);\n";
    std::cout << "    if (spawn.has_value()) {\n";
    std::cout << "      std::cout << name << \": \" << spawn->x << \", \" << spawn->y << \"\\n\";\n";
    std::cout << "    }\n";
    std::cout << "  }\n";

    // ========================================================================
    // Entity Definitions: Query Level Entities
    // ========================================================================
    printSectionHeader("6. ENTITY DEFINITIONS: Querying Level Entities");

    printSubsection("6.1 Get Entity Definitions");

    std::cout << "Get all entity templates defined in the level:\n\n";
    std::cout << "  std::vector<EntityDef> entityDefs = levelSystem->getEntityDefs(level1Id);\n";
    std::cout << "  std::cout << \"Level contains \" << entityDefs.size() << \" entity definitions\\n\";\n\n";

    std::cout << "EntityDef structure:\n";
    std::cout << "  struct EntityDef {\n";
    std::cout << "    std::string type;                               // Entity type\n";
    std::cout << "    Transform2D transform;                           // Position/rotation/scale\n";
    std::cout << "    std::unordered_map<std::string, std::any> properties; // Custom data\n";
    std::cout << "  };\n\n";

    std::cout << "Expected entity types in level1.lua:\n";
    std::cout << "  - \"platform\" (4 instances: ground, left, middle, right)\n";
    std::cout << "  - \"enemy\" (2 instances: walker, stationary)\n";
    std::cout << "  - \"collectible\" (3 instances: coin, powerup, health)\n";
    std::cout << "  - \"exit\" (1 instance: level transition trigger)\n";
    std::cout << "  - \"decoration\" (2 instances: trees)\n\n";

    std::cout << "Total entities: 12\n\n";

    std::cout << "Example entity definition (platform):\n";
    std::cout << "  Type: \"platform\"\n";
    std::cout << "  Transform: Position(960.0, 900.0), Rotation: 0.0\n";
    std::cout << "  Properties:\n";
    std::cout << "    - width: 1920.0\n";
    std::cout << "    - height: 100.0\n";
    std::cout << "    - texture: \"ground_grass\"\n";
    std::cout << "    - isStatic: true\n";
    std::cout << "    - layer: 0\n";

    printSubsection("6.2 Spawn Entities from Definitions");

    std::cout << "Use entity definitions to spawn actual entities:\n\n";
    std::cout << R"(
void spawnLevelEntities(LevelId levelId) {
  auto entityDefs = levelSystem->getEntityDefs(levelId);

  for (const auto& def : entityDefs) {
    Entity entity = entitySystem->createEntity();

    // Add transform component
    entitySystem->emplace<Transform2D>(entity, def.transform);

    // Add type-specific components
    if (def.type == "platform") {
      auto width = std::any_cast<double>(def.properties.at("width"));
      auto height = std::any_cast<double>(def.properties.at("height"));
      auto isStatic = std::any_cast<bool>(def.properties.at("isStatic"));

      // Create physics body for platform
      PhysicsBodyDef bodyDef{
        .type = isStatic ? BodyType::Static : BodyType::Dynamic,
        .transform = def.transform,
        .size = {static_cast<float>(width), static_cast<float>(height)}
      };
      physicsSystem->createBody(entity, bodyDef);

    } else if (def.type == "enemy") {
      auto health = std::any_cast<int>(def.properties.at("health"));
      auto damage = std::any_cast<int>(def.properties.at("damage"));
      // Add enemy components...
    }
    // ... handle other types
  }
}
)" << "\n";

    printSubsection("6.3 Get Level Entities (Already Spawned)");

    std::cout << "Get all entity handles that were spawned for this level:\n\n";
    std::cout << "  std::vector<Entity> entities = levelSystem->getLevelEntities(level1Id);\n";
    std::cout << "  std::cout << entities.size() << \" entities spawned\\n\";\n\n";

    std::cout << "Returns: All Entity handles created for this level\n\n";

    std::cout << "Difference between methods:\n";
    std::cout << "  - getEntityDefs(): Templates/blueprints (before spawning)\n";
    std::cout << "  - getLevelEntities(): Actual spawned entities (after spawning)\n\n";

    std::cout << "Use cases:\n";
    std::cout << "  - getEntityDefs(): Spawn entities when level becomes active\n";
    std::cout << "  - getLevelEntities(): Cleanup entities when unloading level\n";

    // ========================================================================
    // Level Transitions: Switching Between Levels
    // ========================================================================
    printSectionHeader("7. LEVEL TRANSITIONS: Switching Between Levels");

    printSubsection("7.1 Simple Transition");

    std::cout << "Transition from one level to another:\n\n";
    std::cout << "  LevelTransition transition{\n";
    std::cout << "    .fromLevel = level1Id,\n";
    std::cout << "    .toLevel = level2Id,\n";
    std::cout << "    .spawnPoint = \"default\",\n";
    std::cout << "    .unloadPrevious = true\n";
    std::cout << "  };\n";
    std::cout << "  levelSystem->transition(transition);\n\n";

    std::cout << "Expected behavior:\n";
    std::cout << "  1. Level 1 state: Active -> Unloading\n";
    std::cout << "  2. LevelEvent::Deactivated emitted for level 1\n";
    std::cout << "  3. Level 1 entities destroyed\n";
    std::cout << "  4. If level 2 not loaded: Load it first\n";
    std::cout << "  5. Level 2 state: Loaded -> Active\n";
    std::cout << "  6. LevelEvent::Activated emitted for level 2\n";
    std::cout << "  7. If unloadPrevious=true: Level 1 -> Unloaded\n";
    std::cout << "  8. Player spawned at level 2's \"default\" spawn point\n";

    printSubsection("7.2 Transition with Specific Spawn Point");

    std::cout << "Control where the player spawns in the new level:\n\n";
    std::cout << "  LevelTransition transition{\n";
    std::cout << "    .fromLevel = level2Id,\n";
    std::cout << "    .toLevel = level1Id,\n";
    std::cout << "    .spawnPoint = \"fromRight\",  // Specific spawn\n";
    std::cout << "    .unloadPrevious = false       // Keep level 2 loaded\n";
    std::cout << "  };\n";
    std::cout << "  levelSystem->transition(transition);\n\n";

    std::cout << "This allows:\n";
    std::cout << "  - Entering from specific sides (fromLeft, fromRight)\n";
    std::cout << "  - Respawning at checkpoints\n";
    std::cout << "  - Boss arena spawns\n";
    std::cout << "  - Keeping previous level in memory for fast transitions\n";

    printSubsection("7.3 Optional Spawn Point");

    std::cout << "Use default spawn if no specific spawn point is needed:\n\n";
    std::cout << "  LevelTransition transition{\n";
    std::cout << "    .fromLevel = level1Id,\n";
    std::cout << "    .toLevel = level2Id,\n";
    std::cout << "    .spawnPoint = std::nullopt,  // Use default spawn\n";
    std::cout << "    .unloadPrevious = true\n";
    std::cout << "  };\n";
    std::cout << "  levelSystem->transition(transition);\n\n";

    std::cout << "When spawnPoint is std::nullopt:\n";
    std::cout << "  - Level system uses \"default\" spawn point\n";
    std::cout << "  - Fallback to first available spawn if \"default\" doesn't exist\n";

    // ========================================================================
    // Lifecycle: Update Loop
    // ========================================================================
    printSectionHeader("8. LIFECYCLE: Update Loop");

    printSubsection("8.1 Per-Frame Update");

    std::cout << "Call update() every frame:\n\n";
    std::cout << "  void gameLoop() {\n";
    std::cout << "    while (running) {\n";
    std::cout << "      DeltaTime dt = calculateDeltaTime();\n";
    std::cout << "      levelSystem->update(dt);\n";
    std::cout << "      // ... update other systems\n";
    std::cout << "    }\n";
    std::cout << "  }\n\n";

    std::cout << "Update responsibilities:\n";
    std::cout << "  - Process async level loading (if implementing async)\n";
    std::cout << "  - Update level state transitions\n";
    std::cout << "  - Handle queued level transitions\n";
    std::cout << "  - Emit pending level events\n";

    // ========================================================================
    // Unloading: Cleanup
    // ========================================================================
    printSectionHeader("9. UNLOADING: Cleaning Up Levels");

    printSubsection("9.1 Unload Inactive Levels");

    std::cout << "Free memory by unloading levels:\n\n";
    std::cout << "  levelSystem->unloadLevel(level1Id);\n\n";

    std::cout << "Expected behavior:\n";
    std::cout << "  - Level state transitions: Loaded -> Unloading -> Unloaded\n";
    std::cout << "  - LevelEvent::UnloadStarted and UnloadCompleted events emitted\n";
    std::cout << "  - Level entities destroyed (if any remain)\n";
    std::cout << "  - Level data freed from memory\n";
    std::cout << "  - Asset system can unload the Lua file\n\n";

    std::cout << "Note: Cannot unload the active level directly.\n";
    std::cout << "      Either transition to another level first, or deactivate it.\n";

    // ========================================================================
    // Complete API Summary
    // ========================================================================
    printSectionHeader("10. COMPLETE API SUMMARY");

    std::cout << "Lifecycle:\n";
    std::cout << "  void update(DeltaTime dt)\n\n";

    std::cout << "Level Management:\n";
    std::cout << "  Result<LevelId, std::error_code> loadLevel(AssetHandle levelAsset)\n";
    std::cout << "  void unloadLevel(LevelId levelId)\n";
    std::cout << "  void setActiveLevel(LevelId levelId)\n\n";

    std::cout << "Level Transitions:\n";
    std::cout << "  void transition(const LevelTransition& transition)\n\n";

    std::cout << "State Queries:\n";
    std::cout << "  std::optional<LevelId> getActiveLevel() const\n";
    std::cout << "  LevelState getLevelState(LevelId levelId) const\n";
    std::cout << "  LevelMetadata getLevelMetadata(LevelId levelId) const\n";
    std::cout << "  std::vector<LevelMetadata> getLoadedLevels() const\n\n";

    std::cout << "Spawn Points:\n";
    std::cout << "  std::optional<Transform2D> getSpawnPoint(LevelId, const std::string& name) const\n";
    std::cout << "  std::vector<std::string> getSpawnPointNames(LevelId levelId) const\n\n";

    std::cout << "Level Queries:\n";
    std::cout << "  std::vector<Entity> getLevelEntities(LevelId levelId) const\n";
    std::cout << "  std::vector<EntityDef> getEntityDefs(LevelId levelId) const\n";

    // ========================================================================
    // Level Data Format Examples
    // ========================================================================
    printSectionHeader("11. LEVEL DATA FORMAT (Lua Examples)");

    std::cout << "See data/level1.lua and data/level2.lua for complete examples.\n\n";

    std::cout << "Minimal level structure:\n\n";
    std::cout << R"(
return {
  metadata = {
    name = "Level Name",
    width = 1920.0,
    height = 1080.0
  },

  spawnPoints = {
    default = { x = 960.0, y = 540.0, rotation = 0.0 }
  },

  entities = {
    {
      type = "platform",
      transform = { x = 960.0, y = 900.0, rotation = 0.0 },
      properties = {
        width = 1920.0,
        height = 100.0,
        texture = "ground"
      }
    }
  }
}
)" << "\n";

    std::cout << "Key features of Lua format:\n";
    std::cout << "  - Comments allowed (-- comment)\n";
    std::cout << "  - Trailing commas OK\n";
    std::cout << "  - Variables and functions for procedural generation\n";
    std::cout << "  - Math operations (math.sin, math.random, etc.)\n";
    std::cout << "  - Loops for repeated patterns\n";

    // ========================================================================
    // Event System Integration
    // ========================================================================
    printSectionHeader("12. EVENT SYSTEM INTEGRATION");

    std::cout << "Level events are emitted via the Event System:\n\n";

    std::cout << "Subscribe to level events:\n";
    std::cout << R"(
eventSystem->subscribe("Level", [](const EventData& data) {
  auto& levelEvent = std::get<LevelEventData>(data);

  switch (levelEvent.event) {
    case LevelEvent::LoadStarted:
      // Show loading screen
      break;
    case LevelEvent::LoadCompleted:
      // Hide loading screen
      break;
    case LevelEvent::Activated:
      // Start level (spawn player, start music, etc.)
      break;
    case LevelEvent::Deactivated:
      // Pause level, save state
      break;
    case LevelEvent::UnloadStarted:
      // Cleanup level-specific resources
      break;
    case LevelEvent::UnloadCompleted:
      // Level fully unloaded
      break;
  }
});
)" << "\n";

    std::cout << "Available LevelEvent types:\n";
    std::cout << "  - LoadStarted\n";
    std::cout << "  - LoadCompleted\n";
    std::cout << "  - UnloadStarted\n";
    std::cout << "  - UnloadCompleted\n";
    std::cout << "  - Activated\n";
    std::cout << "  - Deactivated\n";

    // ========================================================================
    // Common Usage Patterns
    // ========================================================================
    printSectionHeader("13. COMMON USAGE PATTERNS");

    printSubsection("13.1 Loading Screen Pattern");

    std::cout << R"(
void showLoadingScreen() {
  // Subscribe to level events
  eventSystem->subscribe("Level", [](const EventData& data) {
    auto& evt = std::get<LevelEventData>(data);
    if (evt.event == LevelEvent::LoadStarted) {
      showLoadingUI();
    } else if (evt.event == LevelEvent::LoadCompleted) {
      hideLoadingUI();
    }
  });

  // Trigger level load
  auto result = levelSystem->loadLevel(nextLevelAsset);
  if (result.has_value()) {
    levelSystem->setActiveLevel(result.value());
  }
}
)" << "\n";

    printSubsection("13.2 Checkpoint System");

    std::cout << R"(
void saveCheckpoint(LevelId levelId, const std::string& spawnName) {
  // Save checkpoint data
  checkpointData = {
    .levelId = levelId,
    .spawnPoint = spawnName
  };
}

void respawnAtCheckpoint() {
  // Reload level and spawn at checkpoint
  LevelTransition transition{
    .fromLevel = currentLevelId,
    .toLevel = checkpointData.levelId,
    .spawnPoint = checkpointData.spawnPoint,
    .unloadPrevious = true
  };
  levelSystem->transition(transition);
}
)" << "\n";

    printSubsection("13.3 Level Exit Trigger");

    std::cout << R"(
void onExitTrigger(Entity exitEntity) {
  // Get exit properties from level definition
  auto entityDefs = levelSystem->getEntityDefs(currentLevelId);

  for (const auto& def : entityDefs) {
    if (def.type == "exit") {
      auto targetLevel = std::any_cast<std::string>(def.properties.at("targetLevel"));
      auto targetSpawn = std::any_cast<std::string>(def.properties.at("targetSpawn"));

      // Find the level ID by name (you'd need to track this)
      LevelId targetId = findLevelByName(targetLevel);

      // Transition
      LevelTransition transition{
        .fromLevel = currentLevelId,
        .toLevel = targetId,
        .spawnPoint = targetSpawn,
        .unloadPrevious = true
      };
      levelSystem->transition(transition);
      break;
    }
  }
}
)" << "\n";

    // ========================================================================
    // Conclusion
    // ========================================================================
    printSectionHeader("DEMO COMPLETE");

    std::cout << "This demo has shown:\n\n";
    std::cout << "  ✓ All ILevelSystem interface methods\n";
    std::cout << "  ✓ Level state management (Unloaded -> Loading -> Loaded -> Active)\n";
    std::cout << "  ✓ Level metadata queries\n";
    std::cout << "  ✓ Spawn point queries\n";
    std::cout << "  ✓ Entity definition queries\n";
    std::cout << "  ✓ Level transitions with spawn points\n";
    std::cout << "  ✓ Lua level file format\n";
    std::cout << "  ✓ Event system integration\n";
    std::cout << "  ✓ Common usage patterns\n\n";

    std::cout << "To use in your game:\n";
    std::cout << "  1. Implement ILevelSystem (or use bestow-level library)\n";
    std::cout << "  2. Wire it up with Fruit DI\n";
    std::cout << "  3. Create Lua level files\n";
    std::cout << "  4. Register level assets\n";
    std::cout << "  5. Load and transition between levels\n\n";

    std::cout << "For implementation details, see:\n";
    std::cout << "  - docs/SYSTEM-IMPLEMENTATION-GUIDE.md\n";
    std::cout << "  - docs/bestow-technical-design.md (Level System section)\n";
    std::cout << "  - bestow-contract/src/bestow.level.cppm (interface definition)\n\n";

    return 0;
}
