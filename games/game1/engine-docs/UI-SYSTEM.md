# Bestow UI System Guide

**Engine System**: `bestow.ui`
**Implementation**: RmlUi (HTML/CSS-like)
**Contract**: `/Users/jaaaacob/Documents/GameDev/jframe/bestow-contract/src/bestow.ui.cppm`
**Implementation**: `/Users/jaaaacob/Documents/GameDev/jframe/bestow-ui/src/bestow.ui.impl.cppm`

## Overview

The Bestow UI System provides a complete user interface solution using **RmlUi**, a library that brings HTML/CSS-style UI development to C++ games. If you're familiar with web development, you'll feel right at home.

### What is RmlUi?

RmlUi is a UI framework that uses:
- **RML** - An XML/HTML-like markup language for defining UI structure
- **RCSS** - A CSS2-based stylesheet language for styling UI elements
- **C++ API** - For dynamic UI manipulation and event handling

This gives you the power of web-style UI development with the performance of native C++.

### Key Features

- **Declarative UI** - Define your UI in RML files (like HTML)
- **Flexible Styling** - Style with RCSS (like CSS2/CSS3)
- **Data Binding** - Sync C++ variables with UI elements automatically
- **Event System** - Handle clicks, hovers, focus, etc. with callbacks
- **Dynamic Creation** - Create and modify UI elements at runtime
- **Document Management** - Multiple UI screens (menus, HUD, pause, etc.)
- **Asset Integration** - Loads through AssetSystem for hot reload support

---

## Core Concepts

### 1. Documents

A **document** is a complete UI screen (menu, HUD, dialog, etc.). Documents are loaded from `.rml` files and can be shown/hidden independently.

```cpp
// Load a main menu document
auto menuResult = ui->loadDocument("ui/main_menu.rml");
if (menuResult) {
    UIDocumentHandle menu = *menuResult;
    ui->showDocument(menu);
}
```

### 2. Elements

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

### 3. Stylesheets (RCSS)

**RCSS** files define the visual appearance of your UI using CSS-like syntax. Styles are applied via:
- Element tags (`button { ... }`)
- IDs (`#play-button { ... }`)
- Classes (`.menu-item { ... }`)
- Pseudo-classes (`:hover`, `:active`, `:focus`)

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

**Data binding** synchronizes C++ variables with UI elements automatically. When you change the variable, the UI updates.

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

### Layout Example

```xml
<body>
    <!-- Vertical menu layout -->
    <div id="main-menu" class="vertical-layout">
        <img src="logo.png" class="logo"/>

        <div class="button-group">
            <button id="btn-play">Start Game</button>
            <button id="btn-continue">Continue</button>
            <button id="btn-settings">Settings</button>
            <button id="btn-quit">Quit</button>
        </div>

        <div class="footer">
            <span>Version 1.0.0</span>
        </div>
    </div>
</body>
```

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
    cursor: pointer;
}

button:active {
    background-color: #0D47A1;
}

.text-input:focus {
    border-color: #FF9800;
}
```

### Common RCSS Properties

#### Layout & Positioning

```rcss
.container {
    display: block;              /* block, inline, inline-block, flex, none */
    position: absolute;          /* static, relative, absolute, fixed */
    top: 50px;
    left: 100px;
    width: 300px;
    height: 200px;
    margin: 10px;                /* margin-top, margin-right, etc. */
    padding: 15px;               /* padding-top, padding-right, etc. */
}
```

#### Flexbox (CSS3)

```rcss
.flex-container {
    display: flex;
    flex-direction: row;         /* row, column, row-reverse, column-reverse */
    justify-content: center;     /* flex-start, flex-end, center, space-between */
    align-items: center;         /* flex-start, flex-end, center, stretch */
    gap: 10px;                   /* Space between items */
}

