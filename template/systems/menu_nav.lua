-- Menu Navigation System
-- Provides keyboard/gamepad focus management for UI menus.
-- The RmlUI system has no built-in keyboard/gamepad navigation, so this module
-- tracks focusable elements and moves a ".focused" CSS class between them in
-- response to MenuUp / MenuDown / MenuConfirm / MenuBack actions.

local FOCUSED_CLASS = "focused"

-- Module state — always accessed via app.systems.menu_nav (hot-reload safe)
return {
    -- Current list of focusable element IDs
    items = {},
    -- Index of the currently focused item (1-based)
    index = 0,
    -- The UI document handle these items belong to
    doc = nil,
    -- Callback fired on MenuBack (set by the scene that owns the menu)
    onBack = nil,

    ---------------------------------------------------------------------------
    -- Public API
    ---------------------------------------------------------------------------

    --- Bind this navigator to a document and a list of element IDs.
    ---@param doc  userdata   UI document handle
    ---@param ids  string[]   Ordered list of focusable element IDs (top to bottom)
    setItems = function(doc, ids)
        local self = app.systems.menu_nav
        self.doc   = doc
        self.items = ids
        self.index = 0
        -- Clear any lingering focus class
        for _, id in ipairs(ids) do
            local elem = bestow.ui.getElementById(doc, id)
            if elem then bestow.ui.removeClass(elem, FOCUSED_CLASS) end
        end
    end,

    --- Move focus to a specific element ID.
    ---@param id string  Element ID to focus
    focus = function(id)
        local self = app.systems.menu_nav
        for i, itemId in ipairs(self.items) do
            if itemId == id then
                self._moveTo(i)
                return
            end
        end
    end,

    --- Move focus by a signed offset (+1 = down, -1 = up). Wraps around.
    ---@param offset number
    move = function(offset)
        local self = app.systems.menu_nav
        if #self.items == 0 then return end
        local newIndex = self.index + offset
        -- Wrap around
        if newIndex < 1 then newIndex = #self.items end
        if newIndex > #self.items then newIndex = 1 end
        self._moveTo(newIndex)
    end,

    --- Confirm (click) the currently focused element.
    confirm = function()
        local self = app.systems.menu_nav
        if self.index < 1 or self.index > #self.items then return end
        local id   = self.items[self.index]
        local elem = bestow.ui.getElementById(self.doc, id)
        if elem then
            -- Simulate a click event on the element
            bestow.ui.onElementEvent(elem, "click", nil) -- trigger existing handler
            -- Direct dispatch: the element's click handler is already wired by the scene
            -- We use focus + a manual "click" to bridge keyboard nav to UI events.
            -- The scene wires click handlers on init; we just need to fire the event.
            bestow.ui.focus(elem)
            -- RmlUI dispatches "click" when a focused element receives Enter/Space,
            -- but since we manage focus ourselves we fire it via the Lua event bus.
            bestow.events.emit("menu:confirm", { id = id, index = self.index })
        end
    end,

    --- Clear focus and reset state.
    clear = function()
        local self = app.systems.menu_nav
        if self.doc then
            for _, id in ipairs(self.items) do
                local elem = bestow.ui.getElementById(self.doc, id)
                if elem then bestow.ui.removeClass(elem, FOCUSED_CLASS) end
            end
        end
        self.items = {}
        self.index = 0
        self.doc   = nil
        self.onBack = nil
    end,

    ---------------------------------------------------------------------------
    -- Action event handlers (subscribed in init)
    ---------------------------------------------------------------------------

    init = function()
        local self = app.systems.menu_nav
        bestow.events.subscribe("action:MenuUp",      {}, self, "_onUp")
        bestow.events.subscribe("action:MenuDown",     {}, self, "_onDown")
        bestow.events.subscribe("action:MenuLeft",     {}, self, "_onLeft")
        bestow.events.subscribe("action:MenuRight",    {}, self, "_onRight")
        bestow.events.subscribe("action:MenuConfirm",  {}, self, "_onConfirm")
        bestow.events.subscribe("action:MenuBack",     {}, self, "_onBack")
    end,

    _onUp      = function() app.systems.menu_nav.move(-1) end,
    _onDown    = function() app.systems.menu_nav.move(1)  end,
    _onLeft    = function() bestow.events.emit("menu:left",  {}) end,
    _onRight   = function() bestow.events.emit("menu:right", {}) end,
    _onConfirm = function() app.systems.menu_nav.confirm() end,
    _onBack    = function()
        local self = app.systems.menu_nav
        if self.onBack then self.onBack() end
    end,

    ---------------------------------------------------------------------------
    -- Internal
    ---------------------------------------------------------------------------

    --- Move focus to a 1-based index.
    _moveTo = function(newIndex)
        local self = app.systems.menu_nav
        if not self.doc then return end

        -- Remove focus from old item
        if self.index >= 1 and self.index <= #self.items then
            local oldElem = bestow.ui.getElementById(self.doc, self.items[self.index])
            if oldElem then bestow.ui.removeClass(oldElem, FOCUSED_CLASS) end
        end

        -- Apply focus to new item
        self.index = newIndex
        if self.index >= 1 and self.index <= #self.items then
            local newElem = bestow.ui.getElementById(self.doc, self.items[self.index])
            if newElem then bestow.ui.addClass(newElem, FOCUSED_CLASS) end
        end
    end,
}
