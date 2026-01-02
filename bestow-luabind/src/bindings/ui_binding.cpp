// bestow-luabind/src/bindings/ui_binding.cpp
// UI System Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

void bindUISystem(sol::state& lua, IUISystem& ui) {
    // Get the bestow table
    sol::table bestow = lua["bestow"];

    //=========================================================================
    // Enums
    //=========================================================================

    lua.new_enum<UIError>("UIError", {
        {"None", UIError::None},
        {"DocumentNotFound", UIError::DocumentNotFound},
        {"ElementNotFound", UIError::ElementNotFound},
        {"ParseError", UIError::ParseError},
        {"StyleSheetError", UIError::StyleSheetError},
        {"FontNotFound", UIError::FontNotFound},
        {"InvalidHandle", UIError::InvalidHandle},
        {"NotInitialized", UIError::NotInitialized},
        {"InternalError", UIError::InternalError}
    });

    lua.new_enum<UIInputType>("UIInputType", {
        {"MouseMove", UIInputType::MouseMove},
        {"MouseDown", UIInputType::MouseDown},
        {"MouseUp", UIInputType::MouseUp},
        {"MouseWheel", UIInputType::MouseWheel},
        {"KeyDown", UIInputType::KeyDown},
        {"KeyUp", UIInputType::KeyUp},
        {"TextInput", UIInputType::TextInput}
    });

    lua.new_enum<UIVisibility>("UIVisibility", {
        {"Visible", UIVisibility::Visible},
        {"Hidden", UIVisibility::Hidden},
        {"Collapsed", UIVisibility::Collapsed}
    });

    lua.new_enum<UIDataType>("UIDataType", {
        {"Int", UIDataType::Int},
        {"Float", UIDataType::Float},
        {"Bool", UIDataType::Bool},
        {"String", UIDataType::String}
    });

    //=========================================================================
    // Types
    //=========================================================================

    lua.new_usertype<UIRect>("UIRect",
        sol::constructors<UIRect()>(),
        "x", &UIRect::x,
        "y", &UIRect::y,
        "width", &UIRect::width,
        "height", &UIRect::height
    );

    lua.new_usertype<UIConfig>("UIConfig",
        sol::constructors<UIConfig()>(),
        "baseScale", &UIConfig::baseScale,
        "enableDebugMode", &UIConfig::enableDebugMode,
        "assetsPath", &UIConfig::assetsPath,
        "fontsPath", &UIConfig::fontsPath
    );

    lua.new_usertype<UIInputEvent>("UIInputEvent",
        sol::constructors<UIInputEvent()>(),
        "type", &UIInputEvent::type,
        "x", &UIInputEvent::x,
        "y", &UIInputEvent::y,
        "button", &UIInputEvent::button,
        "wheelDeltaX", &UIInputEvent::wheelDeltaX,
        "wheelDeltaY", &UIInputEvent::wheelDeltaY,
        "keyCode", &UIInputEvent::keyCode,
        "modifiers", &UIInputEvent::modifiers,
        "character", &UIInputEvent::character
    );

    lua.new_usertype<UIEventData>("UIEventData",
        sol::constructors<UIEventData()>(),
        "document", &UIEventData::document,
        "element", &UIEventData::element,
        "eventType", &UIEventData::eventType,
        "targetId", &UIEventData::targetId,
        "targetClass", &UIEventData::targetClass,
        "mouseX", &UIEventData::mouseX,
        "mouseY", &UIEventData::mouseY
    );

    //=========================================================================
    // UI System API
    //=========================================================================

    sol::table uiTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Lifecycle
    //-------------------------------------------------------------------------

    uiTable["initialize"] = [&ui](sol::optional<sol::table> configTable) {
        UIConfig config{};
        if (configTable) {
            sol::table t = *configTable;
            if (t["baseScale"].valid()) config.baseScale = t["baseScale"].get<float>();
            if (t["enableDebugMode"].valid()) config.enableDebugMode = t["enableDebugMode"].get<bool>();
            if (t["assetsPath"].valid()) config.assetsPath = t["assetsPath"].get<std::string>();
            if (t["fontsPath"].valid()) config.fontsPath = t["fontsPath"].get<std::string>();
        }
        auto result = ui.initialize(config);
        return result.has_value();
    };

    uiTable["shutdown"] = [&ui]() {
        ui.shutdown();
    };

    //-------------------------------------------------------------------------
    // Document Management
    //-------------------------------------------------------------------------

    uiTable["loadDocument"] = [&ui](const std::string& path) -> sol::optional<UIDocumentHandle> {
        auto result = ui.loadDocument(path);
        if (result.has_value()) {
            return result.value();
        }
        spdlog::warn("[Lua UI] Failed to load document: {}", path);
        return sol::nullopt;
    };

    uiTable["loadDocumentFromString"] = [&ui](const std::string& content,
                                               sol::optional<std::string> sourceName)
        -> sol::optional<UIDocumentHandle> {
        auto result = ui.loadDocumentFromString(content, sourceName.value_or("inline"));
        if (result.has_value()) {
            return result.value();
        }
        return sol::nullopt;
    };

    uiTable["unloadDocument"] = [&ui](UIDocumentHandle doc) {
        ui.unloadDocument(doc);
    };

    uiTable["showDocument"] = [&ui](UIDocumentHandle doc) {
        ui.showDocument(doc);
    };

    uiTable["hideDocument"] = [&ui](UIDocumentHandle doc) {
        ui.hideDocument(doc);
    };

    uiTable["isDocumentVisible"] = [&ui](UIDocumentHandle doc) {
        return ui.isDocumentVisible(doc);
    };

    uiTable["getLoadedDocuments"] = [&ui]() {
        return ui.getLoadedDocuments();
    };

    //-------------------------------------------------------------------------
    // StyleSheet Management
    //-------------------------------------------------------------------------

    uiTable["loadStyleSheet"] = [&ui](const std::string& path) -> sol::optional<UIStyleSheetHandle> {
        auto result = ui.loadStyleSheet(path);
        if (result.has_value()) {
            return result.value();
        }
        spdlog::warn("[Lua UI] Failed to load stylesheet: {}", path);
        return sol::nullopt;
    };

    uiTable["applyStyleSheet"] = [&ui](UIDocumentHandle doc, UIStyleSheetHandle style) {
        auto result = ui.applyStyleSheet(doc, style);
        return result.has_value();
    };

    //-------------------------------------------------------------------------
    // Element Access
    //-------------------------------------------------------------------------

    uiTable["getElementById"] = [&ui](UIDocumentHandle doc, const std::string& id)
        -> sol::optional<UIElementHandle> {
        return ui.getElementById(doc, id);
    };

    uiTable["getElementsByClass"] = [&ui](UIDocumentHandle doc, const std::string& className) {
        return ui.getElementsByClass(doc, className);
    };

    uiTable["getElementsByTag"] = [&ui](UIDocumentHandle doc, const std::string& tagName) {
        return ui.getElementsByTag(doc, tagName);
    };

    uiTable["getChildren"] = [&ui](UIElementHandle elem) {
        return ui.getChildren(elem);
    };

    uiTable["getParent"] = [&ui](UIElementHandle elem) -> sol::optional<UIElementHandle> {
        return ui.getParent(elem);
    };

    //-------------------------------------------------------------------------
    // Element Properties
    //-------------------------------------------------------------------------

    uiTable["setElementText"] = [&ui](UIElementHandle elem, const std::string& text) {
        ui.setElementText(elem, text);
    };

    uiTable["getElementText"] = [&ui](UIElementHandle elem) {
        return ui.getElementText(elem);
    };

    uiTable["setElementVisible"] = [&ui](UIElementHandle elem, UIVisibility visibility) {
        ui.setElementVisible(elem, visibility);
    };

    uiTable["getElementVisibility"] = [&ui](UIElementHandle elem) {
        return ui.getElementVisibility(elem);
    };

    uiTable["addClass"] = [&ui](UIElementHandle elem, const std::string& className) {
        ui.addElementClass(elem, className);
    };

    uiTable["removeClass"] = [&ui](UIElementHandle elem, const std::string& className) {
        ui.removeElementClass(elem, className);
    };

    uiTable["hasClass"] = [&ui](UIElementHandle elem, const std::string& className) {
        return ui.hasElementClass(elem, className);
    };

    uiTable["setAttribute"] = [&ui](UIElementHandle elem,
                                     const std::string& name,
                                     const std::string& value) {
        ui.setElementAttribute(elem, name, value);
    };

    uiTable["getAttribute"] = [&ui](UIElementHandle elem, const std::string& name)
        -> sol::optional<std::string> {
        return ui.getElementAttribute(elem, name);
    };

    uiTable["setStyle"] = [&ui](UIElementHandle elem,
                                 const std::string& property,
                                 const std::string& value) {
        ui.setElementStyle(elem, property, value);
    };

    uiTable["getBounds"] = [&ui](UIElementHandle elem) {
        return ui.getElementBounds(elem);
    };

    uiTable["focus"] = [&ui](UIElementHandle elem) {
        ui.focusElement(elem);
    };

    uiTable["blur"] = [&ui](UIElementHandle elem) {
        ui.blurElement(elem);
    };

    //-------------------------------------------------------------------------
    // Dynamic Element Creation
    //-------------------------------------------------------------------------

    uiTable["createElement"] = [&ui](UIDocumentHandle doc, const std::string& tagName) {
        return ui.createElement(doc, tagName);
    };

    uiTable["appendChild"] = [&ui](UIElementHandle parent, UIElementHandle child) {
        ui.appendChild(parent, child);
    };

    uiTable["removeElement"] = [&ui](UIElementHandle elem) {
        ui.removeElement(elem);
    };

    uiTable["setInnerContent"] = [&ui](UIElementHandle elem, const std::string& content) {
        ui.setInnerRml(elem, content);
    };

    // Alias for backwards compatibility
    uiTable["setInnerRml"] = uiTable["setInnerContent"];

    //-------------------------------------------------------------------------
    // Data Binding (simplified for Lua - uses string values)
    //-------------------------------------------------------------------------

    // For Lua, we provide a simplified string-based data binding
    // The game can call syncBindings() to update UI elements
    uiTable["syncBindings"] = [&ui]() {
        ui.syncBindings();
    };

    //-------------------------------------------------------------------------
    // Event Handling
    //-------------------------------------------------------------------------

    uiTable["onEvent"] = [&ui](const std::string& eventType, sol::function callback) {
        ui.registerEventCallback(eventType, [callback](const UIEventData& data) {
            try {
                callback(data);
            } catch (const std::exception& e) {
                spdlog::error("[Lua UI] Event callback error: {}", e.what());
            }
        });
    };

    uiTable["onElementEvent"] = [&ui](UIElementHandle elem,
                                       const std::string& eventType,
                                       sol::function callback) {
        ui.registerElementCallback(elem, eventType, [callback](const UIEventData& data) {
            try {
                callback(data);
            } catch (const std::exception& e) {
                spdlog::error("[Lua UI] Element event callback error: {}", e.what());
            }
        });
    };

    uiTable["offEvent"] = [&ui](const std::string& eventType) {
        ui.unregisterEventCallback(eventType);
    };

    //-------------------------------------------------------------------------
    // Input Processing
    //-------------------------------------------------------------------------

    uiTable["processInput"] = [&ui](sol::table eventTable) {
        UIInputEvent event{};
        if (eventTable["type"].valid()) event.type = eventTable["type"].get<UIInputType>();
        if (eventTable["x"].valid()) event.x = eventTable["x"].get<int>();
        if (eventTable["y"].valid()) event.y = eventTable["y"].get<int>();
        if (eventTable["button"].valid()) event.button = eventTable["button"].get<int>();
        if (eventTable["wheelDeltaX"].valid()) event.wheelDeltaX = eventTable["wheelDeltaX"].get<float>();
        if (eventTable["wheelDeltaY"].valid()) event.wheelDeltaY = eventTable["wheelDeltaY"].get<float>();
        if (eventTable["keyCode"].valid()) event.keyCode = eventTable["keyCode"].get<int>();
        if (eventTable["modifiers"].valid()) event.modifiers = eventTable["modifiers"].get<int>();
        if (eventTable["character"].valid()) {
            std::string charStr = eventTable["character"].get<std::string>();
            if (!charStr.empty()) event.character = charStr[0];
        }
        return ui.processInput(event);
    };

    uiTable["wantsKeyboardInput"] = [&ui]() {
        return ui.wantsKeyboardInput();
    };

    uiTable["wantsMouseInput"] = [&ui]() {
        return ui.wantsMouseInput();
    };

    //-------------------------------------------------------------------------
    // Update and Render
    //-------------------------------------------------------------------------

    uiTable["update"] = [&ui](float dt) {
        ui.update(DeltaTime(dt));
    };

    uiTable["render"] = [&ui]() {
        ui.render();
    };

    //-------------------------------------------------------------------------
    // Fonts
    //-------------------------------------------------------------------------

    uiTable["loadFont"] = [&ui](const std::string& path, sol::optional<std::string> familyName) {
        auto result = ui.loadFont(path, familyName.value_or(""));
        if (!result.has_value()) {
            spdlog::warn("[Lua UI] Failed to load font: {}", path);
        }
        return result.has_value();
    };

    //-------------------------------------------------------------------------
    // Debug
    //-------------------------------------------------------------------------

    uiTable["setDebugMode"] = [&ui](bool enabled) {
        ui.setDebugMode(enabled);
    };

    uiTable["getElementCount"] = [&ui]() {
        return ui.getElementCount();
    };

    //-------------------------------------------------------------------------
    // Window/Viewport
    //-------------------------------------------------------------------------

    uiTable["setViewportSize"] = [&ui](int width, int height) {
        ui.setViewportSize(width, height);
    };

    uiTable["setDPIScale"] = [&ui](float scale) {
        ui.setDPIScale(scale);
    };

    // Attach to bestow table
    bestow["ui"] = uiTable;

    spdlog::debug("[LuaContractBinder] Bound IUISystem -> bestow.ui");
}

}  // namespace bestow
