// games/game1/src/snake.game-types.cppm
// Snake Game Types Partition
//
// Shared types used across all game systems: grid positions, directions,
// snake segments, enemies, level data, and game state.

module;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

export module snake.game:types;

import std;
import bestow.types;

export namespace snake {

//==========================================================================
// Game Constants
//==========================================================================

constexpr int INITIAL_GRID_SIZE = 10;
constexpr float CELL_SIZE = 1.0f;
constexpr float INITIAL_MOVE_INTERVAL = 0.12f;
constexpr float MIN_MOVE_INTERVAL = 0.04f;
constexpr float SPEED_INCREASE = 0.001f;  // Slower speed ramp
constexpr float BASE_CAMERA_DISTANCE = 12.0f;
constexpr float BASE_CAMERA_HEIGHT = 10.0f;
constexpr float CAMERA_SCALE = 0.3f;
constexpr float ANIMATION_SMOOTH = 3.0f;
constexpr float FOOD_SPIN_SPEED = 2.0f;
constexpr float EXPANSION_WAIT_TIME = 1.0f;

// Enemy constants
constexpr float ENEMY_MOVE_INTERVAL = 0.5f;     // Enemies move slower than snake
constexpr float HEALTH_BAR_DURATION = 3.0f;     // Show health bar for 3s after damage
constexpr float DETACH_ANIMATION_TIME = 0.5f;   // Segment explosion duration
constexpr float SHATTER_CHANCE = 1.0f / 6.0f;   // 1/6 chance to shatter

// Legacy constants (for transition period)
constexpr float OBSTACLE_SPAWN_CHANCE = 0.4f;   // 40% chance to spawn obstacle when eating

//==========================================================================
// Direction Enum
//==========================================================================

enum class Direction {
    Up,     // -Z
    Down,   // +Z
    Left,   // -X
    Right   // +X
};

//==========================================================================
// Game Phase / State Machine
//==========================================================================

enum class GamePhase {
    MainMenu,
    WorldMap,
    Playing,
    Paused,
    BossFight,
    LevelComplete,
    GameOver
};

//==========================================================================
// Grid Position
//==========================================================================

struct GridPos {
    int x = 0;
    int z = 0;

    bool operator==(const GridPos& other) const {
        return x == other.x && z == other.z;
    }

    bool operator!=(const GridPos& other) const {
        return !(*this == other);
    }

    GridPos operator+(const GridPos& other) const {
        return {x + other.x, z + other.z};
    }

    GridPos operator-(const GridPos& other) const {
        return {x - other.x, z - other.z};
    }

    // For use in std::set/map
    bool operator<(const GridPos& other) const {
        if (x != other.x) return x < other.x;
        return z < other.z;
    }
};

// Hash function for GridPos (for std::unordered_set/map)
struct GridPosHash {
    std::size_t operator()(const GridPos& pos) const {
        return std::hash<int>()(pos.x) ^ (std::hash<int>()(pos.z) << 16);
    }
};

//==========================================================================
// Direction Helpers
//==========================================================================

inline GridPos directionToOffset(Direction dir) {
    switch (dir) {
        case Direction::Up:    return {0, -1};
        case Direction::Down:  return {0, 1};
        case Direction::Left:  return {-1, 0};
        case Direction::Right: return {1, 0};
    }
    return {0, 0};
}

inline Direction oppositeDirection(Direction dir) {
    switch (dir) {
        case Direction::Up:    return Direction::Down;
        case Direction::Down:  return Direction::Up;
        case Direction::Left:  return Direction::Right;
        case Direction::Right: return Direction::Left;
    }
    return dir;
}

//==========================================================================
// Axis-Aligned Bounding Box (2D Integer)
//==========================================================================

struct AABB2Di {
    GridPos min{0, 0};
    GridPos max{0, 0};

    bool contains(const GridPos& pos) const {
        return pos.x >= min.x && pos.x <= max.x &&
               pos.z >= min.z && pos.z <= max.z;
    }

