---
name: ui-and-menus
description: Create UI elements, HUDs, and menu systems in Bestow. Use when implementing health bars, score displays, menus, or any user interface elements.
---

# UI and Menus

Implement user interfaces, HUDs, and menu systems.

## Basic UI System

```lua
-- systems/ui.lua
return {
    -- Screen dimensions (update on resize)
    screenWidth = 1280,
    screenHeight = 720,

    -- UI state
    visible = true,

    init = function()
        local self = app.systems.ui
        local size = bestow.graphics3d.getWindowSize()
        self.screenWidth = size.x
        self.screenHeight = size.y
    end,

    -- Basic drawing functions
    drawText = function(text, x, y, options)
        options = options or {}
        local color = options.color or Color.new(1, 1, 1, 1)
        local scale = options.scale or 1.0
        local align = options.align or "left"

        -- Adjust x based on alignment
        if align == "center" then
            local width = #text * 8 * scale  -- Approximate
            x = x - width / 2
        elseif align == "right" then
            local width = #text * 8 * scale
            x = x - width
        end

        bestow.graphics3d.drawText(text, x, y, {
            color = color,
            scale = scale
        })
    end,

    drawTitle = function(text, x, y)
        local self = app.systems.ui
        self.drawText(text, x, y, {
            scale = 3.0,
            align = "center",
            color = Color.new(1, 0.9, 0.3, 1)
        })
    end,

    drawRect = function(x, y, width, height, color)
        bestow.graphics3d.drawScreenRect(x, y, width, height, color)
    end,

    drawPanel = function(x, y, width, height, options)
        local self = app.systems.ui
        options = options or {}

        local bgColor = options.bgColor or Color.new(0, 0, 0, 0.7)
        local borderColor = options.borderColor or Color.new(1, 1, 1, 0.5)
        local borderWidth = options.borderWidth or 2

        -- Background
        self.drawRect(x, y, width, height, bgColor)

        -- Border
        if borderWidth > 0 then
            self.drawRect(x, y, width, borderWidth, borderColor)  -- Top
            self.drawRect(x, y + height - borderWidth, width, borderWidth, borderColor)  -- Bottom
            self.drawRect(x, y, borderWidth, height, borderColor)  -- Left
            self.drawRect(x + width - borderWidth, y, borderWidth, height, borderColor)  -- Right
        end
    end
}
```

## HUD Elements

```lua
-- systems/hud.lua
return {
    render = function()
        local self = app.systems.hud
        local state = app.main.state

        if not state.player then return end

        -- Health bar
        self.renderHealthBar()

        -- Score
        self.renderScore()

        -- Lives
        self.renderLives()

        -- Coins
        self.renderCoins()

        -- Power-up indicators
        self.renderPowerups()
    end,

    renderHealthBar = function()
        local self = app.systems.hud
        local state = app.main.state

        local health = bestow.entity.getComponent(state.player, "Health")
        if not health then return end

        local x, y = 20, 20
        local width, height = 200, 20
        local healthPercent = health.current / health.max

        -- Background
        app.systems.ui.drawRect(x, y, width, height, Color.new(0.2, 0.2, 0.2, 0.8))

        -- Health fill
        local fillColor = Color.new(0.2, 0.8, 0.2, 1)
        if healthPercent < 0.3 then
            fillColor = Color.new(0.8, 0.2, 0.2, 1)  -- Red when low
        elseif healthPercent < 0.6 then
            fillColor = Color.new(0.8, 0.8, 0.2, 1)  -- Yellow when medium
        end

        app.systems.ui.drawRect(x, y, width * healthPercent, height, fillColor)

        -- Border
        app.systems.ui.drawRect(x, y, width, 2, Color.new(1, 1, 1, 0.5))
        app.systems.ui.drawRect(x, y + height - 2, width, 2, Color.new(1, 1, 1, 0.5))

        -- Text
        local text = math.floor(health.current) .. " / " .. health.max
        app.systems.ui.drawText(text, x + width / 2, y + 3, {
            align = "center",
            scale = 0.8
        })
    end,

    renderScore = function()
        local self = app.systems.hud
        local state = app.main.state

        local x = app.systems.ui.screenWidth - 20
        local y = 20

        app.systems.ui.drawText("SCORE", x, y, {
            align = "right",
            color = Color.new(0.7, 0.7, 0.7, 1)
        })

        app.systems.ui.drawText(tostring(state.score or 0), x, y + 25, {
            align = "right",
            scale = 2.0,
            color = Color.new(1, 1, 0, 1)
        })
    end,

    renderLives = function()
        local self = app.systems.hud
        local state = app.main.state

        local x = 20
        local y = 50

        for i = 1, (state.lives or 3) do
            -- Draw heart icon
            app.systems.ui.drawRect(x + (i - 1) * 25, y, 20, 20, Color.new(1, 0.3, 0.3, 1))
        end
    end,

    renderCoins = function()
        local self = app.systems.hud
        local state = app.main.state

        local x = 20
        local y = 80

        -- Coin icon
        app.systems.ui.drawRect(x, y, 16, 16, Color.new(1, 0.85, 0, 1))

        -- Count
        app.systems.ui.drawText("x " .. (state.coins or 0), x + 25, y, {
            scale = 1.2
        })
    end,

    renderPowerups = function()
        local self = app.systems.hud
        local state = app.main.state

        local x = app.systems.ui.screenWidth / 2
        local y = 20
        local index = 0

        for powerupType, data in pairs(app.systems.powerups.active) do
            local iconX = x + index * 50 - 25

            -- Icon background
            app.systems.ui.drawRect(iconX, y, 40, 40, Color.new(0.2, 0.2, 0.5, 0.8))

            -- Timer bar
            local timerPercent = data.timer / 10  -- Assume max 10 seconds
            app.systems.ui.drawRect(iconX, y + 40, 40 * timerPercent, 5, Color.new(0.5, 0.8, 1, 1))

            -- Timer text
            app.systems.ui.drawText(string.format("%.1f", data.timer), iconX + 20, y + 45, {
                align = "center",
                scale = 0.7
            })

            index = index + 1
        end
    end
}
```

