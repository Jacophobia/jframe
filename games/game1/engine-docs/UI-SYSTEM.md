# UI System Developer Guide

**Engine System**: `bestow.ui`
**Implementation**: RmlUi (HTML/CSS-like)
**Interface**: `/Users/jaaaacob/Documents/GameDev/jframe/bestow-contract/src/bestow.ui.cppm`

## Overview

The Bestow UI System provides a complete user interface solution using **RmlUi**, a library that brings HTML/CSS-style UI development to C++ games. If you're familiar with web development, you'll feel right at home.

### What is RmlUi?

RmlUi is a UI framework that uses:
- **RML** - An XML/HTML-like markup language for defining UI structure
- **RCSS** - A CSS2-based stylesheet language for styling UI elements
- **C++ API** - For dynamic UI manipulation and event handling

This gives you the power of web-style UI development with the performance of native C++.

**Web developers:** If you know HTML/CSS, you already know 90% of RML/RCSS. The main differences are in the C++ integration and event handling.

### Key Features

- **Declarative UI** - Define your UI in RML files (like HTML)
- **Flexible Styling** - Style with RCSS (like CSS2)
- **Data Binding** - Sync C++ variables with UI elements automatically
- **Event System** - Handle clicks, hovers, focus, etc. with callbacks
- **Dynamic Creation** - Create and modify UI elements at runtime
- **Document Management** - Multiple UI screens (menus, HUD, pause, etc.)
- **Font Loading** - Custom fonts via TTF support
- **Input Processing** - Automatic input handling with consumption checking

---

## Core Concepts

### 1. Documents (UIDocumentHandle)

A **document** is a complete UI screen (menu, HUD, dialog, etc.). Documents are loaded from `.rml` files and can be shown/hidden independently.

```cpp
// Load a main menu document
auto menuResult = ui->loadDocument("ui/main_menu.rml");
if (menuResult) {
    UIDocumentHandle menu = *menuResult;
    ui->showDocument(menu);
}
```

**Important:** Documents remain loaded in memory even when hidden. Use `hideDocument()` to hide temporarily, or `unloadDocument()` to free memory. Multiple documents can be visible simultaneously (they layer on top of each other).

### 2. Elements (UIElementHandle)

**Elements** are individual UI widgets (buttons, text, containers, etc.). Each element has:
- **ID** - Unique identifier (like HTML `id`)
- **Classes** - Style categories (like HTML `class`)
- **Attributes** - Custom properties
- **Text Content** - Inner text or RML
- **Styles** - Inline CSS properties

```cpp
// Get element by ID
auto buttonHandle = ui->getElementById(menuDoc, "play-button");
if (buttonHandle) {
    ui->setElementText(*buttonHandle, "Start Game");
}
```

### 3. Stylesheets (UIStyleSheetHandle)

**RCSS** files define the visual appearance of your UI using CSS-like syntax. Styles are typically embedded in RML documents via `<link>` tags or `<style>` blocks.

**Note:** The current RmlUi implementation requires stylesheets to be embedded in documents. Programmatic stylesheet loading (`loadStyleSheet()`, `applyStyleSheet()`) is not fully supported yet.

```rcss
/* styles.rcss */
button {
    width: 200px;
    height: 50px;
    background-color: #4CAF50;
    color: white;
    font-size: 18px;
}

button:hover {
    background-color: #45a049;
}
```

### 4. Data Binding

**Data binding** synchronizes C++ variables with UI elements. When you change the variable and call `syncBindings()`, the UI updates.

```cpp
int playerHealth = 100;
int playerScore = 0;

// Bind variables to UI
ui->bindData("health", &playerHealth);
ui->bindData("score", &playerScore);

// In RML: <div>Health: {{health}}/100</div>
// In RML: <div>Score: {{score}}</div>

// Update the variable
playerHealth = 75;
ui->syncBindings();  // UI now shows 75
```

**Critical:** Data binding uses POINTERS to your variables. The variables must remain valid (not go out of scope) while bound. Use member variables or call `unbindData("name")` before destruction.

---

## RML Document Structure

### Basic Template

