// jframe-dev/src/EntityInspector.cpp
// Entity inspector implementation using ImGui

module;

#include <optional>
#include <string>

#include <imgui.h>

module jframe.dev;

namespace jframe::dev {

EntityInspector::EntityInspector(JFrameEngine& engine) : engine_(engine) {}

void EntityInspector::update() {
    handleEntitySelection();
}

void EntityInspector::handleEntitySelection() {
    if (engine_.input && engine_.input->wasActionJustPressed("dev_select")) {
        Vec2 mouseWorld = engine_.graphics->screenToWorld(
            engine_.input->getMousePosition()
        );
        // Find entity at position (would need spatial query)
    }
}

void EntityInspector::render() {
    if (!selectedEntity_) return;

    renderInspectorWindow();
}

void EntityInspector::renderInspectorWindow() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 310, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

    ImGui::Begin("Entity Inspector");

    if (selectedEntity_ && engine_.entities) {
        ImGui::Text("Entity ID: %u", static_cast<unsigned int>(*selectedEntity_));

        // Would show components here

        if (ImGui::Button("Copy Position (Lua)")) {
            // Copy position to clipboard
            ImGui::SetClipboardText("x = 0.0, y = 0.0");
        }

        if (ImGui::Button("Copy Entity (Lua)")) {
            std::string lua = serializeEntityToLua(*selectedEntity_);
            ImGui::SetClipboardText(lua.c_str());
        }
    }

    ImGui::End();
}

void EntityInspector::selectEntity(Entity entity) {
    selectedEntity_ = entity;
}

void EntityInspector::clearSelection() {
    selectedEntity_ = std::nullopt;
}

std::optional<Entity> EntityInspector::getSelectedEntity() const {
    return selectedEntity_;
}

std::string EntityInspector::serializeEntityToLua(Entity entity) {
    std::string lua = "{\n";
    lua += "  -- Entity data\n";
    lua += "}";
    return lua;
}

}  // namespace jframe::dev