## Menu System

```lua
-- systems/menu.lua
return {
    items = {},
    selectedIndex = 1,
    visible = false,

    show = function(menuItems)
        local self = app.systems.menu
        self.items = menuItems
        self.selectedIndex = 1
        self.visible = true
    end,

    hide = function()
        local self = app.systems.menu
        self.visible = false
    end,

    update = function(dt)
        local self = app.systems.menu

        if not self.visible then return end

        -- Navigate up
        if bestow.input.wasKeyJustPressed(Keys.Up) or
           bestow.input.wasKeyJustPressed(Keys.Comma) or
           bestow.input.wasKeyJustPressed(Keys.W) then
            self.selectedIndex = self.selectedIndex - 1
            if self.selectedIndex < 1 then
                self.selectedIndex = #self.items
            end
            app.systems.audio.playSfx("menu_move")
        end

        -- Navigate down
        if bestow.input.wasKeyJustPressed(Keys.Down) or
           bestow.input.wasKeyJustPressed(Keys.O) or
           bestow.input.wasKeyJustPressed(Keys.S) then
            self.selectedIndex = self.selectedIndex + 1
            if self.selectedIndex > #self.items then
                self.selectedIndex = 1
            end
            app.systems.audio.playSfx("menu_move")
        end

        -- Select
        if bestow.input.wasKeyJustPressed(Keys.Enter) or
           bestow.input.wasKeyJustPressed(Keys.Space) then
            local item = self.items[self.selectedIndex]
            if item and item.action then
                app.systems.audio.playSfx("menu_select")
                item.action()
            end
        end

        -- Back
        if bestow.input.wasKeyJustPressed(Keys.Escape) then
            if self.onBack then
                self.onBack()
            end
        end
    end,

    render = function()
        local self = app.systems.menu

        if not self.visible then return end

        local ui = app.systems.ui
        local centerX = ui.screenWidth / 2
        local startY = ui.screenHeight / 2 - (#self.items * 30) / 2

        for i, item in ipairs(self.items) do
            local y = startY + (i - 1) * 40
            local selected = i == self.selectedIndex

            -- Selection indicator
            if selected then
                ui.drawRect(centerX - 150, y - 5, 300, 35, Color.new(0.3, 0.3, 0.6, 0.8))
                ui.drawText("> ", centerX - 140, y, { color = Color.new(1, 1, 0, 1) })
            end

            -- Menu item text
            local color = selected and Color.new(1, 1, 1, 1) or Color.new(0.7, 0.7, 0.7, 1)
            ui.drawText(item.text, centerX, y, {
                align = "center",
                color = color,
                scale = selected and 1.2 or 1.0
            })
        end
    end
}

-- Usage
app.systems.menu.show({
    { text = "Start Game", action = function() app.main.startGame() end },
    { text = "Options", action = function() app.main.showOptions() end },
    { text = "Quit", action = function() return false end }
})
```

## Dialogue System

