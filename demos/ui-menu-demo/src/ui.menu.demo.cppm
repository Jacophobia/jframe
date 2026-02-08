// demos/ui-menu-demo/src/ui.menu.demo.cppm
// UI Menu Demo Application
//
// Demonstrates:
// - Vulkan UI render backend (VulkanUIRenderBackend)
// - RmlUi document loading from inline RML
// - Click-through menu with multiple screens
// - Button hover/active states with CSS styling
// - Input forwarding from GLFW to UI system
// - UI rendering as overlay on Vulkan

module;

#include <GLFW/glfw3.h>

export module ui.menu.demo;

import std;
import bestow.services;
import bestow.types;
import bestow.graphics3d;
import bestow.ui;
import bestow.ui.impl;

export namespace demo {

//==========================================================================
// Menu State
//==========================================================================

enum class MenuScreen {
    Main,
    Play,
    Settings,
    Credits,
    Quit
};

//==========================================================================
// UI Menu Demo Application
//==========================================================================

class UIMenuDemo : public bestow::Application<UIMenuDemo,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem,
    bestow::IAssetSystem>
{
public:
    UIMenuDemo(bestow::IGraphics3DSystem& graphics,
               bestow::IInputSystem& input,
               bestow::IAssetSystem& assets)
        : graphics_(&graphics)
        , input_(&input)
        , assets_(&assets) {}

    ~UIMenuDemo() override = default;

    void run() override {
        if (!initialize()) {
            return;
        }

        gameLoop();
        cleanup();
    }

    void shutdown() override {
        running_ = false;
    }

private:
    //======================================================================
    // Dependencies
    //======================================================================
    bestow::IGraphics3DSystem* graphics_ = nullptr;
    bestow::IInputSystem* input_ = nullptr;
    bestow::IAssetSystem* assets_ = nullptr;

    //======================================================================
    // UI System (created manually - not through DI)
    //======================================================================
    std::unique_ptr<bestow::RmlUISystem> ui_;

    //======================================================================
    // State
    //======================================================================
    bool running_ = false;
    MenuScreen currentScreen_ = MenuScreen::Main;

    // Document handles for each screen
    bestow::UIDocumentHandle mainMenuDoc_{};
    bestow::UIDocumentHandle playDoc_{};
    bestow::UIDocumentHandle settingsDoc_{};
    bestow::UIDocumentHandle creditsDoc_{};
    bestow::UIDocumentHandle quitDoc_{};

    // GLFW callback state
    GLFWwindow* window_ = nullptr;

    //======================================================================
    // Initialization
    //======================================================================

    bool initialize() {
        // Initialize graphics
        bestow::Graphics3DConfig gfxConfig{
            .windowWidth = 1280,
            .windowHeight = 720,
            .windowTitle = "UI Menu Demo - Bestow",
            .vsync = true,
            .fullscreen = false
        };

        if (!graphics_->initialize(gfxConfig)) {
            std::cerr << "Failed to initialize graphics\n";
            return false;
        }

        graphics_->setClearColor(bestow::Color{25, 25, 35, 255});

        // Get window handle for input
        window_ = static_cast<GLFWwindow*>(graphics_->getNativeWindowHandle());

        // Initialize input
        if (input_) {
            input_->initialize(window_);
        }

        // Create UI system manually (IGraphics3DSystem extends IGraphicsContext)
        ui_ = std::make_unique<bestow::RmlUISystem>(
            static_cast<bestow::IGraphicsContext&>(*graphics_), *assets_);

        bestow::UIConfig uiConfig{};
        auto result = ui_->initialize(uiConfig);
        if (!result) {
            std::cerr << "Failed to initialize UI system\n";
            return false;
        }

        // Set up GLFW input callbacks for UI
        setupInputCallbacks();

        // Load fonts before creating documents
        auto fontResult = ui_->loadFont(":library:/fonts/Press_Start_2P/PressStart2P-Regular.ttf", "PressStart2P");
        if (!fontResult) {
            std::cerr << "Warning: Failed to load PressStart2P font\n";
        }

        auto fontResult2 = ui_->loadFont(":library:/fonts/Orbitron/static/Orbitron-Regular.ttf", "Orbitron");
        if (!fontResult2) {
            std::cerr << "Warning: Failed to load Orbitron font\n";
        }

        // Create all menu screens
        if (!createMenuScreens()) {
            std::cerr << "Failed to create menu screens\n";
            return false;
        }

        // Show the main menu
        showScreen(MenuScreen::Main);

        running_ = true;
        std::cout << "UI Menu Demo initialized successfully!\n";
        std::cout << "Click buttons to navigate between screens.\n";
        std::cout << "Press ESC to quit.\n";
        return true;
    }

    //======================================================================
    // Menu Screen Creation
    //======================================================================

    bool createMenuScreens() {
        // Main Menu
        auto mainResult = ui_->loadDocumentFromString(getMainMenuRml(), "main-menu");
        if (!mainResult) {
            std::cerr << "Failed to load main menu document\n";
            return false;
        }
        mainMenuDoc_ = *mainResult;
        registerMainMenuCallbacks();

        // Play Screen
        auto playResult = ui_->loadDocumentFromString(getPlayScreenRml(), "play-screen");
        if (!playResult) return false;
        playDoc_ = *playResult;
        registerPlayScreenCallbacks();

        // Settings Screen
        auto settingsResult = ui_->loadDocumentFromString(getSettingsScreenRml(), "settings-screen");
        if (!settingsResult) return false;
        settingsDoc_ = *settingsResult;
        registerSettingsScreenCallbacks();

        // Credits Screen
        auto creditsResult = ui_->loadDocumentFromString(getCreditsScreenRml(), "credits-screen");
        if (!creditsResult) return false;
        creditsDoc_ = *creditsResult;
        registerCreditsScreenCallbacks();

        // Quit Confirmation Screen
        auto quitResult = ui_->loadDocumentFromString(getQuitScreenRml(), "quit-screen");
        if (!quitResult) return false;
        quitDoc_ = *quitResult;
        registerQuitScreenCallbacks();

        return true;
    }

    void showScreen(MenuScreen screen) {
        // Hide all
        ui_->hideDocument(mainMenuDoc_);
        ui_->hideDocument(playDoc_);
        ui_->hideDocument(settingsDoc_);
        ui_->hideDocument(creditsDoc_);
        ui_->hideDocument(quitDoc_);

        // Show the target
        currentScreen_ = screen;
        switch (screen) {
            case MenuScreen::Main:
                ui_->showDocument(mainMenuDoc_);
                std::cout << "[Menu] Main Menu\n";
                break;
            case MenuScreen::Play:
                ui_->showDocument(playDoc_);
                std::cout << "[Menu] Play Screen\n";
                break;
            case MenuScreen::Settings:
                ui_->showDocument(settingsDoc_);
                std::cout << "[Menu] Settings Screen\n";
                break;
            case MenuScreen::Credits:
                ui_->showDocument(creditsDoc_);
                std::cout << "[Menu] Credits Screen\n";
                break;
            case MenuScreen::Quit:
                ui_->showDocument(quitDoc_);
                std::cout << "[Menu] Quit Confirmation\n";
                break;
        }
    }

    //======================================================================
    // RML Document Definitions
    //======================================================================

    static std::string getCommonStyles() {
        return R"(
        body {
            font-family: PressStart2P;
            font-size: 16dp;
            width: 1280dp;
            height: 720dp;
        }
        .panel {
            width: 400dp;
            height: auto;
            margin: 150dp auto 0dp auto;
            padding: 30dp 40dp;
            background-color: #1a1a2e;
            border-width: 2dp;
            border-color: #4a4a7a;
            border-radius: 8dp;
            text-align: center;
        }
        h1 {
            color: #e0e0ff;
            font-size: 28dp;
            margin-bottom: 20dp;
        }
        h2 {
            color: #c0c0e0;
            font-size: 22dp;
            margin-bottom: 15dp;
        }
        p {
            color: #a0a0c0;
            font-size: 14dp;
            margin-bottom: 10dp;
        }
        .btn {
            display: block;
            width: 240dp;
            height: 44dp;
            margin: 8dp auto;
            padding: 10dp 0dp;
            background-color: #2a4a8a;
            color: #e0e0ff;
            font-size: 16dp;
            text-align: center;
            border-width: 1dp;
            border-color: #4a6aaa;
            border-radius: 4dp;
        }
        .btn:hover {
            background-color: #3a6aba;
            border-color: #6a8aca;
            color: #ffffff;
        }
        .btn:active {
            background-color: #1a3a6a;
            border-color: #3a5a8a;
        }
        .btn-danger {
            background-color: #8a2a2a;
            border-color: #aa4a4a;
        }
        .btn-danger:hover {
            background-color: #aa3a3a;
            border-color: #ca5a5a;
        }
        .btn-danger:active {
            background-color: #6a1a1a;
        }
        .btn-back {
            background-color: #3a3a5a;
            border-color: #5a5a7a;
            margin-top: 20dp;
        }
        .btn-back:hover {
            background-color: #4a4a6a;
            border-color: #6a6a8a;
        }
        .subtitle {
            color: #8080b0;
            font-size: 12dp;
            margin-bottom: 25dp;
        }
        .setting-row {
            display: block;
            width: 100%;
            margin: 8dp 0dp;
            padding: 8dp 10dp;
            background-color: #222244;
            border-radius: 4dp;
            text-align: left;
        }
        .setting-label {
            color: #c0c0e0;
            font-size: 14dp;
        }
        .setting-value {
            color: #80b0ff;
            font-size: 14dp;
        }
        .credit-section {
            margin: 10dp 0dp;
        }
        .credit-role {
            color: #8080b0;
            font-size: 12dp;
        }
        .credit-name {
            color: #e0e0ff;
            font-size: 16dp;
        }
        )";
    }