```xml
<rml>
<head>
    <!-- Link external stylesheets -->
    <link type="text/rcss" href="styles.rcss"/>

    <!-- Inline styles -->
    <style>
        body {
            font-family: "Arial";
            font-size: 14px;
        }
    </style>

    <title>My Game Menu</title>
</head>
<body>
    <!-- Your UI content goes here -->
    <div id="menu-container">
        <h1>Main Menu</h1>
        <button id="play-button">Play</button>
        <button id="settings-button">Settings</button>
        <button id="quit-button">Quit</button>
    </div>
</body>
</rml>
```

### Common RML Elements

| Element | Purpose | Example |
|---------|---------|---------|
| `<div>` | Container/layout box | `<div class="panel">...</div>` |
| `<span>` | Inline text container | `<span>Health: </span>` |
| `<button>` | Interactive button | `<button id="play">Play</button>` |
| `<input>` | Text input field | `<input type="text" id="name"/>` |
| `<img>` | Image element | `<img src="logo.png"/>` |
| `<h1>` - `<h6>` | Headings | `<h1>Title</h1>` |
| `<p>` | Paragraph text | `<p>Description text</p>` |
| `<progress>` | Progress bar | `<progress value="0.75" max="1.0"/>` |
| `<select>` | Dropdown menu | `<select><option>Easy</option></select>` |
| `<textarea>` | Multi-line text input | `<textarea rows="5"></textarea>` |

---

## RCSS Stylesheets

### Basic Syntax

RCSS is based on CSS2 with some CSS3 features. It supports most CSS properties you're familiar with.

```rcss
/* Element selector */
button {
    width: 200px;
    height: 50px;
    background-color: #2196F3;
    color: white;
    border: 2px solid #1976D2;
    font-size: 16px;
}

/* ID selector */
#play-button {
    background-color: #4CAF50;
}

/* Class selector */
.menu-item {
    margin: 10px;
    padding: 15px;
}

/* Pseudo-class selectors */
button:hover {
    background-color: #1976D2;
}

button:active {
    background-color: #0D47A1;
}
```

### Common RCSS Properties

| Property | Example | Description |
|----------|---------|-------------|
| `display` | `display: flex;` | Layout mode |
| `position` | `position: absolute;` | Positioning scheme |
| `width`, `height` | `width: 100px;` | Size |
| `margin`, `padding` | `margin: 10px;` | Spacing |
| `background-color` | `background-color: #FFF;` | Fill color |
| `color` | `color: white;` | Text color |
| `font-size` | `font-size: 16px;` | Text size |
| `border` | `border: 2px solid #000;` | Border style |
| `border-radius` | `border-radius: 8px;` | Rounded corners |
| `opacity` | `opacity: 0.8;` | Transparency |

---

## API Reference

### Handle Types

```cpp
using UIDocumentHandle = std::uint64_t;   // Identifies a loaded RML document
using UIElementHandle = std::uint64_t;    // Identifies an element within a document
using UIStyleSheetHandle = std::uint64_t; // Identifies a loaded stylesheet
```

### Error Types

```cpp
enum class UIError {
    Success,
    DocumentNotFound,    // Document handle is invalid
    ElementNotFound,     // Element handle is invalid
    InvalidDocument,     // RML document is malformed
    ParseError,          // Failed to parse RML
    StyleSheetError,     // Failed to load stylesheet
    FontNotFound,        // Font file not found or invalid
    TextureNotFound,     // Image referenced in RML not found
    InternalError        // Internal RmlUi error
};
```

### Initialization

```cpp
UIConfig config;
config.baseScale = 1.0f;
config.enableDebugMode = false;  // Set true to see element bounds
config.assetsPath = "assets/ui";
config.fontsPath = "assets/fonts";

auto result = ui->initialize(config);
if (!result) {
    // Handle error
}

// Set viewport size (call when window resizes)
ui->setViewportSize(1920, 1080);
ui->setDPIScale(1.0f);  // For high-DPI displays
```

### Document Management

```cpp
// Load document from file (goes through AssetSystem)
auto docResult = ui->loadDocument("ui/main_menu.rml");
if (docResult) {
    UIDocumentHandle menu = *docResult;

    // Show the document
    ui->showDocument(menu);

    // Hide the document (still loaded)
    ui->hideDocument(menu);

    // Check visibility
    if (ui->isDocumentVisible(menu)) {
        // Document is visible
    }

    // Unload when done
    ui->unloadDocument(menu);
}

// Load document from string
std::string rmlContent = R"(
    <rml>
    <head><title>Inline</title></head>
    <body><h1>Hello!</h1></body>
    </rml>
)";
auto inlineDoc = ui->loadDocumentFromString(rmlContent, "inline-menu");

// Get all loaded documents
auto allDocs = ui->getLoadedDocuments();
```

