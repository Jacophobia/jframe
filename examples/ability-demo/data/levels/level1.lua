-- Ability Demo Level 1 - Epic Adventure
-- 7 zones spanning 6000x1200 pixels with progressive difficulty
-- Uses Lua programming for efficient level generation

-- Constants for consistent spacing and design
local GROUND_Y = 1150
local PLATFORM_HEIGHT = 20
local PLATFORM_SPACING = 150
local ZONE_WIDTH = 800

-- Helper function to create a platform
local function makePlatform(x, y, width, height)
  return {
    type = "platform",
    x = x,
    y = y or GROUND_Y,
    width = width or 200,
    height = height or PLATFORM_HEIGHT
  }
end

-- Helper function to create enemy waves
local function spawnEnemyWave(baseX, baseY, enemyType, count, spacing)
  local enemies = {}
  for i = 1, count do
    table.insert(enemies, {
      type = enemyType,
      x = baseX + (i - 1) * spacing,
      y = baseY,
      patrolRange = 150
    })
  end
  return enemies
end

-- Helper function to create staircase platforms
local function makeStaircase(startX, startY, steps, stepWidth, stepHeight, ascending)
  local platforms = {}
  for i = 1, steps do
    local yOffset = ascending and -(i - 1) * stepHeight or (i - 1) * stepHeight
    table.insert(platforms, makePlatform(
      startX + (i - 1) * stepWidth,
      startY + yOffset,
      stepWidth + 20,
      PLATFORM_HEIGHT
    ))
  end
  return platforms
end

-- Helper function to create a spiral of platforms
local function makeSpiral(centerX, centerY, radius, platformCount, turns)
  local platforms = {}
  for i = 1, platformCount do
    local angle = (i / platformCount) * turns * 2 * math.pi
    local currentRadius = radius * (i / platformCount)
    local x = centerX + math.cos(angle) * currentRadius
    local y = centerY - math.sin(angle) * currentRadius
    table.insert(platforms, makePlatform(x, y, 120, PLATFORM_HEIGHT))
  end
  return platforms
end

-- Build the level entities
local entities = {}

-- Helper to add multiple entities
local function addEntities(newEntities)
  for _, entity in ipairs(newEntities) do
    table.insert(entities, entity)
  end
end

-- ============================================================================
-- ZONE 1: TUTORIAL (x: 0-800)
-- ============================================================================

-- Ground platform for entire zone
table.insert(entities, makePlatform(400, GROUND_Y, 800, 30))

-- Starting area - safe ground
table.insert(entities, makePlatform(100, 1050, 150, PLATFORM_HEIGHT))

