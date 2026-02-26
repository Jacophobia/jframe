# UI System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 5
> **Dependencies:** Types, Assets, Graphics Context, UI Render Backend, Input
> **Lua Paths:** `bestow.ui` (high-level), `bestow.ui.core` (low-level)

## Purpose

The UI System provides document-based user interface rendering for menus, HUDs, dialogs, and overlays. It uses RML (RmlUi markup language) documents with CSS-like styling. The high-level API provides simple document loading and element manipulation including text changes, visibility toggling, class management, click handlers, and input capture queries. The low-level API exposes full DOM access with element queries by ID, class, and tag, parent/child traversal, dynamic element creation and removal, data binding for live value updates, per-element and global event handling, stylesheet management, input routing, font loading, viewport configuration, and debug utilities.

## High-Level API: `IUISystem`

The simplified API for common UI tasks. Load and show documents, set text, toggle classes, register click handlers, and query input capture state. No lifecycle methods -- the engine manages those internally.

### Document Management

| Method | Returns | Description |
|--------|---------|-------------|
| `loadDocument(std::string_view path)` | `Result<UIDocHandle>` | Load and display a UI document from file |
| `hideDocument(UIDocHandle doc)` | `void` | Hide a loaded document |
| `showDocument(UIDocHandle doc)` | `void` | Show a hidden document |
| `unloadDocument(UIDocHandle doc)` | `void` | Unload document and free resources |

### Element Properties

| Method | Returns | Description |
|--------|---------|-------------|
| `setText(UIDocHandle doc, std::string_view elementId, std::string_view text)` | `Result<void>` | Set text content of an element by ID |
| `getText(UIDocHandle doc, std::string_view elementId)` | `Result<std::string>` | Get text content of an element by ID |
| `setVisible(UIDocHandle doc, std::string_view elementId, bool visible)` | `Result<void>` | Show or hide a specific element |
| `addClass(UIDocHandle doc, std::string_view elementId, std::string_view className)` | `Result<void>` | Add CSS class to element |
| `removeClass(UIDocHandle doc, std::string_view elementId, std::string_view className)` | `Result<void>` | Remove CSS class from element |

### Events

| Method | Returns | Description |
|--------|---------|-------------|
| `onClick(UIDocHandle doc, std::string_view elementId, std::function<void()> callback)` | `SubscriptionId` | Register click handler for a specific element |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered event subscription |

### Input

| Method | Returns | Description |
|--------|---------|-------------|
| `wantsInput()` | `bool` | Check if UI is capturing input (for input passthrough decisions) |

## Low-Level API: `IUICore`