### Element Access

```cpp
// Get element by ID
auto btnHandle = ui->getElementById(menuDoc, "play-button");
if (btnHandle) {
    // Element found
}

// Get elements by class name
auto menuItems = ui->getElementsByClass(menuDoc, "menu-item");
for (auto item : menuItems) {
    ui->setElementText(item, "Item");
}

// Get elements by tag name
auto allButtons = ui->getElementsByTag(menuDoc, "button");

// Navigate element tree
auto children = ui->getChildren(containerHandle);
auto parent = ui->getParent(childHandle);
```

### Element Properties

```cpp
// Text content
ui->setElementText(labelHandle, "Health: 100");
std::string text = ui->getElementText(labelHandle);

// Visibility
ui->setElementVisible(elemHandle, UIVisibility::Visible);   // Visible
ui->setElementVisible(elemHandle, UIVisibility::Hidden);    // Hidden but takes space
ui->setElementVisible(elemHandle, UIVisibility::Collapsed); // Hidden and takes no space

auto visibility = ui->getElementVisibility(elemHandle);

// Classes (for styling)
ui->addElementClass(btnHandle, "active");
ui->removeElementClass(btnHandle, "disabled");
bool hasClass = ui->hasElementClass(btnHandle, "active");

// Attributes
ui->setElementAttribute(inputHandle, "placeholder", "Enter name");
auto attrValue = ui->getElementAttribute(inputHandle, "placeholder");

// Inline styles
ui->setElementStyle(btnHandle, "background-color", "#FF0000");
ui->setElementStyle(btnHandle, "font-size", "20px");

// Bounding box (for positioning/collision)
UIRect bounds = ui->getElementBounds(btnHandle);
// bounds.x, bounds.y, bounds.width, bounds.height

// Focus management
ui->focusElement(inputHandle);  // Give keyboard focus
ui->blurElement(inputHandle);   // Remove keyboard focus
```

### Dynamic Element Creation

```cpp
// Create a new element
UIElementHandle newDiv = ui->createElement(docHandle, "div");
ui->setElementText(newDiv, "Dynamic content");
ui->setElementStyle(newDiv, "color", "red");

// Append child elements
ui->appendChild(parentHandle, newDiv);

// Set inner RML (replace children)
ui->setInnerRml(containerHandle, "<h1>Title</h1><p>Text</p>");

// Remove element
ui->removeElement(newDiv);
```

### Data Binding

```cpp
// Declare your game variables
int playerHealth = 100;
float playerSpeed = 5.5f;
bool isPaused = false;
std::string playerName = "Hero";

// Bind them to the UI
ui->bindData("health", &playerHealth);
ui->bindData("speed", &playerSpeed);
ui->bindData("paused", &isPaused);
ui->bindData("name", &playerName);

// In your RML:
// <div>Health: {{health}}</div>
// <div>Speed: {{speed}}</div>
// <div>Paused: {{paused}}</div>
// <div>Name: {{name}}</div>

// Update variables in game loop
playerHealth -= 10;  // Player took damage
ui->syncBindings();  // UI updates to show new health

// Unbind when done
ui->unbindData("health");
```

**Note:** Data binding synchronization (`syncBindings()`) is currently a TODO in the implementation. Bound data is stored but not yet automatically synchronized with the UI.

### Event Handling

```cpp
// Register global event callback (all elements)
ui->registerEventCallback("click", [](const UIEventData& data) {
    std::cout << "Clicked element: " << data.targetId << std::endl;
    std::cout << "Event type: " << data.eventType << std::endl;
    std::cout << "Mouse position: " << data.mouseX << ", " << data.mouseY << std::endl;
});

// Register callback for specific element
auto playBtn = ui->getElementById(menuDoc, "play-button");
if (playBtn) {
    ui->registerElementCallback(*playBtn, "click", [](const UIEventData& data) {
        // Start the game!
        startGame();
    });
}

// Common event types:
// - "click" - Mouse click
// - "dblclick" - Double click
// - "mouseover" - Mouse enters element
// - "mouseout" - Mouse leaves element
// - "focus" - Element gains focus
// - "blur" - Element loses focus
// - "change" - Input value changed
// - "submit" - Form submitted

// Unregister event callbacks
ui->unregisterEventCallback("click");
```