-- Tutorial text platforms (imaginary signs)
table.insert(entities, makePlatform(200, 1000, 100, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(350, 950, 100, PLATFORM_HEIGHT))

-- Jump training platforms
for i = 1, 5 do
  table.insert(entities, makePlatform(
    450 + i * 60,
    1050 - i * 30,
    80,
    PLATFORM_HEIGHT
  ))
end

-- Jump zone to teach jumping mechanics
table.insert(entities, {
  type = "jump_zone",
  x = 500,
  y = 1000,
  width = 200,
  height = 150
})

-- Sword collectable
table.insert(entities, {
  type = "collectable",
  x = 700,
  y = 900,
  ability = "Sword"
})

-- First checkpoint
table.insert(entities, {
  type = "checkpoint",
  x = 750,
  y = 1120
})

-- ============================================================================
-- ZONE 2: COMBAT TRAINING (x: 800-1600)
-- ============================================================================

-- Ground extension
table.insert(entities, makePlatform(1200, GROUND_Y, 800, 30))

-- Platforms at varying heights
table.insert(entities, makePlatform(900, 1000, 150, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(1100, 900, 150, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(1300, 850, 150, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(1000, 750, 120, PLATFORM_HEIGHT))

-- Walker enemies patrolling
addEntities(spawnEnemyWave(850, GROUND_Y - 50, "enemy_walker", 3, 200))

-- Additional elevated walker
table.insert(entities, {
  type = "enemy_walker",
  x = 1100,
  y = 850,
  patrolRange = 100
})

-- Switch mechanism
table.insert(entities, {
  type = "switch",
  x = 1250,
  y = 820,
  switchId = "door1"
})

-- Door that opens with switch
table.insert(entities, {
  type = "door",
  x = 1500,
  y = 1050,
  width = 30,
  height = 150,
  doorId = "door1"
})

-- DoubleJump collectable at top platform
table.insert(entities, {
  type = "collectable",
  x = 1000,
  y = 700,
  ability = "DoubleJump"
})

-- Checkpoint after combat
table.insert(entities, {
  type = "checkpoint",
  x = 1550,
  y = 1120
})

-- ============================================================================
-- ZONE 3: PLATFORMING CHALLENGE (x: 1600-2400)
-- ============================================================================

-- Ground
table.insert(entities, makePlatform(2000, GROUND_Y, 800, 30))

-- Spiral tower of platforms going up
addEntities(makeSpiral(2000, 900, 250, 15, 3))

-- Jump zones strategically placed for assistance
table.insert(entities, {
  type = "jump_zone",
  x = 1800,
  y = 1000,
  width = 150,
  height = 200
})

table.insert(entities, {
  type = "jump_zone",
  x = 2100,
  y = 700,
  width = 150,
  height = 150
})

-- Moving platforms
for i = 1, 3 do
  table.insert(entities, {
    type = "moving_platform",
    x = 1700 + i * 200,
    y = 950 - i * 100,
    width = 100,
    height = PLATFORM_HEIGHT,
    moveX = 100,
    moveY = 0,
    speed = 2.0,
    oscillate = true
  })
end

-- Vertical moving platforms
table.insert(entities, {
  type = "moving_platform",
  x = 2200,
  y = 800,
  width = 100,
  height = PLATFORM_HEIGHT,
  moveX = 0,
  moveY = 200,
  speed = 1.5,
  oscillate = true
})

-- Jumper enemies on platforms
for i = 1, 4 do
  table.insert(entities, {
    type = "enemy_jumper",
    x = 1900 + math.sin(i) * 200,
    y = 800 - i * 80,
    jumpInterval = 2.0 + i * 0.5
  })
end

-- WallJump collectable at the top
table.insert(entities, {
  type = "collectable",
  x = 2000,
  y = 400,
  ability = "WallJump"
})

-- Walls for wall jump practice
table.insert(entities, makePlatform(1650, 600, 40, 400))
table.insert(entities, makePlatform(2350, 600, 40, 400))

-- Checkpoint
table.insert(entities, {
  type = "checkpoint",
  x = 2350,
  y = 1120
})

-- ============================================================================
-- ZONE 4: PUZZLE AREA (x: 2400-3200)
-- ============================================================================

-- Ground
table.insert(entities, makePlatform(2800, GROUND_Y, 800, 30))

-- Puzzle platforms
table.insert(entities, makePlatform(2500, 1000, 200, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(2800, 950, 200, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(3100, 900, 200, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(2650, 750, 150, PLATFORM_HEIGHT))

-- Switch sequence puzzle (must hit in order)
for i = 1, 4 do
  table.insert(entities, {
    type = "switch",
    x = 2450 + i * 200,
    y = 1000 - i * 50,
    switchId = "puzzle_switch_" .. i,
    sequence = i
  })
end

-- Doors for puzzle
table.insert(entities, {
  type = "door",
  x = 2700,
  y = 1050,
  width = 30,
  height = 120,
  doorId = "puzzle_door_1"
})

table.insert(entities, {
  type = "door",
  x = 3000,
  y = 1050,
  width = 30,
  height = 120,
  doorId = "puzzle_door_2"
})

-- Breakable floor
table.insert(entities, {
  type = "breakable",
  x = 2650,
  y = 720,
  width = 150,
  height = 30,
  requiresAbility = "GroundPound"
})

-- Secret area under breakable floor
table.insert(entities, makePlatform(2650, 600, 150, PLATFORM_HEIGHT))

-- GroundPound collectable in secret
table.insert(entities, {
  type = "collectable",
  x = 2650,
  y = 550,
  ability = "GroundPound"
})

-- Shooter enemies on ledges
for i = 1, 3 do
  table.insert(entities, {
    type = "enemy_shooter",
    x = 2500 + i * 250,
    y = 900 - i * 50,
    shootInterval = 3.0,
    shootRange = 300
  })
end

-- Timed section trigger
table.insert(entities, {
  type = "trigger_zone",
  x = 3050,
  y = 1000,
  width = 100,
  height = 200,
  triggerEvent = "start_timer"
})

-- Checkpoint
table.insert(entities, {
  type = "checkpoint",
  x = 3150,
  y = 1120
})

-- ============================================================================
-- ZONE 5: VERTICAL CLIMB (x: 3200-4000)
-- ============================================================================

-- Ground
table.insert(entities, makePlatform(3600, GROUND_Y, 800, 30))

-- Vertical shaft walls
table.insert(entities, makePlatform(3350, 600, 40, 600))
table.insert(entities, makePlatform(3850, 600, 40, 600))

-- Alternating wall platforms for wall jumping
for i = 1, 12 do
  local leftSide = (i % 2 == 1)
  local x = leftSide and 3400 or 3750
  local y = 1100 - i * 60
  table.insert(entities, makePlatform(x, y, 100, PLATFORM_HEIGHT))
end

-- Shield collectable midway up
table.insert(entities, {
  type = "collectable",
  x = 3600,
  y = 700,
  ability = "Shield"
})

-- Platform for shield
table.insert(entities, makePlatform(3550, 750, 100, PLATFORM_HEIGHT))

-- Flying enemies patrolling up and down
for i = 1, 5 do
  table.insert(entities, {
    type = "enemy_flying",
    x = 3600,
    y = 1000 - i * 120,
    patrolY = 200,
    speed = 1.5
  })
end

-- Jump zones in shaft for assistance
table.insert(entities, {
  type = "jump_zone",
  x = 3500,
  y = 900,
  width = 200,
  height = 150
})

table.insert(entities, {
  type = "jump_zone",
  x = 3500,
  y = 600,
  width = 200,
  height = 150
})

-- Top platform
table.insert(entities, makePlatform(3600, 350, 300, PLATFORM_HEIGHT))

-- Health pickup at top
table.insert(entities, {
  type = "health_pickup",
  x = 3600,
  y = 300,
  healAmount = 50
})

-- Exit ledge
table.insert(entities, makePlatform(3900, 400, 150, PLATFORM_HEIGHT))

-- Checkpoint at bottom of next zone
table.insert(entities, {
  type = "checkpoint",
  x = 3950,
  y = 1120
})

-- ============================================================================
-- ZONE 6: ENEMY GAUNTLET (x: 4000-4800)
-- ============================================================================

-- Ground arena
table.insert(entities, makePlatform(4400, GROUND_Y, 800, 30))

-- Arena walls
table.insert(entities, makePlatform(4000, 800, 40, 400))
table.insert(entities, makePlatform(4800, 800, 40, 400))

-- Multi-level arena platforms
table.insert(entities, makePlatform(4150, 1000, 150, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(4650, 1000, 150, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(4400, 850, 200, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(4200, 700, 120, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(4600, 700, 120, PLATFORM_HEIGHT))

-- Wave 1: Walkers
addEntities(spawnEnemyWave(4100, GROUND_Y - 50, "enemy_walker", 4, 150))

-- Wave 2: Jumpers
for i = 1, 3 do
  table.insert(entities, {
    type = "enemy_jumper",
    x = 4150 + i * 200,
    y = 950,
    jumpInterval = 2.0
  })
end

-- Wave 3: Shooters on high platforms
table.insert(entities, {
  type = "enemy_shooter",
  x = 4200,
  y = 650,
  shootInterval = 2.5,
  shootRange = 400
})

table.insert(entities, {
  type = "enemy_shooter",
  x = 4600,
  y = 650,
  shootInterval = 2.5,
  shootRange = 400
})

-- Wave 4: Flying enemies
for i = 1, 4 do
  table.insert(entities, {
    type = "enemy_flying",
    x = 4200 + i * 150,
    y = 600,
    patrolY = 150,
    speed = 2.0
  })
end

-- Ranged collectable as reward (top center)
table.insert(entities, {
  type = "collectable",
  x = 4400,
  y = 800,
  ability = "Ranged"
})

-- Health pickups
table.insert(entities, {
  type = "health_pickup",
  x = 4150,
  y = 950,
  healAmount = 30
})

table.insert(entities, {
  type = "health_pickup",
  x = 4650,
  y = 950,
  healAmount = 30
})

-- Door that opens when all enemies defeated
table.insert(entities, {
  type = "door",
  x = 4790,
  y = 1050,
  width = 40,
  height = 150,
  doorId = "arena_exit",
  openOnClearRoom = true
})

-- Trigger zone for enemy waves
table.insert(entities, {
  type = "trigger_zone",
  x = 4300,
  y = 1000,
  width = 200,
  height = 200,
  triggerEvent = "start_gauntlet"
})

-- Checkpoint after gauntlet
table.insert(entities, {
  type = "checkpoint",
  x = 4850,
  y = 1120
})

-- ============================================================================
-- ZONE 7: BOSS ARENA (x: 4800-6000)
-- ============================================================================

-- Large flat arena floor
table.insert(entities, makePlatform(5400, GROUND_Y, 1200, 40))

-- Arena boundary walls
table.insert(entities, makePlatform(4800, 700, 40, 500))
table.insert(entities, makePlatform(6000, 700, 40, 500))

-- Elevated platform for boss phases
table.insert(entities, makePlatform(5400, 900, 300, PLATFORM_HEIGHT))

-- Cover pillars
for i = 1, 5 do
  table.insert(entities, makePlatform(
    4900 + i * 220,
    GROUND_Y - 200,
    60,
    200
  ))
end

-- Side platforms for tactical positioning
table.insert(entities, makePlatform(4900, 1000, 150, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(5900, 1000, 150, PLATFORM_HEIGHT))
table.insert(entities, makePlatform(5400, 750, 200, PLATFORM_HEIGHT))

-- Boss spawn point
table.insert(entities, {
  type = "boss",
  x = 5400,
  y = 850,
  bossType = "DemoMaster",
  health = 500,
  phases = 3
})

-- Health pickups in boss arena (scattered)
table.insert(entities, {
  type = "health_pickup",
  x = 4950,
  y = 950,
  healAmount = 25
})

table.insert(entities, {
  type = "health_pickup",
  x = 5850,
  y = 950,
  healAmount = 25
})

table.insert(entities, {
  type = "health_pickup",
  x = 5400,
  y = 700,
  healAmount = 50
})

-- Boss trigger zone
table.insert(entities, {
  type = "trigger_zone",
  x = 5200,
  y = 1000,
  width = 400,
  height = 200,
  triggerEvent = "boss_fight_start"
})

-- Victory trigger zone (appears after boss defeat)
table.insert(entities, {
  type = "trigger_zone",
  x = 5800,
  y = 1000,
  width = 200,
  height = 200,
  triggerEvent = "level_complete",
  requiresBossDefeat = true
})

-- Final checkpoint
table.insert(entities, {
  type = "checkpoint",
  x = 5900,
  y = 1120
})

-- Victory platform
table.insert(entities, makePlatform(5900, GROUND_Y - 100, 200, PLATFORM_HEIGHT))

-- ============================================================================
-- RETURN LEVEL DEFINITION
-- ============================================================================

return {
  name = "Ability Demo - Epic Adventure",
  width = 6000,
  height = 1200,

  -- Player spawns at the beginning
  spawnPoints = {
    player = { x = 100, y = 1000 }
  },

  -- All entities generated above
  entities = entities,

  -- Level metadata
  metadata = {
    difficulty = "progressive",
    recommendedTime = 600, -- 10 minutes
    zones = {
      { name = "Tutorial", start = 0, finish = 800 },
      { name = "Combat Training", start = 800, finish = 1600 },
      { name = "Platforming Challenge", start = 1600, finish = 2400 },
      { name = "Puzzle Area", start = 2400, finish = 3200 },
      { name = "Vertical Climb", start = 3200, finish = 4000 },
      { name = "Enemy Gauntlet", start = 4000, finish = 4800 },
      { name = "Boss Arena", start = 4800, finish = 6000 }
    },
    totalCollectables = 7,
    totalEnemies = 45,
    totalCheckpoints = 7
  }
}