Full control API. Exposes lifecycle, document loading from strings, unloading, visibility queries, stylesheet management, element access by ID/class/tag, parent/child traversal, full element property control, dynamic DOM creation, data binding, per-element and global events, input routing, font loading, viewport configuration, and debug mode.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize(const UIConfig& config)` | `Result<void>` | Initialize the UI system with the given configuration |
| `shutdown()` | `void` | Shut down the UI system and release all resources |
| `update(DeltaTime dt)` | `void` | Process data binding updates, animation timers, and pending events |
| `render()` | `void` | Render all visible documents to the screen |

### Document Management

| Method | Returns | Description |
|--------|---------|-------------|
| `loadDocument(std::filesystem::path path)` | `Result<UIDocHandle>` | Load a UI document from a file path (.rml or .html) and return its handle |
| `loadDocumentFromString(std::string_view content, std::string_view sourceName)` | `Result<UIDocHandle>` | Load a document from inline RML/HTML markup |
| `unloadDocument(UIDocHandle doc)` | `void` | Unload a document and free all associated resources |
| `showDocument(UIDocHandle doc)` | `void` | Make a document visible and interactive |
| `hideDocument(UIDocHandle doc)` | `void` | Hide a document (invisible but still loaded in memory) |
| `isDocumentVisible(UIDocHandle doc)` | `bool` | Return whether a document is currently visible |
| `getLoadedDocuments()` | `std::vector<UIDocHandle>` | Return handles to all currently loaded documents |

### Stylesheets

| Method | Returns | Description |
|--------|---------|-------------|
| `loadStyleSheet(std::filesystem::path path)` | `Result<UIStyleSheetHandle>` | Load a stylesheet from a file path (.rcss or .css) and return its handle |
| `applyStyleSheet(UIDocHandle doc, UIStyleSheetHandle sheet)` | `Result<void>` | Apply a loaded stylesheet to a specific document |

### Element Access

| Method | Returns | Description |
|--------|---------|-------------|
| `getElementById(UIDocHandle doc, std::string_view id)` | `std::optional<UIElementHandle>` | Find an element within a document by its ID attribute |
| `getElementsByClass(UIDocHandle doc, std::string_view className)` | `std::vector<UIElementHandle>` | Find all elements with the given CSS class within a document |
| `getElementsByTag(UIDocHandle doc, std::string_view tagName)` | `std::vector<UIElementHandle>` | Find all elements with the given tag name within a document |
| `getChildren(UIElementHandle elem)` | `std::vector<UIElementHandle>` | Return the immediate child elements of the given element |
| `getParent(UIElementHandle elem)` | `std::optional<UIElementHandle>` | Return the parent element, or nullopt if this is a root element |

### Element Properties

| Method | Returns | Description |
|--------|---------|-------------|
| `setElementText(UIElementHandle elem, std::string_view text)` | `void` | Set the inner text content of an element |
| `getElementText(UIElementHandle elem)` | `std::string` | Return the inner text content of an element |
| `setElementVisible(UIElementHandle elem, UIVisibility visibility)` | `void` | Set the visibility of an individual element |
| `getElementVisibility(UIElementHandle elem)` | `UIVisibility` | Return the current visibility state of an element |
| `addElementClass(UIElementHandle elem, std::string_view className)` | `void` | Add a CSS class to an element |
| `removeElementClass(UIElementHandle elem, std::string_view className)` | `void` | Remove a CSS class from an element |
| `hasElementClass(UIElementHandle elem, std::string_view className)` | `bool` | Check whether an element has the given CSS class |
| `setElementAttribute(UIElementHandle elem, std::string_view name, std::string_view value)` | `void` | Set an HTML attribute on an element |
| `getElementAttribute(UIElementHandle elem, std::string_view name)` | `std::optional<std::string>` | Get the value of an HTML attribute, or nullopt if not present |
| `setElementStyle(UIElementHandle elem, std::string_view property, std::string_view value)` | `void` | Set an inline CSS style property on an element |
| `getElementBounds(UIElementHandle elem)` | `UIRect` | Return the bounding rectangle of an element in viewport coordinates |
| `focusElement(UIElementHandle elem)` | `void` | Give keyboard focus to an element |
| `blurElement(UIElementHandle elem)` | `void` | Remove keyboard focus from an element |

### Dynamic Elements

| Method | Returns | Description |
|--------|---------|-------------|
| `createElement(UIDocHandle doc, std::string_view tagName)` | `UIElementHandle` | Create a new element with the given tag name within a document |
| `appendChild(UIElementHandle parent, UIElementHandle child)` | `void` | Append a child element to a parent element |
| `removeElement(UIElementHandle elem)` | `void` | Remove an element from its parent and destroy it |
| `setInnerRml(UIElementHandle elem, std::string_view rml)` | `void` | Replace the inner content of an element with raw RML markup |

### Data Binding

| Method | Returns | Description |
|--------|---------|-------------|
| `bindData(std::string_view name, int* value)` | `void` | Bind an integer variable to a data model name for automatic UI updates |
| `bindData(std::string_view name, float* value)` | `void` | Bind a float variable to a data model name |
| `bindData(std::string_view name, bool* value)` | `void` | Bind a boolean variable to a data model name |
| `bindData(std::string_view name, std::string* value)` | `void` | Bind a string variable to a data model name |
| `unbindData(std::string_view name)` | `void` | Remove a data binding by name |
| `syncBindings()` | `void` | Force all data bindings to synchronize their values to the UI |

### Events

| Method | Returns | Description |
|--------|---------|-------------|
| `registerEventCallback(std::string_view eventType, UIEventCallback callback)` | `void` | Subscribe to a UI event type (e.g., "click", "hover", "submit") across all documents |
| `registerElementCallback(UIElementHandle elem, std::string_view eventType, UIEventCallback callback)` | `void` | Subscribe to a UI event type on a specific element |
| `unregisterEventCallback(std::string_view eventType)` | `void` | Remove a global event callback by event type |

### Input

| Method | Returns | Description |
|--------|---------|-------------|
| `processInput(const UIInputEvent& event)` | `bool` | Route an input event to the UI system; returns true if the UI consumed the event |
| `wantsKeyboardInput()` | `bool` | Return true if the UI has an active text input or focused element that wants keyboard events |
| `wantsMouseInput()` | `bool` | Return true if the mouse is over a UI element and the UI should consume mouse events |

### Fonts

| Method | Returns | Description |
|--------|---------|-------------|
| `loadFont(std::filesystem::path path, std::string_view familyName)` | `Result<void>` | Load a font file (.ttf, .otf) for use in UI documents with the given family name |

### Viewport

| Method | Returns | Description |
|--------|---------|-------------|
| `setViewportSize(int width, int height)` | `void` | Set the UI viewport dimensions (typically matches window size) |
| `setDPIScale(float scale)` | `void` | Set the DPI scale factor for high-resolution displays |

### Debug

| Method | Returns | Description |
|--------|---------|-------------|
| `setDebugMode(bool enabled)` | `void` | Enable or disable debug mode (shows element outlines and layout info) |
| `getElementCount()` | `std::size_t` | Return the total number of elements across all loaded documents |

## Types

### UIDocHandle

`Handle<UIDocTag>` -- Strongly typed handle identifying a loaded UI document. Valid handles are non-zero.

### UIElementHandle

`Handle<UIElementTag>` -- Strongly typed handle identifying a specific element within a document. Valid handles are non-zero.

### UIStyleSheetHandle

`Handle<UIStyleSheetTag>` -- Strongly typed handle identifying a loaded stylesheet. Valid handles are non-zero.

### UIConfig

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `baseScale` | `float` | `1.0f` | Global UI scale factor multiplier |
| `enableDebugMode` | `bool` | `false` | Whether to show debug overlays on startup |
| `assetsPath` | `std::filesystem::path` | `""` | Base path for resolving UI asset references |
| `fontsPath` | `std::filesystem::path` | `""` | Path to the fonts directory |

### UIVisibility

```cpp
enum class UIVisibility : std::uint8_t {
    Visible,    // Element is rendered and occupies layout space
    Hidden,     // Element is invisible but still occupies layout space
    Collapsed   // Element is invisible and removed from layout flow
};
```

### UIRect

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x` | `float` | `0.0f` | X position of the bounding rectangle |
| `y` | `float` | `0.0f` | Y position of the bounding rectangle |
| `width` | `float` | `0.0f` | Width of the bounding rectangle |
| `height` | `float` | `0.0f` | Height of the bounding rectangle |

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
| `type` | `UIInputType` | -- | Input event type |
| `x` | `int` | `0` | Mouse X position or horizontal coordinate |
| `y` | `int` | `0` | Mouse Y position or vertical coordinate |
| `button` | `int` | `0` | Mouse button (0 = left, 1 = right, 2 = middle) |
| `wheelDelta` | `int` | `0` | Mouse scroll wheel delta |
| `keyCode` | `int` | `0` | Keyboard key code |
| `modifiers` | `int` | `0` | Modifier key flags (Shift, Ctrl, Alt) |
| `character` | `char32_t` | `0` | Unicode character for TextInput events |

