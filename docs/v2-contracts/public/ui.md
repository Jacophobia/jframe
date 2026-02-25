# UI System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 6
> **Dependencies:** Types, UIRenderBackend, GraphicsContext, Input, Assets
> **Lua Paths:** `bestow.ui` (high-level), `bestow.ui.core` (low-level)

## Purpose

The UI System manages document-based user interfaces for menus, HUDs, overlays, and in-game UI. It uses an HTML/CSS-like document model (RML/RCSS via RmlUi) where UI layouts are defined in markup documents and styled with CSS-like stylesheets. The system supports element access by ID, class, and tag, dynamic DOM manipulation, data binding for live value updates, event handling, font loading, and viewport scaling. The high-level API provides simple document loading, element text changes, class toggling, and event subscription, while the low-level API exposes the full DOM manipulation, data binding, stylesheet management, input routing, font management, viewport control, and debug utilities.

## High-Level API: `IUISystem`

The simplified API for common UI tasks. Load and show documents, find elements, set text, toggle classes, and subscribe to events. No lifecycle methods -- the engine manages those internally.

### Document Management

| Method | Returns | Description |
|--------|---------|-------------|
| `loadDocument(std::string_view path)` | `Result<UIDocHandle>` | Load a UI document from a file path (.rml or .html) and return its handle |
| `showDocument(UIDocHandle doc)` | `Result<void>` | Make a loaded document visible and interactive |
| `hideDocument(UIDocHandle doc)` | `Result<void>` | Hide a document (invisible but still loaded in memory) |

### Element Access

| Method | Returns | Description |
|--------|---------|-------------|
| `getElementById(UIDocHandle doc, std::string_view id)` | `std::optional<UIElementHandle>` | Find an element within a document by its ID attribute |

### Element Properties

| Method | Returns | Description |
|--------|---------|-------------|
| `setText(UIElementHandle elem, std::string_view text)` | `Result<void>` | Set the inner text content of an element |
| `addClass(UIElementHandle elem, std::string_view cls)` | `Result<void>` | Add a CSS class to an element |
| `removeClass(UIElementHandle elem, std::string_view cls)` | `Result<void>` | Remove a CSS class from an element |

### Events

| Method | Returns | Description |
|--------|---------|-------------|
| `onEvent(std::string_view eventType, std::function<void(const UIEventData&)> callback)` | `SubscriptionId` | Subscribe to a UI event type (e.g., "click", "hover", "submit") across all documents |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered UI event subscription |

## Low-Level API: `IUICore`