**Note:** Per-element callbacks (`registerElementCallback()`) are currently a TODO in the implementation. Use global callbacks and check the `data.targetId` to identify which element triggered the event.

### Input Processing

```cpp
// Convert from IInputSystem to UIInputEvent
UIInputEvent toUIInput(const InputEvent& gameEvent) {
    UIInputEvent uiEvent;
    uiEvent.type = UIInputType::MouseMove;  // or MouseDown, KeyDown, etc.
    uiEvent.x = gameEvent.mouseX;
    uiEvent.y = gameEvent.mouseY;
    uiEvent.button = gameEvent.button;
    uiEvent.keyCode = gameEvent.keyCode;
    uiEvent.modifiers = gameEvent.modifiers;
    return uiEvent;
}

// In your input handling code:
UIInputEvent event = toUIInput(gameEvent);

// Process the event
bool uiConsumed = ui->processInput(event);
if (uiConsumed) {
    // UI handled the event, don't pass to game
    return;
}

// Check if UI wants input
if (ui->wantsKeyboardInput()) {
    // UI has focus (e.g., text input), don't process game keys
}

if (ui->wantsMouseInput()) {
    // Mouse is over UI, don't process game mouse clicks
}
```

**Note:** `wantsKeyboardInput()` and `wantsMouseInput()` currently return `false` (TODO in implementation). You may need to track this manually by checking if text input elements are focused.

### Update & Render

```cpp
// In your game loop:
void update(DeltaTime dt) {
    ui->update(dt);  // Update RmlUi context
}

void render() {
    // Render your game...

    // Then render UI on top
    ui->render();
}
```

### Font Loading

```cpp
// Load a font (goes through AssetSystem)
auto result = ui->loadFont("fonts/Roboto-Regular.ttf", "Roboto");
if (!result) {
    // Handle error
}

// Use in RCSS:
// body { font-family: "Roboto"; }
```

### Debug Mode

```cpp
// Enable debug overlay (shows element bounds, IDs, etc.)
ui->setDebugMode(true);

// Get stats
size_t elementCount = ui->getElementCount();
std::cout << "Active UI elements: " << elementCount << std::endl;
```

### Input Event Types

```cpp
enum class UIInputType : std::uint8_t {
    MouseMove,      // Mouse moved
    MouseDown,      // Mouse button pressed
    MouseUp,        // Mouse button released
    MouseScroll,    // Mouse wheel scrolled
    KeyDown,        // Keyboard key pressed
    KeyUp,          // Keyboard key released
    TextInput       // Text character input
};

struct UIInputEvent {
    UIInputType type;
    int x = 0;             // Mouse X position
    int y = 0;             // Mouse Y position
    int button = 0;        // Mouse button (0=left, 1=right, 2=middle)
    int wheelDelta = 0;    // Mouse wheel delta
    int keyCode = 0;       // Keyboard key code
    int modifiers = 0;     // Shift, Ctrl, Alt flags
    char32_t character = 0;// Unicode character for text input
};
```

### UIVisibility Enum

```cpp
enum class UIVisibility : std::uint8_t {
    Visible,    // Element visible and takes space
    Hidden,     // Element hidden but takes space (layout preserved)
    Collapsed   // Element hidden and takes no space (removed from layout)
};
```

### UIRect Structure

```cpp
struct UIRect {
    float x = 0.0f;        // X position (screen coordinates)
    float y = 0.0f;        // Y position (screen coordinates)
    float width = 0.0f;    // Width in pixels
    float height = 0.0f;   // Height in pixels
};
```

---

## Common UI Patterns

### 1. Main Menu

**ui/main_menu.rml:**
```xml
<rml>
<head>
    <link type="text/rcss" href="ui/menu_styles.rcss"/>
    <title>Main Menu</title>
</head>
<body>
    <div id="menu-background">
        <div class="menu-container">
            <img src="ui/logo.png" class="game-logo"/>

            <div class="button-list">
                <button id="btn-new-game" class="menu-button">New Game</button>
                <button id="btn-continue" class="menu-button">Continue</button>
                <button id="btn-settings" class="menu-button">Settings</button>
                <button id="btn-quit" class="menu-button">Quit</button>
            </div>
        </div>
    </div>
</body>
</rml>
```

