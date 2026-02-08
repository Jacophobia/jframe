-- Controls screen
-- Two tabs: Controller / Mouse & Keyboard.
-- Shows current bindings; supports rebinding (listens for next input).

return {
    phase = "controls",

    enter = function(params)
        local nav = app.systems.menu_nav
        local doc = bestow.ui.loadDocument("assets/ui/controls.rml")
        if not doc then return end
        bestow.ui.showDocument(doc)

        -- State
        local activeTab = "controller"  -- "controller" or "keyboard"
        local listening = false         -- true while waiting for a rebind keypress

        -- Element references
        local tabCtrl     = bestow.ui.getElementById(doc, "tab-controller")
        local tabKb       = bestow.ui.getElementById(doc, "tab-keyboard")
        local panelCtrl   = bestow.ui.getElementById(doc, "panel-controller")
        local panelKb     = bestow.ui.getElementById(doc, "panel-keyboard")
        local prompt      = bestow.ui.getElementById(doc, "listening-prompt")
        local btnBack     = bestow.ui.getElementById(doc, "btn-back")

        -- Tab switching
        local function showTab(tab)
            activeTab = tab
            if tab == "controller" then
                bestow.ui.addClass(tabCtrl, "active")
                bestow.ui.removeClass(tabKb, "active")
                bestow.ui.removeClass(panelCtrl, "hidden")
                bestow.ui.addClass(panelKb, "hidden")
            else
                bestow.ui.removeClass(tabCtrl, "active")
                bestow.ui.addClass(tabKb, "active")
                bestow.ui.addClass(panelCtrl, "hidden")
                bestow.ui.removeClass(panelKb, "hidden")
            end
        end

        bestow.ui.onElementEvent(tabCtrl, "click", function() showTab("controller") end)
        bestow.ui.onElementEvent(tabKb,   "click", function() showTab("keyboard") end)

        -- Tab switching via left/right menu actions
        bestow.scene.subscribe("menu:left", function()
            showTab("controller")
        end)
        bestow.scene.subscribe("menu:right", function()
            showTab("keyboard")
        end)

        -- Rebinding flow
        -- When a binding-key element is clicked, start listening for input.
        -- The next key/button press is captured and the binding is updated.
        local function startListening(bindElem)
            listening = true
            bestow.ui.addClass(bindElem, "listening")
            bestow.ui.setElementText(bindElem, "Press a key...")
            bestow.ui.removeClass(prompt, "hidden")
            bestow.input.startListeningForInput()
        end

        local function stopListening(bindElem, newLabel)
            listening = false
            bestow.ui.removeClass(bindElem, "listening")
            if newLabel then
                bestow.ui.setElementText(bindElem, newLabel)
            end
            bestow.ui.addClass(prompt, "hidden")
            bestow.input.stopListeningForInput()
        end

        -- Wire click on each binding-key to start listening
        local bindElems = bestow.ui.getElementsByClass(doc, "binding-key")
        if bindElems then
            for _, elem in ipairs(bindElems) do
                bestow.ui.onElementEvent(elem, "click", function()
                    if not listening then
                        startListening(elem)
                    end
                end)
            end
        end

        -- Back button
        bestow.ui.onElementEvent(btnBack, "click", function()
            bestow.scene.pop()
        end)

        -- Keyboard/gamepad nav
        nav.setItems(doc, {"tab-controller", "tab-keyboard", "btn-back"})
        nav.focus("tab-controller")
        nav.onBack = function() bestow.scene.pop() end

        bestow.scene.subscribe("menu:confirm", function(data)
            if data.id == "tab-controller" then showTab("controller")
            elseif data.id == "tab-keyboard" then showTab("keyboard")
            elseif data.id == "btn-back" then bestow.scene.pop()
            end
        end)

        showTab("controller")
    end,

    update = function(dt)
        -- Check if we're listening for rebind input
        if bestow.input.isListeningForInput() then
            local binding = bestow.input.getLastInput()
            if binding then
                -- A key/button was pressed — rebinding captured
                -- In a full implementation, update the action binding here.
                -- For now, just stop listening (save system not yet implemented).
                bestow.input.stopListeningForInput()
            end
        end
        return true
    end,

    exit = function()
        app.systems.menu_nav.clear()
        if bestow.input.isListeningForInput() then
            bestow.input.stopListeningForInput()
        end
    end,
}