Full control API. Exposes lifecycle, document loading from strings, unloading, visibility queries, stylesheet management, element access by class/tag, parent/child traversal, full element property control, dynamic DOM creation, data binding, per-element events, input routing, font loading, viewport configuration, and debug mode.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize(const UIConfig& config)` | `Result<void>` | Initialize the UI system with the given configuration |
| `shutdown()` | `void` | Shut down the UI system and release all resources |
| `update(DeltaTime dt)` | `void` | Process data binding updates and animation timers |
| `render()` | `void` | Render all visible documents to the screen |

### Document Management

| Method | Returns | Description |
|--------|---------|-------------|
| `loadDocument(std::string_view path)` | `Result<UIDocHandle>` | Load a UI document from a file path |
| `loadDocumentFromString(std::string_view content, std::string_view name = "inline")` | `Result<UIDocHandle>` | Load a document from inline RML/HTML markup |
| `unloadDocument(UIDocHandle doc)` | `Result<void>` | Unload a document and free all associated resources |
| `showDocument(UIDocHandle doc)` | `Result<void>` | Make a document visible |
| `hideDocument(UIDocHandle doc)` | `Result<void>` | Hide a document |
| `isDocumentVisible(UIDocHandle doc)` | `bool` | Return whether a document is currently visible |

### Stylesheets

| Method | Returns | Description |
|--------|---------|-------------|
| `loadStyleSheet(std::string_view path)` | `Result<void>` | Load a stylesheet from a file path (.rcss or .css) |
| `applyStyleSheet(UIDocHandle doc, std::string_view stylePath)` | `Result<void>` | Apply a loaded stylesheet to a specific document |

### Element Access

| Method | Returns | Description |
|--------|---------|-------------|
| `getElementById(UIDocHandle doc, std::string_view id)` | `std::optional<UIElementHandle>` | Find an element by its ID attribute within a document |
| `getElementsByClass(UIDocHandle doc, std::string_view className)` | `std::vector<UIElementHandle>` | Find all elements with the given CSS class within a document |
| `getElementsByTag(UIDocHandle doc, std::string_view tagName)` | `std::vector<UIElementHandle>` | Find all elements with the given tag name within a document |
| `getChildren(UIElementHandle elem)` | `std::vector<UIElementHandle>` | Return the immediate child elements of the given element |
| `getParent(UIElementHandle elem)` | `std::optional<UIElementHandle>` | Return the parent element, or nullopt if this is a root element |

### Element Properties

| Method | Returns | Description |
|--------|---------|-------------|
| `setText(UIElementHandle elem, std::string_view text)` | `Result<void>` | Set the inner text content of an element |
| `getText(UIElementHandle elem)` | `std::string` | Return the inner text content of an element |
| `setVisible(UIElementHandle elem, bool visible)` | `Result<void>` | Set the visibility of an individual element |
| `isVisible(UIElementHandle elem)` | `bool` | Return whether an element is currently visible |
| `addClass(UIElementHandle elem, std::string_view cls)` | `Result<void>` | Add a CSS class to an element |
| `removeClass(UIElementHandle elem, std::string_view cls)` | `Result<void>` | Remove a CSS class from an element |
| `hasClass(UIElementHandle elem, std::string_view cls)` | `bool` | Check whether an element has the given CSS class |
| `setAttribute(UIElementHandle elem, std::string_view name, std::string_view value)` | `Result<void>` | Set an HTML attribute on an element |
| `getAttribute(UIElementHandle elem, std::string_view name)` | `std::optional<std::string>` | Get the value of an HTML attribute, or nullopt if not present |
| `setStyle(UIElementHandle elem, std::string_view property, std::string_view value)` | `Result<void>` | Set an inline CSS style property on an element |

### Dynamic DOM

| Method | Returns | Description |
|--------|---------|-------------|
| `createElement(UIDocHandle doc, std::string_view tag)` | `Result<UIElementHandle>` | Create a new element with the given tag name within a document |
| `appendChild(UIElementHandle parent, UIElementHandle child)` | `Result<void>` | Append a child element to a parent element |
| `removeElement(UIElementHandle elem)` | `Result<void>` | Remove an element from its parent and destroy it |
| `setInnerRml(UIElementHandle elem, std::string_view rml)` | `Result<void>` | Replace the inner content of an element with raw RML markup |

### Data Binding

| Method | Returns | Description |
|--------|---------|-------------|
| `bindInt(std::string_view name, int* value)` | `Result<void>` | Bind an integer variable to a data model name for automatic UI updates |
| `bindFloat(std::string_view name, float* value)` | `Result<void>` | Bind a float variable to a data model name |
| `bindBool(std::string_view name, bool* value)` | `Result<void>` | Bind a boolean variable to a data model name |
| `bindString(std::string_view name, std::string* value)` | `Result<void>` | Bind a string variable to a data model name |
| `unbindData(std::string_view name)` | `void` | Remove a data binding by name |
| `syncBindings()` | `void` | Force all data bindings to synchronize their values to the UI |

### Events

| Method | Returns | Description |
|--------|---------|-------------|
| `onEvent(std::string_view eventType, std::function<void(const UIEventData&)> callback)` | `SubscriptionId` | Subscribe to a UI event type across all documents |
| `onElementEvent(UIElementHandle elem, std::string_view eventType, std::function<void(const UIEventData&)> callback)` | `SubscriptionId` | Subscribe to a UI event type on a specific element |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a UI event subscription |

### Input

| Method | Returns | Description |
|--------|---------|-------------|
| `processInput(const UIInputEvent& event)` | `bool` | Route an input event to the UI system; returns true if the UI consumed the event |
| `wantsKeyboardInput()` | `bool` | Return true if the UI has an active text input or focused element that wants keyboard events |
| `wantsMouseInput()` | `bool` | Return true if the mouse is over a UI element and the UI should consume mouse events |

### Fonts

| Method | Returns | Description |
|--------|---------|-------------|
| `loadFont(std::string_view path, std::string_view familyName = "")` | `Result<void>` | Load a font file (.ttf, .otf) for use in UI documents; optionally specify a family name |

### Viewport

| Method | Returns | Description |
|--------|---------|-------------|
| `setViewportSize(int width, int height)` | `void` | Set the UI viewport dimensions (typically matches window size) |
| `setDPIScale(float scale)` | `void` | Set the DPI scale factor for high-resolution displays |

### Debug

| Method | Returns | Description |
|--------|---------|-------------|
| `setDebugMode(bool enabled)` | `void` | Enable or disable debug mode (shows element outlines and info) |
| `getElementCount()` | `std::size_t` | Return the total number of elements across all loaded documents |

## Types

### UIConfig

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `baseScale` | `float` | `1.0f` | Global UI scale factor multiplier |
| `enableDebugMode` | `bool` | `false` | Whether to show debug overlays on startup |
| `assetsPath` | `std::filesystem::path` | `""` | Base path for resolving UI asset references |
| `fontsPath` | `std::filesystem::path` | `""` | Path to the fonts directory |

### UIEventData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `document` | `UIDocHandle` | -- | Handle of the document that owns the event target |
| `element` | `UIElementHandle` | -- | Handle of the element that triggered the event |
| `eventType` | `std::string` | `""` | Event type string (e.g., "click", "hover", "focus", "submit") |
| `targetId` | `std::string` | `""` | ID attribute of the target element |
| `targetClass` | `std::string` | `""` | Class attribute of the target element |
| `mouseX` | `int` | `0` | Mouse X position at the time of the event |
| `mouseY` | `int` | `0` | Mouse Y position at the time of the event |

### UIInputEvent

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `UIInputType` | -- | Input event type (MouseMove, MouseDown, MouseUp, MouseScroll, KeyDown, KeyUp, TextInput) |
| `x` | `int` | `0` | Mouse X position or horizontal coordinate |
| `y` | `int` | `0` | Mouse Y position or vertical coordinate |
| `button` | `int` | `0` | Mouse button (0 = left, 1 = right, 2 = middle) |
| `wheelDelta` | `int` | `0` | Mouse scroll wheel delta |
| `keyCode` | `int` | `0` | Keyboard key code |
| `modifiers` | `int` | `0` | Modifier key flags (Shift, Ctrl, Alt) |
| `character` | `char32_t` | `0` | Unicode character for TextInput events |

### UIInputType

| Value | Description |
|-------|-------------|
| `MouseMove` | Mouse cursor movement |
| `MouseDown` | Mouse button pressed |
| `MouseUp` | Mouse button released |
| `MouseScroll` | Mouse scroll wheel moved |
| `KeyDown` | Keyboard key pressed |
| `KeyUp` | Keyboard key released |
| `TextInput` | Text character input |

### UIVertex

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `position` | `Vec2` | `{0,0}` | Screen-space vertex position |
| `texCoord` | `Vec2` | `{0,0}` | Texture coordinate |
| `color` | `Color` | `{1,1,1,1}` | Vertex color |

### UIDocHandle

`Handle<UIDocTag>` -- Strongly typed handle identifying a loaded UI document. Valid handles are non-zero.

### UIElementHandle

`Handle<UIElementTag>` -- Strongly typed handle identifying a specific element within a document. Valid handles are non-zero.

## Lua Examples

```lua
-- High-level: Load and display a menu
local doc, err = bestow.ui.loadDocument("ui/main_menu.rml")
if err then print("UI error: " .. err.message) return end