**ui/menu_styles.rcss:**
```rcss
body {
    font-family: "Arial", sans-serif;
    font-size: 16px;
}

#menu-background {
    position: absolute;
    width: 100%;
    height: 100%;
    background-color: rgba(0, 0, 0, 0.9);
}

.menu-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    width: 100%;
    height: 100%;
    gap: 20px;
}

.menu-button {
    width: 300px;
    height: 60px;
    background-color: #2196F3;
    color: white;
    font-size: 20px;
    font-weight: bold;
    border: 2px solid #1976D2;
    border-radius: 8px;
}

.menu-button:hover {
    background-color: #1976D2;
}
```

**C++ Code:**
```cpp
class MainMenu {
public:
    void init(IUISystem* ui) {
        ui_ = ui;

        // Load menu document
        auto result = ui_->loadDocument("ui/main_menu.rml");
        if (result) {
            menuDoc_ = *result;
            setupEventHandlers();
            ui_->showDocument(menuDoc_);
        }
    }

    void setupEventHandlers() {
        // New Game button
        auto newGameBtn = ui_->getElementById(menuDoc_, "btn-new-game");
        if (newGameBtn) {
            ui_->registerElementCallback(*newGameBtn, "click", [this](const UIEventData&) {
                startNewGame();
            });
        }

        // Quit button
        auto quitBtn = ui_->getElementById(menuDoc_, "btn-quit");
        if (quitBtn) {
            ui_->registerElementCallback(*quitBtn, "click", [this](const UIEventData&) {
                quitGame();
            });
        }
    }

private:
    IUISystem* ui_ = nullptr;
    UIDocumentHandle menuDoc_;

    void startNewGame() { /* ... */ }
    void quitGame() { /* ... */ }
};
```

### 2. HUD with Health Bar

**ui/game_hud.rml:**
```xml
<rml>
<head>
    <link type="text/rcss" href="ui/hud_styles.rcss"/>
    <title>Game HUD</title>
</head>
<body>
    <div id="hud-container">
        <div class="hud-panel" id="stats-panel">
            <div class="stat-item">
                <span class="stat-label">Health:</span>
                <div class="health-bar-container">
                    <div id="health-bar" class="health-bar" style="width: 100%;"></div>
                </div>
                <span id="health-text" class="stat-value">100/100</span>
            </div>

            <div class="stat-item">
                <span class="stat-label">Score:</span>
                <span id="score-text" class="stat-value">0</span>
            </div>
        </div>
    </div>
</body>
</rml>
```

**C++ Code:**
```cpp
class GameHUD {
public:
    void init(IUISystem* ui) {
        ui_ = ui;

        auto result = ui_->loadDocument("ui/game_hud.rml");
        if (result) {
            hudDoc_ = *result;

            // Cache element handles
            healthBar_ = ui_->getElementById(hudDoc_, "health-bar");
            healthText_ = ui_->getElementById(hudDoc_, "health-text");
            scoreText_ = ui_->getElementById(hudDoc_, "score-text");

            ui_->showDocument(hudDoc_);
        }
    }

    void updateHealth(int current, int max) {
        if (!healthBar_ || !healthText_) return;

        float percentage = static_cast<float>(current) / max * 100.0f;
        ui_->setElementStyle(*healthBar_, "width", std::to_string(percentage) + "%");

        // Change color based on health
        if (percentage < 25.0f) {
            ui_->setElementStyle(*healthBar_, "background-color", "#F44336");  // Red
        } else if (percentage < 50.0f) {
            ui_->setElementStyle(*healthBar_, "background-color", "#FF9800");  // Orange
        } else {
            ui_->setElementStyle(*healthBar_, "background-color", "#4CAF50");  // Green
        }

        ui_->setElementText(*healthText_,
            std::to_string(current) + "/" + std::to_string(max));
    }

    void updateScore(int score) {
        if (!scoreText_) return;
        ui_->setElementText(*scoreText_, std::to_string(score));
    }

private:
    IUISystem* ui_ = nullptr;
    UIDocumentHandle hudDoc_;
    std::optional<UIElementHandle> healthBar_;
    std::optional<UIElementHandle> healthText_;
    std::optional<UIElementHandle> scoreText_;
};
```

### 3. Pause Menu