    static std::string getMainMenuRml() {
        return "<rml><head><style>" + getCommonStyles() + "</style></head><body>"
            "<div class=\"panel\">"
            "<h1>Bestow Engine</h1>"
            "<p class=\"subtitle\">UI Menu Demo</p>"
            "<div id=\"btn-play\" class=\"btn\">Play Game</div>"
            "<div id=\"btn-settings\" class=\"btn\">Settings</div>"
            "<div id=\"btn-credits\" class=\"btn\">Credits</div>"
            "<div id=\"btn-quit\" class=\"btn btn-danger\">Quit</div>"
            "</div>"
            "</body></rml>";
    }

    static std::string getPlayScreenRml() {
        return "<rml><head><style>" + getCommonStyles() + "</style></head><body>"
            "<div class=\"panel\">"
            "<h1>Play Game</h1>"
            "<div id=\"btn-new\" class=\"btn\">New Game</div>"
            "<div id=\"btn-continue\" class=\"btn\">Continue</div>"
            "<div id=\"btn-load\" class=\"btn\">Load Save</div>"
            "<div id=\"btn-back\" class=\"btn btn-back\">Back</div>"
            "</div>"
            "</body></rml>";
    }

    static std::string getSettingsScreenRml() {
        return "<rml><head><style>" + getCommonStyles() + "</style></head><body>"
            "<div class=\"panel\">"
            "<h1>Settings</h1>"
            "<div class=\"setting-row\">"
            "<span class=\"setting-label\">Resolution: </span>"
            "<span class=\"setting-value\">1280x720</span>"
            "</div>"
            "<div class=\"setting-row\">"
            "<span class=\"setting-label\">VSync: </span>"
            "<span class=\"setting-value\">On</span>"
            "</div>"
            "<div class=\"setting-row\">"
            "<span class=\"setting-label\">Fullscreen: </span>"
            "<span class=\"setting-value\">Off</span>"
            "</div>"
            "<div class=\"setting-row\">"
            "<span class=\"setting-label\">Volume: </span>"
            "<span class=\"setting-value\">80%</span>"
            "</div>"
            "<div id=\"btn-back\" class=\"btn btn-back\">Back</div>"
            "</div>"
            "</body></rml>";
    }