bestow.ui.showDocument(doc)

-- Find and modify elements
local title = bestow.ui.getElementById(doc, "title")
if title then
    bestow.ui.setText(title, "Welcome to Bestow!")
    bestow.ui.addClass(title, "highlighted")
end

-- Subscribe to button clicks
local subId = bestow.ui.onEvent("click", function(event)
    if event.targetId == "startButton" then
        bestow.scene.pushScene("gameplay")
    elseif event.targetId == "quitButton" then
        -- handle quit
    end
end)

-- Clean up
bestow.ui.unsubscribe(subId)
bestow.ui.hideDocument(doc)

-- Low-level: Full DOM manipulation
local doc, err = bestow.ui.core.loadDocument("ui/inventory.rml")
bestow.ui.core.showDocument(doc)

-- Query elements by class
local slots = bestow.ui.core.getElementsByClass(doc, "item-slot")
for _, slot in ipairs(slots) do
    bestow.ui.core.addClass(slot, "empty")
end

-- Create dynamic elements
local list = bestow.ui.core.getElementById(doc, "itemList")
for i, item in ipairs(inventory) do
    local elem, err = bestow.ui.core.createElement(doc, "div")
    if elem then
        bestow.ui.core.addClass(elem, "item-entry")
        bestow.ui.core.setText(elem, item.name)
        bestow.ui.core.setAttribute(elem, "data-id", tostring(item.id))
        bestow.ui.core.appendChild(list, elem)
    end