.flex-item {
    flex: 1;                     /* Grow to fill space */
    flex-grow: 1;
    flex-shrink: 0;
    flex-basis: auto;
}
```

#### Colors & Backgrounds

```rcss
.styled-box {
    background-color: #FF5722;
    background-color: rgba(255, 87, 34, 0.8);  /* With alpha */
    color: white;
    opacity: 0.9;
}
```

#### Borders & Outlines

```rcss
.bordered {
    border: 2px solid #333;
    border-radius: 8px;          /* Rounded corners */
    border-top-left-radius: 4px; /* Individual corners */
}
```

#### Text Styling

```rcss
.text-style {
    font-family: "Arial", sans-serif;
    font-size: 18px;
    font-weight: bold;           /* normal, bold, or 100-900 */
    font-style: italic;          /* normal, italic */
    text-align: center;          /* left, center, right, justify */
    line-height: 1.5;
    letter-spacing: 2px;
    text-decoration: underline;  /* none, underline, line-through */
}
```

#### Visibility

```rcss
.hidden {
    visibility: hidden;          /* hidden, visible */
    display: none;               /* Removes from layout */
}
```

### Units

| Unit | Description | Example |
|------|-------------|---------|
| `px` | Pixels (absolute) | `width: 200px;` |
| `%` | Percentage of parent | `width: 50%;` |
| `em` | Relative to font size | `margin: 1.5em;` |
| `rem` | Relative to root font size | `padding: 2rem;` |
| `dp` | Density-independent pixels | `font-size: 16dp;` |

### Animations (Basic)

```rcss
/* Transitions */
button {
    background-color: #2196F3;
    transition: background-color 0.3s ease;
}

button:hover {
    background-color: #1976D2;
}
```

---

## API Reference

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
// Load document from file
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

### Input Processing

```cpp
// In your input handling code:
UIInputEvent event;
event.type = UIInputType::MouseMove;
event.x = mouseX;
event.y = mouseY;

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

### Update & Render

```cpp
// In your game loop:
void update(DeltaTime dt) {
    ui->update(dt);  // Update animations, transitions, etc.
}

void render() {
    // Render your game...

    // Then render UI on top
    ui->render();
}
```

### Font Loading

```cpp
// Load a font
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

            <div class="version-info">
                <span>Version 1.0.0</span>
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

.game-logo {
    width: 400px;
    height: 200px;
    margin-bottom: 40px;
}

.button-list {
    display: flex;
    flex-direction: column;
    gap: 15px;
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
    transition: background-color 0.3s ease;
}

.menu-button:hover {
    background-color: #1976D2;
    cursor: pointer;
}

.menu-button:active {
    background-color: #0D47A1;
}

.version-info {
    position: absolute;
    bottom: 20px;
    right: 20px;
    color: #888;
    font-size: 12px;
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

            // Register event handlers
            setupEventHandlers();

            // Show menu
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

        // Continue button
        auto continueBtn = ui_->getElementById(menuDoc_, "btn-continue");
        if (continueBtn) {
            // Disable if no save exists
            if (!hasSaveGame()) {
                ui_->setElementVisible(*continueBtn, UIVisibility::Hidden);
            } else {
                ui_->registerElementCallback(*continueBtn, "click", [this](const UIEventData&) {
                    continueGame();
                });
            }
        }

        // Settings button
        auto settingsBtn = ui_->getElementById(menuDoc_, "btn-settings");
        if (settingsBtn) {
            ui_->registerElementCallback(*settingsBtn, "click", [this](const UIEventData&) {
                openSettings();
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
    void continueGame() { /* ... */ }
    void openSettings() { /* ... */ }
    void quitGame() { /* ... */ }
    bool hasSaveGame() { return true; }
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
        <!-- Top-left: Health and stats -->
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

            <div class="stat-item">
                <span class="stat-label">Lives:</span>
                <span id="lives-text" class="stat-value">3</span>
            </div>
        </div>

        <!-- Top-right: Mini-map (placeholder) -->
        <div class="hud-panel" id="minimap-panel">
            <div class="minimap">
                <!-- Minimap rendering goes here -->
            </div>
        </div>

        <!-- Bottom-center: Action prompt -->
        <div id="action-prompt">
            <span id="action-text"></span>
        </div>
    </div>
</body>
</rml>
```

