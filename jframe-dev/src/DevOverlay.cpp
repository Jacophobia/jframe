// jframe-dev/src/DevOverlay.cpp
// Debug overlay implementation using ImGui

module;

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <string>

#include <imgui.h>
#include <spdlog/spdlog.h>

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

    // Tab state
    int selectedTab = 0;

    // System colors for the bar chart
    static constexpr std::array<ImU32, 17> systemColors = {
        IM_COL32(255, 99, 132, 255),   // Core - Red
        IM_COL32(255, 159, 64, 255),   // Events - Orange
        IM_COL32(255, 205, 86, 255),   // Entity - Yellow
        IM_COL32(75, 192, 192, 255),   // Graphics - Teal
        IM_COL32(54, 162, 235, 255),   // Audio - Blue
        IM_COL32(153, 102, 255, 255),  // Input - Purple
        IM_COL32(201, 203, 207, 255),  // Physics - Gray
        IM_COL32(255, 99, 71, 255),    // Assets - Tomato
        IM_COL32(50, 205, 50, 255),    // Save - LimeGreen
        IM_COL32(255, 215, 0, 255),    // Level - Gold
        IM_COL32(0, 191, 255, 255),    // AI - DeepSkyBlue
        IM_COL32(255, 105, 180, 255),  // Camera - HotPink
        IM_COL32(144, 238, 144, 255),  // Config - LightGreen
        IM_COL32(186, 85, 211, 255),   // GAS - MediumOrchid
        IM_COL32(100, 149, 237, 255),  // Blueprints - CornflowerBlue
        IM_COL32(220, 20, 60, 255),    // DevTools - Crimson
        IM_COL32(46, 139, 87, 255)     // Application - SeaGreen
    };
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
    ImGui::SetNextWindowSize(ImVec2(450, 550), ImGuiCond_FirstUseEver);

    ImGui::Begin("JFrame Dev Tools", &visible_);

    // Tab bar for different views
    if (ImGui::BeginTabBar("DevToolsTabs")) {
        // Performance Tab
        if (ImGui::BeginTabItem("Performance")) {
            renderPerformanceTab();
            ImGui::EndTabItem();
        }

        // Systems Tab
        if (ImGui::BeginTabItem("Systems")) {
            renderSystemsTab();
            ImGui::EndTabItem();
        }

        // Code Paths Tab
        if (ImGui::BeginTabItem("Code Paths")) {
            renderCodePathsTab();
            ImGui::EndTabItem();
        }

        // Export Tab
        if (ImGui::BeginTabItem("Export")) {
            renderExportTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void DevOverlay::renderPerformanceTab() {
    // FPS Section
    ImGui::SeparatorText("Frame Rate");
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
    ImGui::SeparatorText("Game State");
    ImGui::Text("Entities: %zu", entityCount_);
    ImGui::Text("Frame: %llu", metrics::MetricsCollector::instance().frameCount());

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
        metrics::MetricsCollector::instance().reset();
    }
}

void DevOverlay::renderSystemsTab() {
    const auto& collector = metrics::MetricsCollector::instance();

    ImGui::SeparatorText("System Time Breakdown");

    // Calculate total time for percentage
    double totalTime = 0.0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(metrics::SystemId::Count); ++i) {
        totalTime += collector.getSystemTiming(static_cast<metrics::SystemId>(i)).lastFrameTimeMs;
    }

    // Horizontal stacked bar showing frame breakdown
    ImGui::Text("Frame Breakdown:");
    ImVec2 barStart = ImGui::GetCursorScreenPos();
    float barWidth = ImGui::GetContentRegionAvail().x;
    float barHeight = 25.0f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float xOffset = 0.0f;
    for (std::size_t i = 0; i < static_cast<std::size_t>(metrics::SystemId::Count); ++i) {
        const auto& timing = collector.getSystemTiming(static_cast<metrics::SystemId>(i));
        if (timing.lastFrameTimeMs > 0.001 && totalTime > 0.0) {
            float width = static_cast<float>(timing.lastFrameTimeMs / totalTime) * barWidth;
            if (width >= 1.0f) {
                drawList->AddRectFilled(
                    ImVec2(barStart.x + xOffset, barStart.y),
                    ImVec2(barStart.x + xOffset + width, barStart.y + barHeight),
                    g_data.systemColors[i]
                );
                xOffset += width;
            }
        }
    }

    // Draw border
    drawList->AddRect(barStart, ImVec2(barStart.x + barWidth, barStart.y + barHeight),
                      IM_COL32(255, 255, 255, 128));

    ImGui::Dummy(ImVec2(0, barHeight + 5));

    // Legend and detailed stats
    ImGui::Spacing();
    ImGui::SeparatorText("Per-System Metrics");

    ImGui::BeginChild("SystemsList", ImVec2(0, 250), true);

    // Table header
    ImGui::Columns(5, "SystemsTable");
    ImGui::SetColumnWidth(0, 100);
    ImGui::SetColumnWidth(1, 70);
    ImGui::SetColumnWidth(2, 70);
    ImGui::SetColumnWidth(3, 70);
    ImGui::SetColumnWidth(4, 70);

    ImGui::Text("System"); ImGui::NextColumn();
    ImGui::Text("Last"); ImGui::NextColumn();
    ImGui::Text("Avg"); ImGui::NextColumn();
    ImGui::Text("Max"); ImGui::NextColumn();
    ImGui::Text("Calls"); ImGui::NextColumn();
    ImGui::Separator();

    for (std::size_t i = 0; i < static_cast<std::size_t>(metrics::SystemId::Count); ++i) {
        const auto& timing = collector.getSystemTiming(static_cast<metrics::SystemId>(i));
        if (timing.callCount > 0) {
            // Color indicator
            ImVec2 colorBoxPos = ImGui::GetCursorScreenPos();
            drawList->AddRectFilled(
                colorBoxPos,
                ImVec2(colorBoxPos.x + 10, colorBoxPos.y + 14),
                g_data.systemColors[i]
            );
            ImGui::Dummy(ImVec2(14, 0));
            ImGui::SameLine();

            ImGui::Text("%s", std::string(metrics::systemIdToString(
                static_cast<metrics::SystemId>(i))).c_str());
            ImGui::NextColumn();

            ImGui::Text("%.2fms", timing.lastFrameTimeMs);
            ImGui::NextColumn();

            ImGui::Text("%.2fms", timing.avgTimeMs);
            ImGui::NextColumn();

            ImGui::Text("%.2fms", timing.maxTimeMs);
            ImGui::NextColumn();

            ImGui::Text("%llu", timing.callCount);
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    // System history graphs
    ImGui::Spacing();
    static int selectedSystem = static_cast<int>(metrics::SystemId::Physics);
    ImGui::Text("System History:");
    ImGui::SameLine();

    // Dropdown for system selection
    const char* systemNames[] = {
        "Core", "Events", "Entity", "Graphics", "Audio", "Input",
        "Physics", "Assets", "Save", "Level", "AI", "Camera",
        "Config", "GAS", "Blueprints", "DevTools", "Application"
    };
    ImGui::SetNextItemWidth(150);
    ImGui::Combo("##SystemSelect", &selectedSystem, systemNames,
                 static_cast<int>(metrics::SystemId::Count));

    const auto& selectedTiming = collector.getSystemTiming(
        static_cast<metrics::SystemId>(selectedSystem));
    ImGui::PlotLines("##SystemHistory", selectedTiming.history.data(),
                     metrics::SystemTimingData::kHistorySize,
                     selectedTiming.historyIndex,
                     systemNames[selectedSystem],
                     0.0f, static_cast<float>(selectedTiming.maxTimeMs * 1.2),
                     ImVec2(0, 60));
}

void DevOverlay::renderCodePathsTab() {
    const auto& collector = metrics::MetricsCollector::instance();
    const auto& codePaths = collector.getCodePaths();

    ImGui::SeparatorText("Tracked Code Paths");
    ImGui::Text("Total paths tracked: %zu", codePaths.size());

    ImGui::Spacing();

    if (codePaths.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
            "No code paths tracked yet.\n\n"
            "Use metrics::MetricsCollector::instance().trackCodePath(\"name\")\n"
            "or metrics::ScopedCodePath tracker(\"name\") to track code paths.");
    } else {
        ImGui::BeginChild("CodePathsList", ImVec2(0, 350), true);

        ImGui::Columns(3, "CodePathsTable");
        ImGui::SetColumnWidth(0, 200);
        ImGui::SetColumnWidth(1, 100);
        ImGui::SetColumnWidth(2, 100);

        ImGui::Text("Path Name"); ImGui::NextColumn();
        ImGui::Text("Total Calls"); ImGui::NextColumn();
        ImGui::Text("Last Called"); ImGui::NextColumn();
        ImGui::Separator();

        for (const auto& [name, stats] : codePaths) {
            ImGui::Text("%s", name.c_str());
            ImGui::NextColumn();

            ImGui::Text("%llu", stats.callCount);
            ImGui::NextColumn();

            if (stats.framesSinceLastCall == 0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "This frame");
            } else if (stats.framesSinceLastCall < 60) {
                ImGui::Text("%llu frames ago", stats.framesSinceLastCall);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                    "%llu frames ago", stats.framesSinceLastCall);
            }
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::EndChild();
    }

    ImGui::Spacing();
    ImGui::TextWrapped(
        "Code path tracking helps identify which parts of your code are being executed "
        "and which are not. Paths that haven't been called recently are highlighted in orange.");
}