### UIInputType

```cpp
enum class UIInputType : std::uint8_t {
    MouseMove,    // Mouse cursor movement
    MouseDown,    // Mouse button pressed
    MouseUp,      // Mouse button released
    MouseScroll,  // Mouse scroll wheel moved
    KeyDown,      // Keyboard key pressed
    KeyUp,        // Keyboard key released
    TextInput     // Text character input
};
```

### UIEventCallback

```cpp
using UIEventCallback = std::function<void(const UIEventData&)>;
```

## Lua Mapping

The UI System maps to two Lua namespaces:

- **`bestow.ui`** maps to `IUISystem` (high-level). The high-level API uses document handles and element ID strings directly: `bestow.ui.setText(doc, "title", "Hello")`.
- **`bestow.ui.core`** maps to `IUICore` (low-level). The low-level API uses element handles for direct DOM manipulation: `bestow.ui.core.setElementText(elem, "Hello")`.

`Result<T>` returns follow the Lua multi-return convention: `value, err` where `err` is `nil` on success or a table with a `message` field on failure. `std::optional` maps to a value or `nil`. Handle types (`UIDocHandle`, `UIElementHandle`, `UIStyleSheetHandle`) are opaque userdata. `UIVisibility` maps to string values: `"visible"`, `"hidden"`, `"collapsed"`. `UIRect` maps to a Lua table with `x`, `y`, `width`, `height` fields. `UIEventData` maps to a Lua table with the same field names. `UIInputEvent` maps to a Lua table; `UIInputType` uses string values matching the enum names.

