-- Example: HUD and Pause Menu with bestow.ui (RmlUI)
-- Shows loading documents, updating text, handling events, and phase switching

-- systems/hud.lua
return {
    doc = nil,

    init = function()
        local self = app.systems.hud

        -- Initialize the UI system
        bestow.ui.initialize()

        -- Load HUD document from RML file
        self.doc = bestow.ui.loadDocument("ui/hud.rml")
        bestow.ui.showDocument(self.doc)

        -- Or load inline RML for simple UIs
        -- self.doc = bestow.ui.loadDocumentFromString([[
        --   <rml><body>
        --     <div id="score">Score: 0</div>
        --     <div id="health-bar"><div id="health-fill"/></div>
        --   </body></rml>
        -- ]])
    end,

    update = function(dt)
        local self = app.systems.hud
        local state = app.main.state
        if not self.doc then return end

        -- Update score text
        local scoreEl = bestow.ui.getElementById(self.doc, "score")
        if scoreEl then
            bestow.ui.setElementText(scoreEl, "Score: " .. (state.score or 0))
        end

        -- Update health bar width
        if state.player and bestow.entity.isValid(state.player) then
            local health = bestow.entity.getComponent(state.player, "Health")
            if health then
                local healthEl = bestow.ui.getElementById(self.doc, "health-fill")
                if healthEl then
                    local pct = math.floor((health.current / health.max) * 100)
                    bestow.ui.setStyle(healthEl, "width", pct .. "%")
                end
            end
        end

        -- Process UI input (so RmlUI receives mouse/keyboard)
        bestow.ui.processInput()
    end,

    render = function()
        bestow.ui.render()
    end,

    shutdown = function()
        local self = app.systems.hud
        bestow.ui.shutdown()
    end
}

-- Example HUD RML file (ui/hud.rml):
--[[
<rml>
<head>
    <style>
        body { font-family: LatoLatin; font-size: 18px; }
        #score {
            position: absolute;
            top: 20px;
            left: 20px;
            color: white;
            font-size: 24px;
        }
        #health-bar {
            position: absolute;
            top: 20px;
            right: 20px;
            width: 200px;
            height: 20px;
            background-color: #333;
            border: 2px solid #666;
        }
        #health-fill {
            height: 100%;
            width: 100%;
            background-color: #4a4;
        }
    </style>
</head>
<body>
    <div id="score">Score: 0</div>
    <div id="health-bar">
        <div id="health-fill"/>
    </div>
</body>
</rml>
]]--
