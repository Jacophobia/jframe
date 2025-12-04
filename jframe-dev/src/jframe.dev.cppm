// jframe-dev/src/jframe.dev.cppm
// Development tools module

module;

#include <imgui.h>

// MSVC C++23 module compatibility - include full EnTT before import std
#include <jframe/entt_compat.hpp>

// Include GLFW header for GLFWwindow - forward declaration causes type
// mismatch with MSVC C++23 modules when implementation includes full header
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>

export module jframe.dev;

import std;
import jframe;
import jframe.core;

export namespace jframe::dev {

//==========================================================================
// ImGui Backend Functions
//==========================================================================

void initializeImGui(GLFWwindow* window);
void beginImGuiFrame();
void renderImGui();
void shutdownImGui();

//==========================================================================
// File Change Event
//==========================================================================

struct FileChange {
    std::filesystem::path path;
    enum class Action { Added, Modified, Deleted } action;
};

//==========================================================================
// Hot Reload Manager
//==========================================================================

class HotReloadManager {
public:
    HotReloadManager();
    ~HotReloadManager();

    void watchDirectory(const std::filesystem::path& dir);
    void stopWatching();
    void update();

    std::function<void(const std::filesystem::path&)> onBlueprintChanged;
    std::function<void(const std::filesystem::path&)> onLevelChanged;
    std::function<void(const std::filesystem::path&)> onTextureChanged;
    std::function<void(const std::filesystem::path&)> onAudioChanged;
    std::function<void(const std::filesystem::path&)> onConfigChanged;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

//==========================================================================
// Dev Overlay
//==========================================================================

class DevOverlay {
public:
    DevOverlay() = default;
    ~DevOverlay() = default;

    void update(DeltaTime dt);
    void render();

    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    void toggle() { visible_ = !visible_; }

    void setFPS(float fps) { fps_ = fps; }
    void setEntityCount(std::size_t count) { entityCount_ = count; }
    void setLastReload(const std::string& file);

private:
    bool visible_ = true;
    float fps_ = 0.0f;
    std::size_t entityCount_ = 0;
    std::string lastReloadFile_;
    std::chrono::steady_clock::time_point lastReloadTime_;
};

//==========================================================================
// Entity Inspector
//==========================================================================

class EntityInspector {
public:
    EntityInspector(JFrameEngine& engine);
    ~EntityInspector() = default;

    void update();
    void render();

    void selectEntity(Entity entity);
    void clearSelection();
    std::optional<Entity> getSelectedEntity() const;

private:
    void handleEntitySelection();
    void renderInspectorWindow();
    std::string serializeEntityToLua(Entity entity);

    JFrameEngine& engine_;
    std::optional<Entity> selectedEntity_;
};

}  // namespace jframe::dev
