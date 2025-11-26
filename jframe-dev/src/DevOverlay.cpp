// jframe-dev/src/DevOverlay.cpp
// Debug overlay implementation using ImGui

module;

#include <chrono>
#include <cstddef>
#include <string>

#include <imgui.h>

module jframe.dev;

namespace jframe::dev {

void DevOverlay::update(DeltaTime dt) {
    // Update overlay state
}

void DevOverlay::render() {
    if (!visible_) return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);

    ImGui::Begin("JFrame Dev", &visible_, ImGuiWindowFlags_NoResize);

    ImGui::Text("FPS: %.1f", fps_);
    ImGui::Text("Entities: %zu", entityCount_);

    if (!lastReloadFile_.empty()) {
        auto elapsed = std::chrono::steady_clock::now() - lastReloadTime_;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        ImGui::Text("Last Reload: %s (%llds ago)", lastReloadFile_.c_str(), seconds);
    }

    ImGui::End();
}

void DevOverlay::setLastReload(const std::string& file) {
    lastReloadFile_ = file;
    lastReloadTime_ = std::chrono::steady_clock::now();
}

}  // namespace jframe::dev