**ui/hud_styles.rcss:**
```rcss
body {
    font-family: "Arial", sans-serif;
    font-size: 14px;
}

#hud-container {
    position: absolute;
    width: 100%;
    height: 100%;
    pointer-events: none;  /* Let clicks pass through to game */
}

.hud-panel {
    background-color: rgba(0, 0, 0, 0.7);
    border: 2px solid rgba(255, 255, 255, 0.3);
    border-radius: 8px;
    padding: 15px;
    pointer-events: auto;  /* Enable interaction for panels */
}

#stats-panel {
    position: absolute;
    top: 20px;
    left: 20px;
    width: 250px;
}

.stat-item {
    display: flex;
    align-items: center;
    margin-bottom: 10px;
    gap: 10px;
}

.stat-label {
    color: #AAA;
    font-size: 14px;
    min-width: 60px;
}

.stat-value {
    color: white;
    font-size: 16px;
    font-weight: bold;
}

.health-bar-container {
    flex: 1;
    height: 20px;
    background-color: #333;
    border: 1px solid #666;
    border-radius: 4px;
    overflow: hidden;
}

.health-bar {
    height: 100%;
    background: linear-gradient(to bottom, #4CAF50, #2E7D32);
    transition: width 0.3s ease;
}

#minimap-panel {
    position: absolute;
    top: 20px;
    right: 20px;
    width: 200px;
    height: 200px;
}

.minimap {
    width: 100%;
    height: 100%;
    background-color: #222;
}

#action-prompt {
    position: absolute;
    bottom: 100px;
    left: 50%;
    transform: translateX(-50%);
    background-color: rgba(0, 0, 0, 0.8);
    padding: 10px 20px;
    border-radius: 5px;
    opacity: 0;
    transition: opacity 0.3s ease;
}

#action-prompt.visible {
    opacity: 1;
}

#action-text {
    color: white;
    font-size: 16px;
}
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

            // Get element handles
            healthBar_ = ui_->getElementById(hudDoc_, "health-bar");
            healthText_ = ui_->getElementById(hudDoc_, "health-text");
            scoreText_ = ui_->getElementById(hudDoc_, "score-text");
            livesText_ = ui_->getElementById(hudDoc_, "lives-text");
            actionText_ = ui_->getElementById(hudDoc_, "action-text");
            actionPrompt_ = ui_->getElementById(hudDoc_, "action-prompt");

            ui_->showDocument(hudDoc_);
        }
    }

    void updateHealth(int current, int max) {
        if (!healthBar_ || !healthText_) return;

        float percentage = static_cast<float>(current) / max * 100.0f;
        ui_->setElementStyle(*healthBar_, "width", std::to_string(percentage) + "%");

        // Change color based on health
        if (percentage < 25.0f) {
            ui_->setElementStyle(*healthBar_, "background", "#F44336");  // Red
        } else if (percentage < 50.0f) {
            ui_->setElementStyle(*healthBar_, "background", "#FF9800");  // Orange
        } else {
            ui_->setElementStyle(*healthBar_, "background", "#4CAF50");  // Green
        }

        ui_->setElementText(*healthText_, std::to_string(current) + "/" + std::to_string(max));
    }

    void updateScore(int score) {
        if (!scoreText_) return;
        ui_->setElementText(*scoreText_, std::to_string(score));
    }

    void updateLives(int lives) {
        if (!livesText_) return;
        ui_->setElementText(*livesText_, std::to_string(lives));
    }

    void showActionPrompt(const std::string& text) {
        if (!actionText_ || !actionPrompt_) return;
        ui_->setElementText(*actionText_, text);
        ui_->addElementClass(*actionPrompt_, "visible");
    }

    void hideActionPrompt() {
        if (!actionPrompt_) return;
        ui_->removeElementClass(*actionPrompt_, "visible");
    }

private:
    IUISystem* ui_ = nullptr;
    UIDocumentHandle hudDoc_;
    std::optional<UIElementHandle> healthBar_;
    std::optional<UIElementHandle> healthText_;
    std::optional<UIElementHandle> scoreText_;
    std::optional<UIElementHandle> livesText_;
    std::optional<UIElementHandle> actionText_;
    std::optional<UIElementHandle> actionPrompt_;
};
```

