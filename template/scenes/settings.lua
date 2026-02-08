-- Settings screen
-- Sections: Display, Audio, Graphics
-- Reads initial values from config, applies changes immediately.
-- Persistence is stubbed (save system not yet implemented).

return {
    phase = "settings",

    enter = function(params)
        local nav = app.systems.menu_nav
        local doc = bestow.ui.loadDocument("assets/ui/settings.rml")
        if not doc then return end
        bestow.ui.showDocument(doc)

        -- Load current state from app.main.state.settings (initialised in main.lua)
        local s = app.main.state.settings

        -----------------------------------------------------------------------
        -- Helpers
        -----------------------------------------------------------------------

        local function setToggle(id, labelId, value)
            local elem  = bestow.ui.getElementById(doc, id)
            local label = bestow.ui.getElementById(doc, labelId)
            if value then
                bestow.ui.addClass(elem, "on")
                bestow.ui.setElementText(label, "On")
            else
                bestow.ui.removeClass(elem, "on")
                bestow.ui.setElementText(label, "Off")
            end
        end

        local function setSlider(fillId, labelId, value, maxVal, fmt)
            maxVal = maxVal or 1.0
            local pct = math.floor((value / maxVal) * 100 + 0.5)
            local fill  = bestow.ui.getElementById(doc, fillId)
            local label = bestow.ui.getElementById(doc, labelId)
            if fill then bestow.ui.setStyle(fill, "width", pct .. "%") end
            if label then
                if fmt == "pct" then
                    bestow.ui.setElementText(label, pct .. "%")
                else
                    bestow.ui.setElementText(label, string.format("%.1f", value))
                end
            end
        end

        -- Apply initial UI state
        setToggle("toggle-fullscreen", "toggle-fullscreen-label", s.fullscreen)
        setToggle("toggle-vsync",      "toggle-vsync-label",      s.vsync)
        setSlider("slider-render-scale-fill", "label-render-scale", s.renderScale, 1.0, "float")
        setSlider("slider-master-fill", "label-master", s.masterVolume, 1.0, "pct")
        setSlider("slider-music-fill",  "label-music",  s.musicVolume,  1.0, "pct")
        setSlider("slider-sfx-fill",    "label-sfx",    s.sfxVolume,    1.0, "pct")
        setSlider("slider-ui-fill",     "label-ui",     s.uiVolume,     1.0, "pct")
        setToggle("toggle-shadows", "toggle-shadows-label", s.shadows)
        setToggle("toggle-ssao",    "toggle-ssao-label",    s.ssao)
        setToggle("toggle-bloom",   "toggle-bloom-label",   s.bloom)

        -----------------------------------------------------------------------
        -- Toggle handlers
        -----------------------------------------------------------------------

        local function wireToggle(id, labelId, getter, setter)
            local elem = bestow.ui.getElementById(doc, id)
            bestow.ui.onElementEvent(elem, "click", function()
                local newVal = not getter()
                setter(newVal)
                setToggle(id, labelId, newVal)
            end)
        end

        wireToggle("toggle-fullscreen", "toggle-fullscreen-label",
            function() return s.fullscreen end,
            function(v)
                s.fullscreen = v
                bestow.graphics3d.setFullscreen(v)
            end)

        wireToggle("toggle-vsync", "toggle-vsync-label",
            function() return s.vsync end,
            function(v) s.vsync = v end)

        wireToggle("toggle-shadows", "toggle-shadows-label",
            function() return s.shadows end,
            function(v) s.shadows = v end)

        wireToggle("toggle-ssao", "toggle-ssao-label",
            function() return s.ssao end,
            function(v) s.ssao = v end)

        wireToggle("toggle-bloom", "toggle-bloom-label",
            function() return s.bloom end,
            function(v) s.bloom = v end)

        -----------------------------------------------------------------------
        -- Slider handlers (click cycles through steps for now;
        -- full drag-based sliders would need mouse-move tracking)
        -----------------------------------------------------------------------

        local volumeStep = 0.1

        local function wireVolumeSlider(sliderId, fillId, labelId, getter, setter)
            local elem = bestow.ui.getElementById(doc, sliderId)
            bestow.ui.onElementEvent(elem, "click", function()
                local cur = getter()
                local next = cur + volumeStep
                if next > 1.05 then next = 0.0 end  -- wrap around
                next = math.floor(next * 10 + 0.5) / 10
                setter(next)
                setSlider(fillId, labelId, next, 1.0, "pct")
            end)
        end

        wireVolumeSlider("slider-master", "slider-master-fill", "label-master",
            function() return s.masterVolume end,
            function(v) s.masterVolume = v; bestow.audio.setMasterVolume(v) end)

        wireVolumeSlider("slider-music", "slider-music-fill", "label-music",
            function() return s.musicVolume end,
            function(v) s.musicVolume = v; bestow.audio.setGroupVolume("music", v) end)

        wireVolumeSlider("slider-sfx", "slider-sfx-fill", "label-sfx",
            function() return s.sfxVolume end,
            function(v) s.sfxVolume = v; bestow.audio.setGroupVolume("sfx", v) end)

        wireVolumeSlider("slider-ui", "slider-ui-fill", "label-ui",
            function() return s.uiVolume end,
            function(v) s.uiVolume = v; bestow.audio.setGroupVolume("ui", v) end)

        -----------------------------------------------------------------------
        -- Back button
        -----------------------------------------------------------------------
        local btnBack = bestow.ui.getElementById(doc, "btn-back")
        bestow.ui.onElementEvent(btnBack, "click", function()
            bestow.scene.pop()
        end)

        -- Keyboard/gamepad nav — navigate the toggles and back button
        nav.setItems(doc, {
            "toggle-fullscreen", "toggle-vsync",
            "slider-master", "slider-music", "slider-sfx", "slider-ui",
            "toggle-shadows", "toggle-ssao", "toggle-bloom",
            "btn-back",
        })
        nav.focus("toggle-fullscreen")
        nav.onBack = function() bestow.scene.pop() end

        bestow.events.subscribe("menu:confirm", function(data)
            if data.id == "btn-back" then
                bestow.scene.pop()
            end
            -- Toggles and sliders are handled by their own click handlers
        end)
    end,

    update = function(dt)
        return true
    end,

    exit = function()
        app.systems.menu_nav.clear()
        -- Persistence stub: when save system is implemented,
        -- write app.main.state.settings to the user settings file here.
    end,
}