    int width() const { return max.x - min.x + 1; }
    int height() const { return max.z - min.z + 1; }
};

//==========================================================================
// Snake Segment
//==========================================================================

struct SnakeSegment {
    GridPos pos;
    bestow::Vec4 color{0.2f, 0.8f, 0.3f, 1.0f};  // Default green
};

//==========================================================================
// Detached Segment (for chain break animation)
//==========================================================================

struct DetachedSegment {
    GridPos pos;
    bestow::Vec4 color;
    float timer = DETACH_ANIMATION_TIME;
    bool willShatter = false;        // Determined at detach time (1/6 chance)
    bestow::Vec3 velocity{0, 0, 0};  // For explosion animation
    bestow::Vec3 worldPos{0, 0, 0};  // Precise world position for particles
    float scale = 1.0f;              // Size multiplier (small for explosion particles)
    bool isParticle = false;         // True for tiny explosion cubes
    int bounceCount = 0;             // Number of ground bounces
    float angularVel = 0.0f;         // Spin speed for rolling
    bool isResting = false;          // True when settled on ground
};

//==========================================================================
// Food Pickup (spawned from detached segments)
//==========================================================================

struct FoodPickup {
    GridPos pos;
    bestow::Vec4 color;
    float spawnTime = 0.0f;  // For animation
};

//==========================================================================
// Enemy
//==========================================================================

struct Enemy {
    GridPos pos;
    AABB2Di patrolZone;
    Direction currentDir = Direction::Right;
    float moveTimer = 0.0f;
    float moveInterval = ENEMY_MOVE_INTERVAL;

    int maxHealth = 1;
    int currentHealth = 1;
    bool showHealthBar = false;
    float healthBarTimer = 0.0f;

    bool isBoss = false;
    std::string bossType;

    // Damage flash effect
    bool isDamaged = false;
    float damageFlashTimer = 0.0f;

    // Visual interpolation for smooth movement
    float visualX = 0.0f;
    float visualZ = 0.0f;
    bool visualInitialized = false;
};

//==========================================================================
// Enemy Zone (for level definitions)
//==========================================================================

struct EnemyZone {
    AABB2Di bounds;
    int enemyCount = 1;
    float moveInterval = ENEMY_MOVE_INTERVAL;
    int health = 1;
};

//==========================================================================
// Level Map
//==========================================================================

struct LevelMap {
    std::string name;
    int width = 15;
    int height = 15;

    std::vector<GridPos> walls;
    std::vector<GridPos> foodSpawnPoints;
    std::vector<EnemyZone> enemyZones;
    GridPos playerStart{0, 0};

    int foodRequired = 5;
    bool isBossLevel = false;

    // Runtime queries
    bool isWall(const GridPos& pos) const {
        for (const auto& wall : walls) {
            if (wall == pos) return true;
        }
        return false;
    }

    bool isInBounds(const GridPos& pos) const {
        return pos.x >= 0 && pos.x < width && pos.z >= 0 && pos.z < height;
    }

    bool isValidFoodSpawn(const GridPos& pos) const {
        // Not on walls
        if (isWall(pos)) return false;

        // Not on edges (1 cell margin)
        if (pos.x <= 0 || pos.x >= width - 1) return false;
        if (pos.z <= 0 || pos.z >= height - 1) return false;

        return true;
    }
};

//==========================================================================
// World Map Node
//==========================================================================

struct WorldMapNode {
    GridPos gridPos;
    int levelIndex = -1;          // Index into levels array (-1 = no level)
    std::string displayName;
    bool isUnlocked = false;
    bool isCompleted = false;
    bool isBoss = false;
};

//==========================================================================
// World Config
//==========================================================================

struct WorldConfig {
    std::string name;
    std::string theme;

    std::vector<std::string> levelFiles;
    std::vector<int> unlockRequirements;  // Food needed to unlock each level
    std::vector<WorldMapNode> nodes;
    std::vector<std::pair<int, int>> paths;  // Connections between nodes

    // Boss configuration
    std::string bossName;
    std::string bossType;
    int bossHealth = 10;
};

//==========================================================================
// World Progress (for saving)
//==========================================================================

struct WorldProgress {
    std::vector<bool> levelsCompleted;
    std::vector<int> foodCollected;      // Per level
    bool bossDefeated = false;
    int totalFoodEarned = 0;             // Sum of all food in this world
};

//==========================================================================
// Game Save Data
//==========================================================================

struct GameSaveData {
    std::vector<WorldProgress> worldProgress;
    int currentWorld = 0;
    std::vector<bool> unlockedWorlds;

    int getTotalFoodInWorld(int worldIndex) const {
        if (worldIndex < 0 || worldIndex >= static_cast<int>(worldProgress.size())) {
            return 0;
        }
        return worldProgress[worldIndex].totalFoodEarned;
    }
};

//==========================================================================
// Ring Detection Result
//==========================================================================

struct RingResult {
    bool formed = false;
    std::vector<GridPos> enclosedCells;
    int ringStartIndex = -1;  // Index in snake where ring connects
};

}  // namespace snake