### 3. Inventory Screen

**ui/inventory.rml:**
```xml
<rml>
<head>
    <link type="text/rcss" href="ui/inventory_styles.rcss"/>
    <title>Inventory</title>
</head>
<body>
    <div id="inventory-overlay">
        <div class="inventory-container">
            <div class="inventory-header">
                <h2>Inventory</h2>
                <button id="close-button" class="close-btn">×</button>
            </div>

            <div class="inventory-content">
                <!-- Item grid -->
                <div class="item-grid" id="item-grid">
                    <!-- Items dynamically created here -->
                </div>

                <!-- Item details panel -->
                <div class="item-details" id="item-details">
                    <img id="detail-icon" class="detail-icon" src=""/>
                    <h3 id="detail-name">Select an item</h3>
                    <p id="detail-description">Click on an item to view details.</p>
                    <div class="detail-actions">
                        <button id="btn-use" class="action-btn">Use</button>
                        <button id="btn-drop" class="action-btn">Drop</button>
                    </div>
                </div>
            </div>
        </div>
    </div>
</body>
</rml>
```

**ui/inventory_styles.rcss:**
```rcss
#inventory-overlay {
    position: absolute;
    width: 100%;
    height: 100%;
    background-color: rgba(0, 0, 0, 0.8);
    display: flex;
    align-items: center;
    justify-content: center;
}

.inventory-container {
    width: 80%;
    height: 70%;
    background-color: #2C2C2C;
    border: 3px solid #555;
    border-radius: 10px;
    padding: 20px;
    display: flex;
    flex-direction: column;
}

.inventory-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
    border-bottom: 2px solid #555;
    padding-bottom: 10px;
}

.inventory-header h2 {
    color: white;
    font-size: 24px;
    margin: 0;
}

.close-btn {
    width: 40px;
    height: 40px;
    background-color: #F44336;
    color: white;
    font-size: 28px;
    border: none;
    border-radius: 50%;
}

.close-btn:hover {
    background-color: #D32F2F;
}

.inventory-content {
    display: flex;
    gap: 20px;
    flex: 1;
    overflow: hidden;
}

.item-grid {
    flex: 2;
    display: grid;
    grid-template-columns: repeat(5, 1fr);
    gap: 10px;
    overflow-y: auto;
    padding: 10px;
}

.item-slot {
    aspect-ratio: 1;
    background-color: #444;
    border: 2px solid #666;
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: border-color 0.2s ease;
}

.item-slot:hover {
    border-color: #2196F3;
}

.item-slot.selected {
    border-color: #4CAF50;
}

.item-slot img {
    width: 80%;
    height: 80%;
    object-fit: contain;
}

.item-details {
    flex: 1;
    background-color: #333;
    border: 2px solid #555;
    border-radius: 8px;
    padding: 20px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 15px;
}

.detail-icon {
    width: 100px;
    height: 100px;
    object-fit: contain;
}

.detail-name {
    color: white;
    font-size: 20px;
    margin: 0;
}

.detail-description {
    color: #AAA;
    font-size: 14px;
    text-align: center;
    flex: 1;
}

.detail-actions {
    display: flex;
    gap: 10px;
    width: 100%;
}

.action-btn {
    flex: 1;
    height: 40px;
    background-color: #2196F3;
    color: white;
    border: none;
    border-radius: 5px;
    font-size: 16px;
}

.action-btn:hover {
    background-color: #1976D2;
}
```

