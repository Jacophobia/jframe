// jframe-dev/src/EntityInspector.cpp
// Entity inspector implementation using ImGui

module;

#include <optional>
#include <string>
#include <vector>

#include <imgui.h>

module jframe.dev;

namespace jframe::dev {

// Forward declaration and external accessor
class ComponentRegistry {
public:
    static ComponentRegistry& instance();
    std::vector<std::string> getComponentsForEntity(const JFrameEngine& engine, Entity entity) const;
    std::string serializeComponent(const JFrameEngine& engine, Entity entity, const std::string& componentName) const;
};

ComponentRegistry& getComponentRegistry();

EntityInspector::EntityInspector(JFrameEngine& engine) : engine_(engine) {}

void EntityInspector::update() {
    handleEntitySelection();
}

void EntityInspector::handleEntitySelection() {
    // Manual entity selection via keyboard shortcut
    // Game code should call selectEntity() directly based on mouse clicks or other input
}

void EntityInspector::render() {
    renderInspectorWindow();
}

void EntityInspector::renderInspectorWindow() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 410, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);

    ImGui::Begin("Entity Inspector");

    if (!selectedEntity_ || !engine_.entities) {
        ImGui::TextDisabled("No entity selected");
        ImGui::Spacing();
        ImGui::Text("Select an entity in the game to inspect it.");
        ImGui::End();
        return;
    }

    Entity entity = *selectedEntity_;

    // Check if entity is still valid
    if (!engine_.entities->isValid(entity)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Entity destroyed!");
        if (ImGui::Button("Clear Selection")) {
            clearSelection();
        }
        ImGui::End();
        return;
    }

    // Entity Header
    ImGui::Text("Entity ID: %u", static_cast<unsigned int>(entity));
    ImGui::Separator();

    // Get components for this entity
    auto& registry = getComponentRegistry();
    auto components = registry.getComponentsForEntity(engine_, entity);

    if (components.empty()) {
        ImGui::TextDisabled("No components");
    } else {
        ImGui::Text("Components (%zu):", components.size());
        ImGui::Spacing();

        // Display each component in a collapsible header
        for (const auto& componentName : components) {
            if (ImGui::CollapsingHeader(componentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                std::string serialized = registry.serializeComponent(engine_, entity, componentName);

                // Display as read-only text
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 0.7f, 1.0f));
                ImGui::TextWrapped("%s", serialized.c_str());
                ImGui::PopStyleColor();

                ImGui::Spacing();
            }
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Action buttons
    if (ImGui::Button("Copy Entity (Lua)")) {
        std::string lua = serializeEntityToLua(entity);
        ImGui::SetClipboardText(lua.c_str());
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Selection")) {
        clearSelection();
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
    if (!engine_.entities || !engine_.entities->isValid(entity)) {
        return "-- Invalid entity\n";
    }

    std::string lua = "{\n";

    // Get all components and serialize them
    auto& registry = getComponentRegistry();
    auto components = registry.getComponentsForEntity(engine_, entity);

    for (const auto& componentName : components) {
        std::string serialized = registry.serializeComponent(engine_, entity, componentName);
        lua += "  " + serialized + ",\n";
    }

    lua += "}";
    return lua;
}

}  // namespace jframe::dev