## Examples

### Lua

```lua
-- High-level: Load and display a menu
local doc, err = bestow.ui.loadDocument("ui/main_menu.rml")
if err then print("UI error: " .. err.message) return end

bestow.ui.showDocument(doc)

-- Set and get element text
bestow.ui.setText(doc, "title", "Welcome to Bestow!")
local text = bestow.ui.getText(doc, "title")

-- Toggle element visibility
bestow.ui.setVisible(doc, "loadingSpinner", false)

-- Manage CSS classes
bestow.ui.addClass(doc, "title", "highlighted")
bestow.ui.removeClass(doc, "title", "dimmed")

-- Register click handlers
local subId = bestow.ui.onClick(doc, "startButton", function()
    bestow.scene.pushScene("gameplay")
end)

local quitSubId = bestow.ui.onClick(doc, "quitButton", function()
    bestow.quit()
end)

-- Check if UI wants input (for passthrough)
if bestow.ui.wantsInput() then
    -- Don't pass input to game systems
end

-- Clean up
bestow.ui.unsubscribe(subId)
bestow.ui.unsubscribe(quitSubId)
bestow.ui.hideDocument(doc)
bestow.ui.unloadDocument(doc)

-- Low-level: Full DOM manipulation
local doc, err = bestow.ui.core.loadDocument("ui/inventory.rml")
bestow.ui.core.showDocument(doc)

-- Query elements
local title = bestow.ui.core.getElementById(doc, "inventoryTitle")
if title then
    bestow.ui.core.setElementText(title, "Inventory (32 items)")
    bestow.ui.core.addElementClass(title, "bold")
end

local slots = bestow.ui.core.getElementsByClass(doc, "item-slot")
for _, slot in ipairs(slots) do
    bestow.ui.core.addElementClass(slot, "empty")
end

local divs = bestow.ui.core.getElementsByTag(doc, "div")

-- Parent/child traversal
local children = bestow.ui.core.getChildren(title)
local parent = bestow.ui.core.getParent(title)

-- Element properties
bestow.ui.core.setElementVisible(title, "visible")
local vis = bestow.ui.core.getElementVisibility(title)
bestow.ui.core.setElementAttribute(title, "data-count", "32")
local attr = bestow.ui.core.getElementAttribute(title, "data-count")
bestow.ui.core.setElementStyle(title, "color", "red")
local bounds = bestow.ui.core.getElementBounds(title)
print("Bounds: " .. bounds.x .. ", " .. bounds.y .. ", " .. bounds.width .. ", " .. bounds.height)
bestow.ui.core.focusElement(title)
bestow.ui.core.blurElement(title)

-- Dynamic element creation
local list = bestow.ui.core.getElementById(doc, "itemList")
for i, item in ipairs(inventory) do
    local elem = bestow.ui.core.createElement(doc, "div")
    bestow.ui.core.addElementClass(elem, "item-entry")
    bestow.ui.core.setElementText(elem, item.name)
    bestow.ui.core.setElementAttribute(elem, "data-id", tostring(item.id))
    bestow.ui.core.appendChild(list, elem)
end

-- Replace inner content with RML
bestow.ui.core.setInnerRml(list, "<div class='empty'>No items</div>")

-- Remove elements
local obsolete = bestow.ui.core.getElementById(doc, "oldWidget")
if obsolete then
    bestow.ui.core.removeElement(obsolete)
end

-- Data binding (updates UI automatically when bound values change)
bestow.ui.core.bindData("playerHP", hpPointer)
bestow.ui.core.bindData("playerName", namePointer)
bestow.ui.core.bindData("isAlive", alivePointer)
bestow.ui.core.syncBindings()
bestow.ui.core.unbindData("playerHP")

-- Global and per-element events
bestow.ui.core.registerEventCallback("click", function(event)
    print("Clicked: " .. event.targetId)
end)

local btn = bestow.ui.core.getElementById(doc, "craftButton")
bestow.ui.core.registerElementCallback(btn, "click", function(event)
    print("Craft clicked!")
end)

bestow.ui.core.unregisterEventCallback("click")

-- Stylesheets
local sheet, err = bestow.ui.core.loadStyleSheet("ui/dark_theme.rcss")
if sheet then
    bestow.ui.core.applyStyleSheet(doc, sheet)
end

-- Fonts
bestow.ui.core.loadFont("fonts/Roboto-Regular.ttf", "Roboto")

-- Viewport and DPI
bestow.ui.core.setViewportSize(1920, 1080)
bestow.ui.core.setDPIScale(2.0)

-- Input routing
local consumed = bestow.ui.core.processInput({
    type = "MouseDown",
    x = mouseX,
    y = mouseY,
    button = 0
})

if bestow.ui.core.wantsKeyboardInput() then
    -- UI has focus, skip game input
end

-- Debug
bestow.ui.core.setDebugMode(true)
print("Total elements: " .. bestow.ui.core.getElementCount())

-- List loaded documents
local docs = bestow.ui.core.getLoadedDocuments()
for _, d in ipairs(docs) do
    print("Visible: " .. tostring(bestow.ui.core.isDocumentVisible(d)))
end

-- Load from inline RML string
local inlineDoc, err = bestow.ui.core.loadDocumentFromString([[
    <rml>
    <head><title>Tooltip</title></head>
    <body><p id="tip">Hover text</p></body>
    </rml>
]], "tooltip")
```