**C++ Code:**
```cpp
struct InventoryItem {
    std::string id;
    std::string name;
    std::string description;
    std::string iconPath;
};

class InventoryUI {
public:
    void init(IUISystem* ui) {
        ui_ = ui;

        auto result = ui_->loadDocument("ui/inventory.rml");
        if (result) {
            invDoc_ = *result;

            // Get element handles
            itemGrid_ = ui_->getElementById(invDoc_, "item-grid");
            closeBtn_ = ui_->getElementById(invDoc_, "close-button");

            // Setup close button
            if (closeBtn_) {
                ui_->registerElementCallback(*closeBtn_, "click", [this](const UIEventData&) {
                    hide();
                });
            }
        }
    }

    void show(const std::vector<InventoryItem>& items) {
        if (!itemGrid_) return;

        // Clear existing items
        ui_->setInnerRml(*itemGrid_, "");

        // Create item slots
        for (const auto& item : items) {
            // Create item slot dynamically
            std::string slotHtml = R"(
                <div class="item-slot" id="item-)" + item.id + R"(">
                    <img src=")" + item.iconPath + R"("/>
                </div>
            )";

            // Append to grid (simplified - in reality you'd create elements properly)
            // This is just an example
        }

        ui_->showDocument(invDoc_);
    }

    void hide() {
        ui_->hideDocument(invDoc_);
    }

private:
    IUISystem* ui_ = nullptr;
    UIDocumentHandle invDoc_;
    std::optional<UIElementHandle> itemGrid_;
    std::optional<UIElementHandle> closeBtn_;
};
```

### 4. Pause Menu

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

        // Settings
        auto settingsBtn = ui_->getElementById(pauseDoc_, "btn-settings");
        if (settingsBtn) {
            ui_->registerElementCallback(*settingsBtn, "click", [this](const UIEventData&) {
                openSettings();
            });
        }

        // Main Menu
        auto mainMenuBtn = ui_->getElementById(pauseDoc_, "btn-main-menu");
        if (mainMenuBtn) {
            ui_->registerElementCallback(*mainMenuBtn, "click", [this](const UIEventData&) {
                returnToMainMenu();
            });
        }

        // Quit
        auto quitBtn = ui_->getElementById(pauseDoc_, "btn-quit");
        if (quitBtn) {
            ui_->registerElementCallback(*quitBtn, "click", [this](const UIEventData&) {
                quitGame();
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
    void openSettings() { /* ... */ }
    void returnToMainMenu() { /* ... */ }
    void quitGame() { /* ... */ }
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
      inventory.rml
    styles/
      common.rcss       # Shared styles
      menu_styles.rcss
      hud_styles.rcss
```

### 2. Responsive Design

Use percentages and flexbox for scalable UI:

```rcss
/* Flexible container */
.container {
    width: 80%;           /* Scale with window */
    max-width: 1200px;    /* Cap maximum size */
    margin: 0 auto;       /* Center */
}

/* Flexible grid */
.item-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(100px, 1fr));
    gap: 10px;
}

/* Relative sizes */
.button {
    font-size: 1.2em;     /* Relative to parent */
    padding: 0.5em 1em;   /* Scales with font size */
}
```

### 3. Performance Considerations

**DO:**
- Cache element handles instead of querying every frame
- Use `setInnerRml()` sparingly (expensive)
- Batch style updates when possible
- Hide documents instead of destroying/recreating
- Use `UIVisibility::Collapsed` for hidden elements that shouldn't take space

**DON'T:**
- Query elements by ID/class every frame
- Create/destroy elements in hot loops
- Bind too many variables (bind only what's actually displayed)
- Forget to unload unused documents

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
        ui->setElementText(*healthText_, std::to_string(health));
    }
}
```

### 4. Input Handling

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

// Check for input focus
void update() {
    if (ui->wantsKeyboardInput()) {
        // Text input is focused, disable game keyboard controls
        disableGameKeyboard();
    }

    if (ui->wantsMouseInput()) {
        // Mouse is over UI, don't shoot on click
        disableGameMouseActions();
    }
}
```

### 5. State Management

Organize UI into logical screens:

```cpp
class UIManager {
public:
    enum class Screen {
        None,
        MainMenu,
        GameHUD,
        PauseMenu,
        Inventory,
        Settings
    };

    void showScreen(Screen screen) {
        // Hide all screens
        hideAll();

        // Show requested screen
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
            // ...
        }

        currentScreen_ = screen;
    }

    void hideAll() {
        ui->hideDocument(mainMenuDoc_);
        ui->hideDocument(hudDoc_);
        ui->hideDocument(pauseDoc_);
        ui->hideDocument(inventoryDoc_);
        ui->hideDocument(settingsDoc_);
    }

private:
    Screen currentScreen_ = Screen::None;
    // Document handles...
};
```

### 6. Styling Best Practices

**Use CSS classes, not inline styles:**

```rcss
/* GOOD: Reusable classes */
.button-primary {
    background-color: #2196F3;
    color: white;
}