    static std::string getCreditsScreenRml() {
        return "<rml><head><style>" + getCommonStyles() + "</style></head><body>"
            "<div class=\"panel\">"
            "<h1>Credits</h1>"
            "<div class=\"credit-section\">"
            "<p class=\"credit-role\">Engine</p>"
            "<p class=\"credit-name\">Bestow Game Framework</p>"
            "</div>"
            "<div class=\"credit-section\">"
            "<p class=\"credit-role\">Rendering</p>"
            "<p class=\"credit-name\">Vulkan API</p>"
            "</div>"
            "<div class=\"credit-section\">"
            "<p class=\"credit-role\">UI System</p>"
            "<p class=\"credit-name\">RmlUi</p>"
            "</div>"
            "<div class=\"credit-section\">"
            "<p class=\"credit-role\">Shader Compilation</p>"
            "<p class=\"credit-name\">shaderc (runtime GLSL to SPIR-V)</p>"
            "</div>"
            "<div id=\"btn-back\" class=\"btn btn-back\">Back</div>"
            "</div>"
            "</body></rml>";
    }

    static std::string getQuitScreenRml() {
        return "<rml><head><style>" + getCommonStyles() + "</style></head><body>"
            "<div class=\"panel\">"
            "<h2>Are you sure you want to quit?</h2>"
            "<div id=\"btn-yes\" class=\"btn btn-danger\">Yes, Quit</div>"
            "<div id=\"btn-no\" class=\"btn\">No, Go Back</div>"
            "</div>"
            "</body></rml>";
    }