### C++

```cpp
// High-level: Simple menu UI
auto docResult = ui->loadDocument("ui/main_menu.rml");
if (!docResult) {
    spdlog::error("UI: {}", docResult.error().message);
    return;
}
UIDocHandle doc = *docResult;
ui->showDocument(doc);

// Set and get element text
ui->setText(doc, "title", "Welcome to Bestow!");
auto text = ui->getText(doc, "title"); // Result<std::string>

// Toggle element visibility
ui->setVisible(doc, "loadingSpinner", false);

// Manage CSS classes
ui->addClass(doc, "title", "highlighted");
ui->removeClass(doc, "title", "dimmed");

// Register click handler
auto subId = ui->onClick(doc, "startButton", [&]() {
    scene->pushScene("gameplay");
});

// Check input capture
if (ui->wantsInput()) {
    // Don't pass input to game systems
}

// Clean up
ui->unsubscribe(subId);
ui->hideDocument(doc);
ui->unloadDocument(doc);

// Low-level: Full DOM control
auto docResult = uiCore->loadDocument("ui/inventory.rml");
UIDocHandle doc = *docResult;
uiCore->showDocument(doc);

// Query elements by ID, class, tag
auto title = uiCore->getElementById(doc, "inventoryTitle");
if (title) {
    uiCore->setElementText(*title, "Inventory (32 items)");
    uiCore->addElementClass(*title, "bold");
}

auto slots = uiCore->getElementsByClass(doc, "item-slot");
for (auto slot : slots) {
    uiCore->addElementClass(slot, "empty");
}

auto divs = uiCore->getElementsByTag(doc, "div");

// Parent/child traversal
auto children = uiCore->getChildren(*title);
auto parent = uiCore->getParent(*title);

// Element properties
uiCore->setElementVisible(*title, UIVisibility::Visible);
UIVisibility vis = uiCore->getElementVisibility(*title);
uiCore->setElementAttribute(*title, "data-count", "32");
auto attr = uiCore->getElementAttribute(*title, "data-count");
uiCore->setElementStyle(*title, "color", "red");
UIRect bounds = uiCore->getElementBounds(*title);
uiCore->focusElement(*title);
uiCore->blurElement(*title);

// Dynamic DOM creation
auto list = uiCore->getElementById(doc, "itemList");
for (const auto& item : inventory) {
    auto elem = uiCore->createElement(doc, "div");
    uiCore->addElementClass(elem, "item-entry");
    uiCore->setElementText(elem, item.name);
    uiCore->setElementAttribute(elem, "data-id", std::to_string(item.id));
    uiCore->appendChild(*list, elem);
}

// Replace inner content
uiCore->setInnerRml(*list, "<div class='empty'>No items</div>");

// Remove element
auto obsolete = uiCore->getElementById(doc, "oldWidget");
if (obsolete) {
    uiCore->removeElement(*obsolete);
}

// Data binding
int playerHP = 100;
float stamina = 75.0f;
bool isAlive = true;
std::string playerName = "Hero";
uiCore->bindData("playerHP", &playerHP);
uiCore->bindData("stamina", &stamina);
uiCore->bindData("isAlive", &isAlive);
uiCore->bindData("playerName", &playerName);

// When values change, sync to UI
playerHP = 75;
uiCore->syncBindings();

// Unbind when done
uiCore->unbindData("playerHP");

// Global event callback
uiCore->registerEventCallback("click",
    [](const UIEventData& event) {
        spdlog::info("Clicked element: {}", event.targetId);
    });

// Per-element event callback
auto btn = uiCore->getElementById(doc, "craftButton");
if (btn) {
    uiCore->registerElementCallback(*btn, "click",
        [](const UIEventData& event) {
            spdlog::info("Craft button clicked!");
        });
}

uiCore->unregisterEventCallback("click");

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

if (uiCore->wantsMouseInput()) {
    // Don't pass mouse events to game systems
}

// Stylesheets
auto sheetResult = uiCore->loadStyleSheet("ui/dark_theme.rcss");
if (sheetResult) {
    uiCore->applyStyleSheet(doc, *sheetResult);
}

// Fonts
uiCore->loadFont("fonts/Roboto-Regular.ttf", "Roboto");

// Viewport and DPI
uiCore->setViewportSize(1920, 1080);
uiCore->setDPIScale(2.0f);

// Debug
uiCore->setDebugMode(true);
spdlog::info("Total UI elements: {}", uiCore->getElementCount());

// List loaded documents
auto docs = uiCore->getLoadedDocuments();
for (auto d : docs) {
    spdlog::info("Document visible: {}", uiCore->isDocumentVisible(d));
}

// Load from inline RML string
auto inlineResult = uiCore->loadDocumentFromString(R"(
    <rml>
    <head><title>Tooltip</title></head>
    <body><p id="tip">Hover text</p></body>
    </rml>
)", "tooltip");

// Cleanup
uiCore->unloadDocument(doc);
```