**ui/pause_menu.rml:**
```xml
<rml>
<head>
    <link type="text/rcss" href="ui/pause_styles.rcss"/>
    <title>Pause Menu</title>
</head>
<body>
    <div id="pause-overlay">
        <div class="pause-container">
            <h1>Paused</h1>

            <div class="pause-buttons">
                <button id="btn-resume" class="pause-btn">Resume</button>
                <button id="btn-settings" class="pause-btn">Settings</button>
                <button id="btn-main-menu" class="pause-btn">Main Menu</button>
                <button id="btn-quit" class="pause-btn">Quit</button>
            </div>
        </div>
    </div>
</body>
</rml>
```

**C++ Code:**
```cpp
class PauseMenu {
public:
    void init(IUISystem* ui) {
        ui_ = ui;

        auto result = ui_->loadDocument("ui/pause_menu.rml");
        if (result) {
            pauseDoc_ = *result;
            setupHandlers();
        }
    }

    void setupHandlers() {
        // Resume
        auto resumeBtn = ui_->getElementById(pauseDoc_, "btn-resume");
        if (resumeBtn) {
            ui_->registerElementCallback(*resumeBtn, "click", [this](const UIEventData&) {
                hide();
                resumeGame();
            });
        }

        // Main Menu
        auto mainMenuBtn = ui_->getElementById(pauseDoc_, "btn-main-menu");
        if (mainMenuBtn) {
            ui_->registerElementCallback(*mainMenuBtn, "click", [this](const UIEventData&) {
                returnToMainMenu();
            });
        }
    }

    void show() {
        ui_->showDocument(pauseDoc_);
    }

    void hide() {
        ui_->hideDocument(pauseDoc_);
    }

private:
    IUISystem* ui_ = nullptr;
    UIDocumentHandle pauseDoc_;

    void resumeGame() { /* ... */ }
    void returnToMainMenu() { /* ... */ }
};
```

---

## Best Practices

### 1. UI Organization

**Recommended File Structure:**
```
assets/
  ui/
    fonts/
      Roboto-Regular.ttf
      Roboto-Bold.ttf
    textures/
      logo.png
      button_bg.png
    documents/
      main_menu.rml
      game_hud.rml
      pause_menu.rml
    styles/
      common.rcss       # Shared styles
      menu_styles.rcss
      hud_styles.rcss
```

### 2. Performance Considerations

**DO:**
- Cache element handles instead of querying every frame
- Hide documents instead of destroying/recreating
- Use `UIVisibility::Collapsed` for hidden elements that shouldn't take space
- Load fonts and documents during initialization, not gameplay