    //======================================================================
    // Button Callbacks
    //======================================================================

    void registerMainMenuCallbacks() {
        auto play = ui_->getElementById(mainMenuDoc_, "btn-play");
        if (play) {
            ui_->registerElementCallback(*play, "click",
                [this](const bestow::UIEventData&) { showScreen(MenuScreen::Play); });
        }

        auto settings = ui_->getElementById(mainMenuDoc_, "btn-settings");
        if (settings) {
            ui_->registerElementCallback(*settings, "click",
                [this](const bestow::UIEventData&) { showScreen(MenuScreen::Settings); });
        }

        auto credits = ui_->getElementById(mainMenuDoc_, "btn-credits");
        if (credits) {
            ui_->registerElementCallback(*credits, "click",
                [this](const bestow::UIEventData&) { showScreen(MenuScreen::Credits); });
        }

        auto quit = ui_->getElementById(mainMenuDoc_, "btn-quit");
        if (quit) {
            ui_->registerElementCallback(*quit, "click",
                [this](const bestow::UIEventData&) { showScreen(MenuScreen::Quit); });
        }
    }

    void registerPlayScreenCallbacks() {
        auto newGame = ui_->getElementById(playDoc_, "btn-new");
        if (newGame) {
            ui_->registerElementCallback(*newGame, "click",
                [this](const bestow::UIEventData&) {
                    std::cout << "[Action] New Game clicked!\n";
                });
        }

        auto continueGame = ui_->getElementById(playDoc_, "btn-continue");
        if (continueGame) {
            ui_->registerElementCallback(*continueGame, "click",
                [this](const bestow::UIEventData&) {
                    std::cout << "[Action] Continue clicked!\n";
                });
        }

        auto loadSave = ui_->getElementById(playDoc_, "btn-load");
        if (loadSave) {
            ui_->registerElementCallback(*loadSave, "click",
                [this](const bestow::UIEventData&) {
                    std::cout << "[Action] Load Save clicked!\n";
                });
        }

        auto back = ui_->getElementById(playDoc_, "btn-back");
        if (back) {
            ui_->registerElementCallback(*back, "click",
                [this](const bestow::UIEventData&) { showScreen(MenuScreen::Main); });
        }
    }