end

-- Set inner RML
bestow.ui.core.setInnerRml(list, "<div class='empty'>No items</div>")

-- Data binding (updates UI automatically when values change)
bestow.ui.core.bindInt("playerHP", hpPointer)
bestow.ui.core.bindString("playerName", namePointer)
bestow.ui.core.syncBindings()

-- Per-element events
local btn = bestow.ui.core.getElementById(doc, "craftButton")
bestow.ui.core.onElementEvent(btn, "click", function(event)
    print("Craft clicked!")
end)

-- Stylesheets
bestow.ui.core.loadStyleSheet("ui/dark_theme.rcss")
bestow.ui.core.applyStyleSheet(doc, "ui/dark_theme.rcss")

-- Font loading
bestow.ui.core.loadFont("fonts/Roboto-Regular.ttf", "Roboto")

-- Viewport and DPI
bestow.ui.core.setViewportSize(1920, 1080)
bestow.ui.core.setDPIScale(2.0)

-- Debug
bestow.ui.core.setDebugMode(true)
print("Total elements: " .. bestow.ui.core.getElementCount())
```

## C++ Examples

```cpp
// High-level: Simple menu UI
auto docResult = ui->loadDocument("ui/main_menu.rml");
if (!docResult) {
    spdlog::error("UI: {}", docResult.error().message);
    return;
}
UIDocHandle doc = *docResult;
ui->showDocument(doc);

auto title = ui->getElementById(doc, "title");
if (title) {
    ui->setText(*title, "Welcome to Bestow!");
    ui->addClass(*title, "highlighted");
}

auto subId = ui->onEvent("click",
    [&](const UIEventData& event) {
        if (event.targetId == "startButton") {
            scene->pushScene("gameplay");
        }
    });

// Low-level: Full DOM control
auto docResult = uiCore->loadDocument("ui/inventory.rml");
UIDocHandle doc = *docResult;
uiCore->showDocument(doc);

// Query by class
auto slots = uiCore->getElementsByClass(doc, "item-slot");
for (auto slot : slots) {
    uiCore->addClass(slot, "empty");
}

// Dynamic DOM
auto list = uiCore->getElementById(doc, "itemList");
for (const auto& item : inventory) {
    auto elemResult = uiCore->createElement(doc, "div");
    if (elemResult) {
        auto elem = *elemResult;
        uiCore->addClass(elem, "item-entry");
        uiCore->setText(elem, item.name);
        uiCore->setAttribute(elem, "data-id", std::to_string(item.id));
        uiCore->appendChild(*list, elem);
    }
}

// Data binding
int playerHP = 100;
std::string playerName = "Hero";
uiCore->bindInt("playerHP", &playerHP);
uiCore->bindString("playerName", &playerName);

// When values change, sync to UI
playerHP = 75;
uiCore->syncBindings();

// Per-element event
auto btn = uiCore->getElementById(doc, "craftButton");
if (btn) {
    uiCore->onElementEvent(*btn, "click",
        [](const UIEventData& event) {
            spdlog::info("Craft button clicked!");
        });
}

// Input routing
bool consumed = uiCore->processInput(UIInputEvent{
    .type = UIInputType::MouseDown,
    .x = mouseX,
    .y = mouseY,
    .button = 0
});

if (uiCore->wantsKeyboardInput()) {
    // Don't pass keyboard events to game systems
}

// Font and stylesheet
uiCore->loadFont("fonts/Roboto-Regular.ttf", "Roboto");
uiCore->loadStyleSheet("ui/dark_theme.rcss");
uiCore->applyStyleSheet(doc, "ui/dark_theme.rcss");

// Viewport
uiCore->setViewportSize(1920, 1080);
uiCore->setDPIScale(2.0f);

// Debug
uiCore->setDebugMode(true);
spdlog::info("Total UI elements: {}", uiCore->getElementCount());

// Cleanup
uiCore->unsubscribe(subId);
uiCore->unloadDocument(doc);
```