**DON'T:**
- Query elements by ID/class every frame
- Create/destroy elements in hot loops
- Forget to unload unused documents
- Use `setInnerRml()` unnecessarily (it's expensive)

```cpp
// GOOD: Cache handles
class MyUI {
    std::optional<UIElementHandle> healthText_;

    void init() {
        healthText_ = ui->getElementById(doc, "health-text");
    }

    void update() {
        if (healthText_) {
            ui->setElementText(*healthText_, std::to_string(health));
        }
    }
};

// BAD: Query every frame
void update() {
    auto healthText = ui->getElementById(doc, "health-text");  // Slow!
    if (healthText) {
        ui->setElementText(*healthText, std::to_string(health));
    }
}
```

### 3. Input Handling

Always check if UI consumed input before passing to game:

```cpp
void handleInput(const InputEvent& event) {
    // Convert to UIInputEvent
    UIInputEvent uiEvent = convertToUIEvent(event);

    // Let UI process first
    if (ui->processInput(uiEvent)) {
        // UI consumed the event, don't pass to game
        return;
    }

    // UI didn't handle it, process in game
    game->handleInput(event);
}
```

### 4. State Management

Organize UI into logical screens:

```cpp
class UIManager {
public:
    enum class Screen {
        None,
        MainMenu,
        GameHUD,
        PauseMenu
    };

    void showScreen(Screen screen) {
        hideAll();

        switch (screen) {
            case Screen::MainMenu:
                ui->showDocument(mainMenuDoc_);
                break;
            case Screen::GameHUD:
                ui->showDocument(hudDoc_);
                break;
            case Screen::PauseMenu:
                ui->showDocument(pauseDoc_);
                break;
        }

        currentScreen_ = screen;
    }

    void hideAll() {
        ui->hideDocument(mainMenuDoc_);
        ui->hideDocument(hudDoc_);
        ui->hideDocument(pauseDoc_);
    }

private:
    Screen currentScreen_ = Screen::None;
    UIDocumentHandle mainMenuDoc_;
    UIDocumentHandle hudDoc_;
    UIDocumentHandle pauseDoc_;
};
```

---

## Implementation Notes

The current RmlUi implementation has some features that are work-in-progress:

### TODO / Not Yet Implemented

1. **Data binding synchronization** - `syncBindings()` stores bindings but doesn't sync to UI yet
2. **Per-element callbacks** - `registerElementCallback()` needs RmlUi event listener integration
3. **Input focus detection** - `wantsKeyboardInput()` and `wantsMouseInput()` always return false
4. **Programmatic stylesheet loading** - `loadStyleSheet()` and `applyStyleSheet()` not supported by RmlUi's API (use `<link>` tags in RML instead)
5. **Element ownership transfer** - `appendChild()` needs proper ownership handling

### Workarounds

**For stylesheets:** Embed them in RML documents:
```xml
<head>
    <link type="text/rcss" href="styles.rcss"/>
    <!-- Or inline: -->
    <style>
        button { background-color: #2196F3; }
    </style>
</head>
```

**For per-element callbacks:** Use global callbacks and check `data.targetId`:
```cpp
ui->registerEventCallback("click", [](const UIEventData& data) {
    if (data.targetId == "play-button") {
        startGame();
    } else if (data.targetId == "quit-button") {
        quitGame();
    }
});
```

---

## Troubleshooting

### Common Issues

**1. Document fails to load**
- Check file path is correct
- Ensure RML is well-formed XML
- Check console for parse errors
- Verify AssetSystem can access the file

**2. Styles not applying**
- Link stylesheet in `<head>` with `<link type="text/rcss" href="..."/>`
- Check RCSS syntax (must be valid CSS)
- Verify element classes/IDs match selectors
- Enable debug mode to visualize elements

**3. Events not firing**
- Verify element handle is valid
- Check event type string is correct ("click", not "onclick")
- Ensure document is visible
- Remember per-element callbacks are TODO - use global callbacks

**4. Text not updating**
- Cache element handles during initialization
- Check if element was destroyed/recreated
- Verify handle is still valid

**5. UI not rendering**
- Ensure `ui->update(dt)` is called every frame
- Ensure `ui->render()` is called after game rendering
- Check that document is shown with `showDocument()`
- Verify RmlUi was initialized by the graphics system

---

## Additional Resources

### RmlUi Official Documentation
- [RmlUi Documentation](https://mikke89.github.io/RmlUiDoc/)
- [RCSS Reference](https://mikke89.github.io/RmlUiDoc/pages/rcss.html)
- [RML Element Index](https://mikke89.github.io/RmlUiDoc/pages/rml.html)
- [Data Bindings Guide](https://mikke89.github.io/RmlUiDoc/pages/data_bindings.html)

### Bestow Documentation
- Contract Interface: `/Users/jaaaacob/Documents/GameDev/jframe/bestow-contract/src/bestow.ui.cppm`
- Implementation: `/Users/jaaaacob/Documents/GameDev/jframe/bestow-ui/src/bestow.ui.impl.cppm`

---

## Quick Reference

### Common Event Types
| Event | Triggered When |
|-------|----------------|
| `"click"` | Element clicked |
| `"dblclick"` | Element double-clicked |
| `"mouseover"` | Mouse enters element |
| `"mouseout"` | Mouse leaves element |
| `"focus"` | Element gains focus |
| `"blur"` | Element loses focus |
| `"change"` | Input value changed |
| `"submit"` | Form submitted |

### UIVisibility Values
| Value | Behavior |
|-------|----------|
| `UIVisibility::Visible` | Element visible and takes space |
| `UIVisibility::Hidden` | Element hidden but takes space |
| `UIVisibility::Collapsed` | Element hidden and takes no space |

---

## Conclusion

The Bestow UI System provides a powerful, web-style approach to game UI development. By combining RML documents for structure, RCSS stylesheets for presentation, and the C++ API for behavior, you can create professional user interfaces for your games.

Key takeaways:
- Structure your UI with RML (HTML-like)
- Style with RCSS (CSS-like)
- Handle events and updates in C++
- Cache element handles for performance
- Always check if UI consumed input before passing to game
- Use document visibility management for screen transitions
- Load all assets through AssetSystem (never direct file I/O)

Happy UI building!