    void registerSettingsScreenCallbacks() {
        auto back = ui_->getElementById(settingsDoc_, "btn-back");
        if (back) {
            ui_->registerElementCallback(*back, "click",
                [this](const bestow::UIEventData&) { showScreen(MenuScreen::Main); });
        }
    }

    void registerCreditsScreenCallbacks() {
        auto back = ui_->getElementById(creditsDoc_, "btn-back");
        if (back) {
            ui_->registerElementCallback(*back, "click",
                [this](const bestow::UIEventData&) { showScreen(MenuScreen::Main); });
        }
    }

    void registerQuitScreenCallbacks() {
        auto yes = ui_->getElementById(quitDoc_, "btn-yes");
        if (yes) {
            ui_->registerElementCallback(*yes, "click",
                [this](const bestow::UIEventData&) { running_ = false; });
        }

        auto no = ui_->getElementById(quitDoc_, "btn-no");
        if (no) {
            ui_->registerElementCallback(*no, "click",
                [this](const bestow::UIEventData&) { showScreen(MenuScreen::Main); });
        }
    }

    //======================================================================
    // Input Handling
    //======================================================================

    void setupInputCallbacks() {
        // Store this pointer for GLFW callbacks
        glfwSetWindowUserPointer(window_, this);

        glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double x, double y) {
            auto* self = static_cast<UIMenuDemo*>(glfwGetWindowUserPointer(w));
            if (self->ui_) {
                bestow::UIInputEvent evt{
                    .type = bestow::UIInputType::MouseMove,
                    .x = static_cast<int>(x),
                    .y = static_cast<int>(y)
                };
                self->ui_->processInput(evt);
            }
        });

        glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int) {
            auto* self = static_cast<UIMenuDemo*>(glfwGetWindowUserPointer(w));
            if (self->ui_) {
                bestow::UIInputEvent evt{
                    .type = action == GLFW_PRESS ? bestow::UIInputType::MouseDown
                                                 : bestow::UIInputType::MouseUp,
                    .button = button
                };
                double mx, my;
                glfwGetCursorPos(w, &mx, &my);
                evt.x = static_cast<int>(mx);
                evt.y = static_cast<int>(my);
                self->ui_->processInput(evt);
            }
        });

        glfwSetScrollCallback(window_, [](GLFWwindow* w, double, double yOffset) {
            auto* self = static_cast<UIMenuDemo*>(glfwGetWindowUserPointer(w));
            if (self->ui_) {
                bestow::UIInputEvent evt{
                    .type = bestow::UIInputType::MouseScroll,
                    .wheelDelta = static_cast<int>(yOffset * 3)
                };
                self->ui_->processInput(evt);
            }
        });
    }

    //======================================================================
    // Game Loop
    //======================================================================

    void gameLoop() {
        auto previousTime = std::chrono::high_resolution_clock::now();

        while (running_ && !graphics_->shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            bestow::DeltaTime dt =
                std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;

            if (dt > 0.25f) dt = 0.25f;

            // Poll events
            glfwPollEvents();

            // Handle ESC key
            if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                if (currentScreen_ == MenuScreen::Main) {
                    running_ = false;
                } else {
                    showScreen(MenuScreen::Main);
                    // Wait for key release to avoid repeated triggers
                    while (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                        glfwPollEvents();
                    }
                }
            }

            // Update UI
            if (ui_) {
                ui_->update(dt);
            }

            // Render
            graphics_->beginFrame();

            if (ui_) {
                ui_->render();
            }

            graphics_->endFrame();

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    //======================================================================
    // Cleanup
    //======================================================================

    void cleanup() {
        if (ui_) {
            ui_->unloadDocument(mainMenuDoc_);
            ui_->unloadDocument(playDoc_);
            ui_->unloadDocument(settingsDoc_);
            ui_->unloadDocument(creditsDoc_);
            ui_->unloadDocument(quitDoc_);
            ui_->shutdown();
            ui_.reset();
        }

        if (input_) {
            input_->shutdown();
        }

        if (graphics_) {
            graphics_->shutdown();
        }
    }
};

}  // namespace demo
