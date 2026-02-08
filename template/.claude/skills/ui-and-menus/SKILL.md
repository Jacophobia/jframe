---
name: ui-and-menus
description: Create UI elements, HUDs, menus, and overlays using the bestow.ui system (RmlUI-based). Use when implementing health bars, score displays, menus, dialogue boxes, or any user interface elements.
---

# UI System (bestow.ui)

Bestow provides a document-based UI system built on RmlUI. UI is defined using RML documents (HTML-like markup with CSS styling) and controlled from Lua.

**IMPORTANT:** Always use the `bestow.ui` API for UI. Do NOT manually draw UI with graphics primitives - the engine provides a full UI system.

**Note:** The UI system currently works with OpenGL rendering. Vulkan UI rendering is in development.

## Lifecycle

```lua
-- Initialize (usually done once in main.lua init)
bestow.ui.initialize({
    baseScale = 1.0,
    enableDebugMode = false
})

-- In your update loop
bestow.ui.update(dt)

-- In your render function (after beginFrame, before endFrame)
bestow.ui.render()

-- On shutdown
bestow.ui.shutdown()
```

## Loading Documents

```lua
-- Load from file
local doc = bestow.ui.loadDocument("ui/hud.rml")

-- Load from string (for dynamic UI)
local doc = bestow.ui.loadDocumentFromString([[
<rml>
<head>
    <style>
        body { width: 100%; height: 100%; }
        .health-bar {
            width: 200dp; height: 20dp;
            background-color: #333;
            margin: 10dp;
        }
        .health-fill {
            height: 100%;
            background-color: #4a4;
        }
        .score {
            color: #ff0;
            font-size: 24dp;
            text-align: right;
            margin: 10dp;
        }
    </style>
</head>
<body>
    <div class="health-bar">
        <div id="health-fill" class="health-fill" style="width: 100%;"/>
    </div>
    <div id="score" class="score">Score: 0</div>
</body>
</rml>
]], "hud")

-- Show/hide documents
bestow.ui.showDocument(doc)
bestow.ui.hideDocument(doc)
bestow.ui.isDocumentVisible(doc) -> bool

-- Unload when done
bestow.ui.unloadDocument(doc)
```

## Element Access

```lua
-- Find elements
local elem = bestow.ui.getElementById(doc, "health-fill")
local elems = bestow.ui.getElementsByClass(doc, "menu-item")
local buttons = bestow.ui.getElementsByTag(doc, "button")

-- Navigate DOM
local children = bestow.ui.getChildren(elem)
local parent = bestow.ui.getParent(elem)
```

## Element Properties

```lua
-- Text content
bestow.ui.setElementText(elem, "Score: 1500")
local text = bestow.ui.getElementText(elem)

-- Visibility
bestow.ui.setElementVisible(elem, "Visible")    -- UIVisibility: Visible, Hidden, Collapsed
bestow.ui.setElementVisible(elem, "Hidden")      -- Takes space but invisible
bestow.ui.setElementVisible(elem, "Collapsed")   -- No space, invisible

-- CSS classes
bestow.ui.addClass(elem, "active")
bestow.ui.removeClass(elem, "active")
bestow.ui.hasClass(elem, "active") -> bool

-- Attributes
bestow.ui.setAttribute(elem, "data-value", "100")
local val = bestow.ui.getAttribute(elem, "data-value")

-- Inline styles
bestow.ui.setStyle(elem, "width", "75%")
bestow.ui.setStyle(elem, "background-color", "#f00")
bestow.ui.setStyle(elem, "display", "none")

-- Bounds
local rect = bestow.ui.getBounds(elem)  -- { x, y, width, height }

-- Focus
bestow.ui.focus(elem)
bestow.ui.blur(elem)
```

## Dynamic Element Creation

```lua
-- Create new elements
local div = bestow.ui.createElement(doc, "div")
bestow.ui.appendChild(parent, div)
bestow.ui.setElementText(div, "New element!")
bestow.ui.addClass(div, "notification")

-- Set inner HTML content
bestow.ui.setInnerContent(elem, "<span class='bold'>Hello</span> World")

-- Remove elements
bestow.ui.removeElement(elem)
```

## Event Handling

