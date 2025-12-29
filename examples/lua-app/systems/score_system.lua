-- systems/score_system.lua
-- Game system for tracking and displaying score

return {
    name = "ScoreSystem",
    priority = 100,  -- Lower priority means later update

    init = function()
        -- Initialize score state
        _G.gameScore = 0
        _G.highScore = 0

        -- Subscribe to score events
        events.on("scoreChanged", function(points)
            _G.gameScore = _G.gameScore + points
            print("Score: " .. _G.gameScore)

            if _G.gameScore > _G.highScore then
                _G.highScore = _G.gameScore
            end
        end)

        events.on("gameReset", function()
            _G.gameScore = 0
        end)

        print("ScoreSystem initialized")
    end,

    update = function(dt)
        -- Could update UI here
    end,

    shutdown = function()
        print("ScoreSystem shutdown")
    end,

    onReload = function()
        print("ScoreSystem reloaded - current score: " .. _G.gameScore)
    end
}