.button-danger {
    background-color: #F44336;
    color: white;
}
```

```xml
<!-- Use classes in RML -->
<button class="button-primary">OK</button>
<button class="button-danger">Delete</button>
```

**Organize with CSS variables (if supported):**

```rcss
/* Define color scheme */
:root {
    --color-primary: #2196F3;
    --color-secondary: #FFC107;
    --color-danger: #F44336;
    --spacing-small: 5px;
    --spacing-medium: 10px;
    --spacing-large: 20px;
}

/* Use throughout stylesheet */
.button {
    background-color: var(--color-primary);
    padding: var(--spacing-medium);
}
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
- Inspect element classes/IDs match selectors
- Enable debug mode to visualize elements

**3. Events not firing**
- Verify element handle is valid
- Check event type string is correct ("click", not "onclick")
- Ensure document is visible
- Check if UI consumed the input event

**4. Text not updating**
- Call `syncBindings()` after changing bound variables
- Verify element handle is still valid
- Check if element was destroyed/recreated

**5. Layout issues**
- Use flexbox for responsive layouts
- Check for conflicting `position` properties
- Verify parent container has size set
- Enable debug mode to see element bounds

---

## Additional Resources

### RmlUi Official Documentation
- [RmlUi Documentation](https://mikke89.github.io/RmlUiDoc/) - Official docs for RmlUi
- [RCSS Reference](https://mikke89.github.io/RmlUiDoc/pages/rcss.html) - Complete RCSS property reference
- [RML Element Index](https://mikke89.github.io/RmlUiDoc/pages/rml.html) - All supported HTML elements
- [Window Template Tutorial](https://mikke89.github.io/RmlUiDoc/pages/tutorials/window_template.html) - Step-by-step tutorial
- [Data Bindings Guide](https://mikke89.github.io/RmlUiDoc/pages/data_bindings/views_and_controllers.html) - Advanced data binding

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
| `"mousemove"` | Mouse moves within element |
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
| `transition` | `transition: all 0.3s;` | Animated changes |

---

## Conclusion

The Bestow UI System provides a powerful, web-style approach to game UI development. By combining RML documents for structure, RCSS stylesheets for presentation, and the C++ API for behavior, you can create professional, responsive user interfaces for your games.

Key takeaways:
- Structure your UI with RML (HTML-like)
- Style with RCSS (CSS-like)
- Handle events and updates in C++
- Use data binding for automatic synchronization
- Cache element handles for performance
- Always check if UI consumed input before passing to game

Happy UI building!