```lua
-- Listen for events on specific elements
bestow.ui.onElementEvent(button, "click", function(event)
    -- event.targetId, event.targetClass, event.mouseX, event.mouseY
    startGame()
end)

-- Listen for events globally
bestow.ui.onEvent("click", function(event)
    -- Any click in any document
end)

-- Remove listeners
bestow.ui.offEvent("click")
```

## Input Integration

```lua
-- Forward input events to UI (requires a table describing the event)
bestow.ui.processInput({
    type = UIInputType.MouseMove,  -- UIInputType enum (MouseMove, MouseDown, MouseUp, MouseScroll, KeyDown, KeyUp, TextInput)
    x = mx,                        -- Mouse/pointer X position
    y = my,                        -- Mouse/pointer Y position
    button = 0,                    -- Mouse button index (for MouseDown/MouseUp)
    wheelDelta = 0,                -- Scroll wheel delta (for MouseScroll)
    keyCode = 0,                   -- Key code (for KeyDown/KeyUp)
    modifiers = 0,                 -- Modifier key flags
    character = ""                 -- Character string (for TextInput)
}) -> bool  -- Returns true if UI consumed the input

-- Check if UI wants input (gate game input on this)
if bestow.ui.wantsKeyboardInput() then
    -- Don't process game keyboard input
end
if bestow.ui.wantsMouseInput() then
    -- Don't process game mouse input
end
```

## Data Binding

Bind Lua data to UI elements for automatic updates:

```lua
-- Bind data variables
bestow.ui.bindData(doc, "player_health", 100)        -- int
bestow.ui.bindData(doc, "player_name", "Hero")        -- string
bestow.ui.bindData(doc, "is_paused", false)            -- bool
bestow.ui.bindData(doc, "score_multiplier", 1.5)       -- float

-- Update bound data (UI updates automatically)
bestow.ui.updateData(doc, "player_health", 75)
```

In RML, use data binding syntax:
```html
<div>Health: {{ player_health }}</div>
<div data-if="is_paused">PAUSED</div>
```

## Common UI Patterns

### HUD Document

```xml
<!-- ui/hud.rml -->
<rml>
<head>
    <style>
        body { width: 100%; height: 100%; font-family: "default"; }
        #hud { position: absolute; top: 0; left: 0; right: 0; padding: 10dp; }
        .health-container { width: 200dp; height: 24dp; background-color: #222; border: 1dp #666; }
        .health-bar { height: 100%; background-color: #4a4; transition: width 0.3s; }
        .health-bar.low { background-color: #a44; }
        .health-bar.medium { background-color: #aa4; }
        #score { float: right; color: #ff0; font-size: 28dp; }
        #lives { margin-top: 5dp; }
        .heart { display: inline-block; width: 20dp; height: 20dp; background-color: #f44; margin-right: 5dp; }
    </style>
</head>
<body>
    <div id="hud">
        <div id="score">Score: 0</div>
        <div class="health-container">
            <div id="health-bar" class="health-bar" style="width: 100%;"/>
        </div>
        <div id="lives"></div>
    </div>
</body>
</rml>
```

### Updating HUD from Lua

```lua
-- systems/hud.lua
return {
    doc = nil,

    init = function()
        local self = app.systems.hud
        self.doc = bestow.ui.loadDocument("ui/hud.rml")
        bestow.ui.showDocument(self.doc)
    end,

    update = function(dt)
        local self = app.systems.hud
        local state = app.main.state
        if not self.doc or not state.player then return end

        -- Update health bar width
        local health = bestow.entity.getComponent(state.player, "Health")
        if health then
            local pct = math.floor(health.current / health.max * 100)
            local bar = bestow.ui.getElementById(self.doc, "health-bar")
            if bar then
                bestow.ui.setStyle(bar, "width", pct .. "%")
                -- Color based on health
                bestow.ui.removeClass(bar, "low")
                bestow.ui.removeClass(bar, "medium")
                if pct < 30 then bestow.ui.addClass(bar, "low")
                elseif pct < 60 then bestow.ui.addClass(bar, "medium") end
            end
        end

        -- Update score
        local scoreElem = bestow.ui.getElementById(self.doc, "score")
        if scoreElem then
            bestow.ui.setElementText(scoreElem, "Score: " .. (state.score or 0))
        end
    end
}
```

### Menu System