```lua
-- systems/dialogue.lua
return {
    active = false,
    currentDialogue = nil,
    currentLine = 1,
    displayedText = "",
    charIndex = 0,
    charTimer = 0,
    charDelay = 0.03,  -- Seconds per character

    show = function(dialogue)
        local self = app.systems.dialogue
        self.active = true
        self.currentDialogue = dialogue
        self.currentLine = 1
        self.displayedText = ""
        self.charIndex = 0
    end,

    update = function(dt)
        local self = app.systems.dialogue

        if not self.active then return end

        local line = self.currentDialogue[self.currentLine]
        if not line then
            self.active = false
            return
        end

        local fullText = line.text

        -- Typewriter effect
        if self.charIndex < #fullText then
            self.charTimer = self.charTimer + dt
            while self.charTimer >= self.charDelay and self.charIndex < #fullText do
                self.charIndex = self.charIndex + 1
                self.displayedText = string.sub(fullText, 1, self.charIndex)
                self.charTimer = self.charTimer - self.charDelay
            end
        end

        -- Advance dialogue
        if bestow.input.wasKeyJustPressed(Keys.Space) or
           bestow.input.wasKeyJustPressed(Keys.Enter) then
            if self.charIndex < #fullText then
                -- Skip to end of line
                self.charIndex = #fullText
                self.displayedText = fullText
            else
                -- Next line
                self.currentLine = self.currentLine + 1
                if self.currentLine > #self.currentDialogue then
                    self.active = false
                    if self.currentDialogue.onComplete then
                        self.currentDialogue.onComplete()
                    end
                else
                    self.displayedText = ""
                    self.charIndex = 0
                end
            end
        end
    end,

    render = function()
        local self = app.systems.dialogue

        if not self.active then return end

        local ui = app.systems.ui
        local line = self.currentDialogue[self.currentLine]
        if not line then return end

        -- Dialogue box
        local boxX = 50
        local boxY = ui.screenHeight - 180
        local boxW = ui.screenWidth - 100
        local boxH = 130

        ui.drawPanel(boxX, boxY, boxW, boxH)

        -- Speaker name
        if line.speaker then
            ui.drawText(line.speaker, boxX + 20, boxY + 10, {
                color = Color.new(1, 0.8, 0.3, 1),
                scale = 1.2
            })
        end

        -- Dialogue text
        ui.drawText(self.displayedText, boxX + 20, boxY + 40, {
            color = Color.new(1, 1, 1, 1)
        })

        -- Continue indicator
        if self.charIndex >= #line.text then
            ui.drawText("▼", boxX + boxW - 30, boxY + boxH - 25, {
                color = Color.new(1, 1, 1, math.abs(math.sin(os.clock() * 4)))
            })
        end
    end
}

-- Usage
app.systems.dialogue.show({
    { speaker = "NPC", text = "Hello, adventurer! Welcome to our village." },
    { speaker = "NPC", text = "The dragon has been terrorizing us for weeks..." },
    { speaker = "Player", text = "I'll help you defeat it!" },
    onComplete = function()
        -- Give quest
    end
})
```

## Notification System

```lua
-- systems/notifications.lua
return {
    queue = {},
    current = nil,
    displayTime = 3.0,
    timer = 0,

    show = function(message, options)
        local self = app.systems.notifications
        options = options or {}

        table.insert(self.queue, {
            message = message,
            color = options.color or Color.new(1, 1, 1, 1),
            duration = options.duration or self.displayTime,
            icon = options.icon
        })
    end,

    update = function(dt)
        local self = app.systems.notifications

        if self.current then
            self.timer = self.timer - dt
            if self.timer <= 0 then
                self.current = nil
            end
        end

        if not self.current and #self.queue > 0 then
            self.current = table.remove(self.queue, 1)
            self.timer = self.current.duration
        end
    end,

    render = function()
        local self = app.systems.notifications

        if not self.current then return end

        local ui = app.systems.ui
        local x = ui.screenWidth / 2
        local y = 100

        -- Fade out effect
        local alpha = 1.0
        if self.timer < 0.5 then
            alpha = self.timer / 0.5
        end

        local color = self.current.color
        color = Color.new(color.r, color.g, color.b, alpha)

        -- Background
        ui.drawPanel(x - 200, y - 10, 400, 50, {
            bgColor = Color.new(0, 0, 0, 0.7 * alpha)
        })

        -- Text
        ui.drawText(self.current.message, x, y + 5, {
            align = "center",
            color = color,
            scale = 1.2
        })
    end
}

-- Usage
app.systems.notifications.show("Level Complete!")
app.systems.notifications.show("New Ability Unlocked!", { color = Color.new(1, 0.8, 0, 1) })
```

## Best Practices

1. **Layer UI on top** - Render after game world
2. **Use consistent styling** - Colors, fonts, margins
3. **Provide audio feedback** - Menu navigation sounds
4. **Support keyboard navigation** - Dvorak and QWERTY
5. **Fade transitions** - Smooth appearances/disappearances
6. **Keep HUD minimal** - Only show what's needed
7. **Update on window resize** - Recalculate positions
