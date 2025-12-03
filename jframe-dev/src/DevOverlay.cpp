// jframe-dev/src/DevOverlay.cpp
// Debug overlay implementation using ImGui

module;

#include <algorithm>
#include <array>
#include <chrono>
#include <compare>
#include <cstddef>
#include <string>

#include <imgui.h>

module jframe.dev;

namespace jframe::dev {

// Frame history for graphs
constexpr std::size_t FRAME_HISTORY_SIZE = 120;

struct DevOverlayData {
    std::array<float, FRAME_HISTORY_SIZE> fpsHistory{};
    std::array<float, FRAME_HISTORY_SIZE> frameTimeHistory{};
    std::size_t historyIndex = 0;
    float minFPS = 999.0f;
    float maxFPS = 0.0f;
    float avgFPS = 0.0f;
    float minFrameTime = 999.0f;
    float maxFrameTime = 0.0f;
    std::size_t frameCount = 0;
};

static DevOverlayData g_data;

void DevOverlay::update(DeltaTime dt) {
    // Update FPS history
    if (fps_ > 0.0f) {
        g_data.fpsHistory[g_data.historyIndex] = fps_;
        g_data.frameTimeHistory[g_data.historyIndex] = dt * 1000.0f;  // Convert to ms
        g_data.historyIndex = (g_data.historyIndex + 1) % FRAME_HISTORY_SIZE;
        g_data.frameCount++;

        // Update statistics
        g_data.minFPS = std::min(g_data.minFPS, fps_);
        g_data.maxFPS = std::max(g_data.maxFPS, fps_);

        float frameTimeMs = dt * 1000.0f;
        g_data.minFrameTime = std::min(g_data.minFrameTime, frameTimeMs);
        g_data.maxFrameTime = std::max(g_data.maxFrameTime, frameTimeMs);

        // Calculate average FPS (over last 60 frames)
        float sum = 0.0f;
        std::size_t count = std::min(g_data.frameCount, FRAME_HISTORY_SIZE);
        for (std::size_t i = 0; i < count; i++) {
            sum += g_data.fpsHistory[i];
        }
        g_data.avgFPS = count > 0 ? sum / count : 0.0f;
    }
}

void DevOverlay::render() {
    if (!visible_) return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);

    ImGui::Begin("JFrame Dev Tools", &visible_);

    // FPS Section
    ImGui::SeparatorText("Performance");
    ImGui::Text("FPS: %.1f (avg: %.1f)", fps_, g_data.avgFPS);
    ImGui::Text("Range: %.1f - %.1f", g_data.minFPS, g_data.maxFPS);

    // FPS Graph
    ImGui::PlotLines("##FPS", g_data.fpsHistory.data(), FRAME_HISTORY_SIZE,
                     g_data.historyIndex, "FPS History",
                     0.0f, g_data.maxFPS * 1.2f, ImVec2(0, 80));

    // Frame Time Section
    ImGui::Spacing();
    float currentFrameTime = fps_ > 0.0f ? 1000.0f / fps_ : 0.0f;
    ImGui::Text("Frame Time: %.2f ms", currentFrameTime);
    ImGui::Text("Range: %.2f - %.2f ms", g_data.minFrameTime, g_data.maxFrameTime);

    // Frame Time Graph
    ImGui::PlotLines("##FrameTime", g_data.frameTimeHistory.data(), FRAME_HISTORY_SIZE,
                     g_data.historyIndex, "Frame Time (ms)",
                     0.0f, g_data.maxFrameTime * 1.2f, ImVec2(0, 80));

    // Entity Count
    ImGui::Spacing();
    ImGui::SeparatorText("Systems");
    ImGui::Text("Entities: %zu", entityCount_);

    // Hot Reload Status
    if (!lastReloadFile_.empty()) {
        ImGui::Spacing();
        ImGui::SeparatorText("Hot Reload");
        auto elapsed = std::chrono::steady_clock::now() - lastReloadTime_;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        ImGui::Text("Last: %s", lastReloadFile_.c_str());
        ImGui::Text("Time: %llds ago", seconds);
    }

    // Reset Stats Button
    ImGui::Spacing();
    if (ImGui::Button("Reset Stats")) {
        g_data.minFPS = fps_;
        g_data.maxFPS = fps_;
        g_data.minFrameTime = currentFrameTime;
        g_data.maxFrameTime = currentFrameTime;
    }

    ImGui::End();
}

void DevOverlay::setLastReload(const std::string& file) {
    lastReloadFile_ = file;
    lastReloadTime_ = std::chrono::steady_clock::now();
}

}  // namespace jframe::dev