```xml
<!-- ui/main-menu.rml -->
<rml>
<head>
    <style>
        body { width: 100%; height: 100%; }
        #overlay {
            position: absolute; top: 0; left: 0; right: 0; bottom: 0;
            background-color: rgba(0,0,0,0.8);
            display: flex; flex-direction: column;
            align-items: center; justify-content: center;
        }
        h1 { color: #fc3; font-size: 48dp; margin-bottom: 40dp; }
        .menu-btn {
            width: 300dp; padding: 15dp; margin: 5dp;
            background-color: #335; color: #fff; font-size: 20dp;
            text-align: center; cursor: pointer; border: 1dp #557;
        }
        .menu-btn:hover { background-color: #447; border-color: #88a; }
        .menu-btn:active { background-color: #224; }
    </style>
</head>
<body>
    <div id="overlay">
        <h1>My Game</h1>
        <div id="start-btn" class="menu-btn">Start Game</div>
        <div id="options-btn" class="menu-btn">Options</div>
        <div id="quit-btn" class="menu-btn">Quit</div>
    </div>
</body>
</rml>
```

```lua
-- systems/main_menu.lua
return {
    doc = nil,

    show = function()
        local self = app.systems.main_menu
        self.doc = bestow.ui.loadDocument("ui/main-menu.rml")
        bestow.ui.showDocument(self.doc)

        -- Wire up buttons
        local startBtn = bestow.ui.getElementById(self.doc, "start-btn")
        local optionsBtn = bestow.ui.getElementById(self.doc, "options-btn")
        local quitBtn = bestow.ui.getElementById(self.doc, "quit-btn")

        bestow.ui.onElementEvent(startBtn, "click", function()
            app.systems.main_menu.hide()
            app.main.startGame()
        end)

        bestow.ui.onElementEvent(optionsBtn, "click", function()
            app.systems.main_menu.hide()
            app.systems.options.show()
        end)

        bestow.ui.onElementEvent(quitBtn, "click", function()
            app.main.state.running = false
        end)
    end,

    hide = function()
        local self = app.systems.main_menu
        if self.doc then
            bestow.ui.hideDocument(self.doc)
        end
    end
}
```

### Pause Overlay

```lua
return {
    doc = nil,

    toggle = function()
        local self = app.systems.pause
        if self.doc and bestow.ui.isDocumentVisible(self.doc) then
            bestow.ui.hideDocument(self.doc)
            bestow.phase.pop()  -- Resume gameplay input phase
        else
            if not self.doc then
                self.doc = bestow.ui.loadDocumentFromString([[
                <rml><head><style>
                    #overlay { position: absolute; top: 0; left: 0; right: 0; bottom: 0;
                               background-color: rgba(0,0,0,0.6); display: flex;
                               align-items: center; justify-content: center; }
                    h1 { color: #fff; font-size: 48dp; }
                    .btn { width: 200dp; padding: 10dp; margin: 5dp; background-color: #335;
                           color: #fff; text-align: center; cursor: pointer; }
                    .btn:hover { background-color: #447; }
                </style></head><body>
                    <div id="overlay">
                        <h1>PAUSED</h1>
                        <div id="resume-btn" class="btn">Resume</div>
                        <div id="quit-btn" class="btn">Quit to Menu</div>
                    </div>
                </body></rml>
                ]], "pause")

                local resumeBtn = bestow.ui.getElementById(self.doc, "resume-btn")
                bestow.ui.onElementEvent(resumeBtn, "click", function()
                    app.systems.pause.toggle()
                end)
            end
            bestow.ui.showDocument(self.doc)
            bestow.phase.push("pause")  -- Switch input to pause phase
        end
    end
}
```

## Best Practices

1. **Use `bestow.ui` for ALL UI** - Never draw UI manually with graphics primitives
2. **Use CSS for styling** - RmlUI supports a subset of CSS (flexbox, transitions, colors, borders)
3. **Gate game input on UI** - Check `wantsKeyboardInput()`/`wantsMouseInput()` before processing game input
4. **Load documents once, show/hide** - Don't reload documents every frame
5. **Use classes for state** - Add/remove CSS classes instead of inline styles for visual states
6. **Use data binding** for frequently updating values (health, score, timers)
7. **Update UI in update(), render in render()** - Call `bestow.ui.update(dt)` in update and `bestow.ui.render()` in render
