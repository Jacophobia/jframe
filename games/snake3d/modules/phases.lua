-- games/snake3d/modules/phases.lua
-- Game phase/state enum matching C++ exactly
--
-- From games/game1/src/snake.game-types.cppm lines 59-67

local Phases = {}

-- Game phase enum values (matching C++ exactly)
Phases.MainMenu = 0        -- Initial menu screen
Phases.WorldMap = 1        -- Level select / world progression
Phases.Playing = 2         -- Active gameplay
Phases.Paused = 3          -- Game paused (overlay menu)
Phases.BossFight = 4       -- Boss encounter (not fully implemented in C++)
Phases.LevelComplete = 5   -- Victory screen after level completion
Phases.GameOver = 6        -- Death/failure screen

-- Phase names for debugging
Phases.names = {
    [Phases.MainMenu] = "MainMenu",
    [Phases.WorldMap] = "WorldMap",
    [Phases.Playing] = "Playing",
    [Phases.Paused] = "Paused",
    [Phases.BossFight] = "BossFight",
    [Phases.LevelComplete] = "LevelComplete",
    [Phases.GameOver] = "GameOver",
}

-- Get phase name for logging
function Phases.getName(phase)
    return Phases.names[phase] or "Unknown"
end

return Phases