void DevOverlay::renderExportTab() {
    ImGui::SeparatorText("Metrics Export");

    ImGui::TextWrapped(
        "Export current metrics data for external analysis or debugging.");

    ImGui::Spacing();

    static std::string jsonExport;
    static bool showExport = false;

    if (ImGui::Button("Export to JSON")) {
        jsonExport = metrics::MetricsCollector::instance().exportToJson();
        showExport = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Copy to Clipboard") && !jsonExport.empty()) {
        ImGui::SetClipboardText(jsonExport.c_str());
    }

    ImGui::SameLine();
    if (ImGui::Button("Log Summary")) {
        spdlog::info("\n{}", metrics::getMetricsSummary());
    }

    if (showExport && !jsonExport.empty()) {
        ImGui::Spacing();
        ImGui::SeparatorText("JSON Output");

        ImGui::BeginChild("JSONOutput", ImVec2(0, 300), true,
                          ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(jsonExport.c_str());
        ImGui::EndChild();
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Tracy Profiler");

    if constexpr (metrics::kTracyEnabled) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
            "Tracy is ENABLED");
        ImGui::TextWrapped(
            "Connect with the Tracy profiler application to see detailed "
            "frame-by-frame profiling data with timeline visualization.");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
            "Tracy is DISABLED");
        ImGui::TextWrapped(
            "Rebuild with JFRAME_ENABLE_TRACY=ON to enable Tracy profiler integration.");
    }
}

void DevOverlay::setLastReload(const std::string& file) {
    lastReloadFile_ = file;
    lastReloadTime_ = std::chrono::steady_clock::now();
}

}  // namespace jframe::dev
