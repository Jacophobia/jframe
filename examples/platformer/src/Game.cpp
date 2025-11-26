// examples/platformer/src/Game.cpp
// Platformer game implementation

import std;
import jframe;
import jframe.core;

#if defined(JFRAME_DEV_TOOLS)
import jframe.dev;
#endif

namespace platformer {

class Game : public jframe::core::Application {
public:
    Game() = default;
    ~Game() override = default;

protected:
    bool initialize() override {
        jframe::core::logInfo("Initializing Platformer Game");

        // Initialize systems
        // Register input mappings
        // Load initial level

        #if defined(JFRAME_DEV_TOOLS)
        hotReload_.watchDirectory("data/");

        hotReload_.onLevelChanged = [this](const auto& path) {
            jframe::core::logInfo("Level changed: " + path.string());
            // Reload level
        };

        hotReload_.onBlueprintChanged = [this](const auto& path) {
            jframe::core::logInfo("Blueprint changed: " + path.string());
            // Update blueprint registry
        };
        #endif

        return true;
    }

    void update(jframe::DeltaTime dt) override {
        #if defined(JFRAME_DEV_TOOLS)
        hotReload_.update();
        devOverlay_.setFPS(1.0f / dt);
        #endif

        // Update game systems
        // Process physics
        // Handle input
    }

    void render() override {
        // Render game world
        // Render UI

        #if defined(JFRAME_DEV_TOOLS)
        devOverlay_.render();
        entityInspector_->render();
        #endif
    }

    void shutdown() override {
        jframe::core::logInfo("Shutting down Platformer Game");
    }

private:
    #if defined(JFRAME_DEV_TOOLS)
    jframe::dev::HotReloadManager hotReload_;
    jframe::dev::DevOverlay devOverlay_;
    std::unique_ptr<jframe::dev::EntityInspector> entityInspector_;
    #endif
};

}  // namespace platformer
