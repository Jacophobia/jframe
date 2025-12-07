// bestow-ui/src/bestow.ui.impl.cppm
// UI System implementation using RmlUi

module;

#ifdef BESTOW_HAS_RMLUI
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#endif

#include <GLFW/glfw3.h>

export module bestow.ui.impl;

import std;
import bestow.ui;
import bestow.types;

export namespace bestow {

//==========================================================================
// RmlUi System Interface Implementation
//==========================================================================

#ifdef BESTOW_HAS_RMLUI

// Forward declarations for RmlUi backend integration
class BestowRenderInterface;
class BestowSystemInterface;

class RmlUISystem : public IUISystem {
public:
    RmlUISystem() = default;
    ~RmlUISystem() override;

    //======================================================================
    // Lifecycle
    //======================================================================

    Result<void, UIError> initialize(const UIConfig& config) override;
    void shutdown() override;

    //======================================================================
    // Document Management
    //======================================================================

    Result<UIDocumentHandle, UIError> loadDocument(
        const std::filesystem::path& path) override;

    Result<UIDocumentHandle, UIError> loadDocumentFromString(
        std::string_view content,
        const std::string& sourceName = "inline") override;

    void unloadDocument(UIDocumentHandle doc) override;
    void showDocument(UIDocumentHandle doc) override;
    void hideDocument(UIDocumentHandle doc) override;
    bool isDocumentVisible(UIDocumentHandle doc) const override;
    std::vector<UIDocumentHandle> getLoadedDocuments() const override;

    //======================================================================
    // Style Sheet Management
    //======================================================================

    Result<UIStyleSheetHandle, UIError> loadStyleSheet(
        const std::filesystem::path& path) override;

    Result<void, UIError> applyStyleSheet(
        UIDocumentHandle doc,
        UIStyleSheetHandle styleSheet) override;

    //======================================================================
    // Element Access
    //======================================================================

    std::optional<UIElementHandle> getElementById(
        UIDocumentHandle doc,
        const std::string& id) override;

    std::vector<UIElementHandle> getElementsByClass(
        UIDocumentHandle doc,
        const std::string& className) override;

    std::vector<UIElementHandle> getElementsByTag(
        UIDocumentHandle doc,
        const std::string& tagName) override;

    std::vector<UIElementHandle> getChildren(UIElementHandle element) override;
    std::optional<UIElementHandle> getParent(UIElementHandle element) override;

    //======================================================================
    // Element Properties
    //======================================================================

    void setElementText(UIElementHandle elem, const std::string& text) override;
    std::string getElementText(UIElementHandle elem) override;
    void setElementVisible(UIElementHandle elem, UIVisibility visibility) override;
    UIVisibility getElementVisibility(UIElementHandle elem) override;
    void addElementClass(UIElementHandle elem, const std::string& className) override;
    void removeElementClass(UIElementHandle elem, const std::string& className) override;
    bool hasElementClass(UIElementHandle elem, const std::string& className) override;

    void setElementAttribute(
        UIElementHandle elem,
        const std::string& name,
        const std::string& value) override;

    std::optional<std::string> getElementAttribute(
        UIElementHandle elem,
        const std::string& name) override;

    void setElementStyle(
        UIElementHandle elem,
        const std::string& property,
        const std::string& value) override;

    UIRect getElementBounds(UIElementHandle elem) override;
    void focusElement(UIElementHandle elem) override;
    void blurElement(UIElementHandle elem) override;

    //======================================================================
    // Dynamic Element Creation
    //======================================================================

    UIElementHandle createElement(
        UIDocumentHandle doc,
        const std::string& tagName) override;

    void appendChild(UIElementHandle parent, UIElementHandle child) override;
    void removeElement(UIElementHandle element) override;
    void setInnerRml(UIElementHandle elem, const std::string& rml) override;

    //======================================================================
    // Data Binding
    //======================================================================

    void bindData(const std::string& name, int* value) override;
    void bindData(const std::string& name, float* value) override;
    void bindData(const std::string& name, bool* value) override;
    void bindData(const std::string& name, std::string* value) override;
    void unbindData(const std::string& name) override;
    void syncBindings() override;

    //======================================================================
    // Event Handling
    //======================================================================

    void registerEventCallback(
        const std::string& eventType,
        UIEventCallback callback) override;

    void registerElementCallback(
        UIElementHandle elem,
        const std::string& eventType,
        UIEventCallback callback) override;

    void unregisterEventCallback(const std::string& eventType) override;

    //======================================================================
    // Input Processing
    //======================================================================

    bool processInput(const UIInputEvent& event) override;
    bool wantsKeyboardInput() const override;
    bool wantsMouseInput() const override;

    //======================================================================
    // Update and Render
    //======================================================================

    void update(DeltaTime dt) override;
    void render() override;

    //======================================================================
    // Fonts
    //======================================================================

    Result<void, UIError> loadFont(
        const std::filesystem::path& path,
        const std::string& familyName = "") override;

    //======================================================================
    // Debug
    //======================================================================

    void setDebugMode(bool enabled) override;
    std::size_t getElementCount() const override;

    //======================================================================
    // Window Integration
    //======================================================================

    void setViewportSize(int width, int height) override;
    void setDPIScale(float scale) override;

    // Additional method for GLFW integration
    void setWindow(GLFWwindow* window) { window_ = window; }

private:
    GLFWwindow* window_ = nullptr;
    Rml::Context* context_ = nullptr;
    UIConfig config_;
    bool initialized_ = false;
    bool debugMode_ = false;

    int viewportWidth_ = 800;
    int viewportHeight_ = 600;
    float dpiScale_ = 1.0f;

    // Handle mappings
    UIDocumentHandle nextDocHandle_ = 1;
    std::unordered_map<UIDocumentHandle, Rml::ElementDocument*> documents_;
    std::unordered_map<Rml::ElementDocument*, UIDocumentHandle> documentHandles_;

    UIStyleSheetHandle nextStyleHandle_ = 1;
    std::unordered_map<UIStyleSheetHandle, Rml::StyleSheet*> styleSheets_;

    // Element handle mappings (elements are owned by documents)
    std::unordered_map<UIElementHandle, Rml::Element*> elements_;
    std::unordered_map<Rml::Element*, UIElementHandle> elementHandles_;
    UIElementHandle nextElemHandle_ = 1;

    // Event callbacks
    std::unordered_map<std::string, std::vector<UIEventCallback>> eventCallbacks_;

    // Data bindings
    struct DataBindingEntry {
        UIDataType type;
        void* ptr;
    };
    std::unordered_map<std::string, DataBindingEntry> dataBindings_;

    // Backend interfaces
    std::unique_ptr<BestowRenderInterface> renderInterface_;
    std::unique_ptr<BestowSystemInterface> systemInterface_;

    // Helper methods
    UIElementHandle registerElement(Rml::Element* elem);
    Rml::Element* getElement(UIElementHandle handle);
    Rml::ElementDocument* getDocument(UIDocumentHandle handle);
};

#else // !BESTOW_HAS_RMLUI

//==========================================================================
// Stub Implementation (when RmlUi is not available)
//==========================================================================

class StubUISystem : public IUISystem {
public:
    Result<void, UIError> initialize(const UIConfig& config) override {
        return {};
    }

    void shutdown() override {}

    Result<UIDocumentHandle, UIError> loadDocument(
        const std::filesystem::path& path) override {
        return std::unexpected(UIError::InternalError);
    }

    Result<UIDocumentHandle, UIError> loadDocumentFromString(
        std::string_view content,
        const std::string& sourceName) override {
        return std::unexpected(UIError::InternalError);
    }

    void unloadDocument(UIDocumentHandle doc) override {}
    void showDocument(UIDocumentHandle doc) override {}
    void hideDocument(UIDocumentHandle doc) override {}
    bool isDocumentVisible(UIDocumentHandle doc) const override { return false; }
    std::vector<UIDocumentHandle> getLoadedDocuments() const override { return {}; }

    Result<UIStyleSheetHandle, UIError> loadStyleSheet(
        const std::filesystem::path& path) override {
        return std::unexpected(UIError::InternalError);
    }

    Result<void, UIError> applyStyleSheet(
        UIDocumentHandle doc,
        UIStyleSheetHandle styleSheet) override {
        return std::unexpected(UIError::InternalError);
    }

    std::optional<UIElementHandle> getElementById(
        UIDocumentHandle doc,
        const std::string& id) override { return std::nullopt; }

    std::vector<UIElementHandle> getElementsByClass(
        UIDocumentHandle doc,
        const std::string& className) override { return {}; }

    std::vector<UIElementHandle> getElementsByTag(
        UIDocumentHandle doc,
        const std::string& tagName) override { return {}; }

    std::vector<UIElementHandle> getChildren(UIElementHandle element) override { return {}; }
    std::optional<UIElementHandle> getParent(UIElementHandle element) override { return std::nullopt; }

    void setElementText(UIElementHandle elem, const std::string& text) override {}
    std::string getElementText(UIElementHandle elem) override { return ""; }
    void setElementVisible(UIElementHandle elem, UIVisibility visibility) override {}
    UIVisibility getElementVisibility(UIElementHandle elem) override { return UIVisibility::Hidden; }
    void addElementClass(UIElementHandle elem, const std::string& className) override {}
    void removeElementClass(UIElementHandle elem, const std::string& className) override {}
    bool hasElementClass(UIElementHandle elem, const std::string& className) override { return false; }

    void setElementAttribute(
        UIElementHandle elem,
        const std::string& name,
        const std::string& value) override {}

    std::optional<std::string> getElementAttribute(
        UIElementHandle elem,
        const std::string& name) override { return std::nullopt; }

    void setElementStyle(
        UIElementHandle elem,
        const std::string& property,
        const std::string& value) override {}

    UIRect getElementBounds(UIElementHandle elem) override { return {}; }
    void focusElement(UIElementHandle elem) override {}
    void blurElement(UIElementHandle elem) override {}

    UIElementHandle createElement(
        UIDocumentHandle doc,
        const std::string& tagName) override { return 0; }

    void appendChild(UIElementHandle parent, UIElementHandle child) override {}
    void removeElement(UIElementHandle element) override {}
    void setInnerRml(UIElementHandle elem, const std::string& rml) override {}

    void bindData(const std::string& name, int* value) override {}
    void bindData(const std::string& name, float* value) override {}
    void bindData(const std::string& name, bool* value) override {}
    void bindData(const std::string& name, std::string* value) override {}
    void unbindData(const std::string& name) override {}
    void syncBindings() override {}

    void registerEventCallback(
        const std::string& eventType,
        UIEventCallback callback) override {}

    void registerElementCallback(
        UIElementHandle elem,
        const std::string& eventType,
        UIEventCallback callback) override {}

    void unregisterEventCallback(const std::string& eventType) override {}

    bool processInput(const UIInputEvent& event) override { return false; }
    bool wantsKeyboardInput() const override { return false; }
    bool wantsMouseInput() const override { return false; }

    void update(DeltaTime dt) override {}
    void render() override {}

    Result<void, UIError> loadFont(
        const std::filesystem::path& path,
        const std::string& familyName) override {
        return std::unexpected(UIError::InternalError);
    }

    void setDebugMode(bool enabled) override {}
    std::size_t getElementCount() const override { return 0; }
    void setViewportSize(int width, int height) override {}
    void setDPIScale(float scale) override {}
};

#endif // BESTOW_HAS_RMLUI

//==========================================================================
// RmlUi Implementation Details
//==========================================================================

#ifdef BESTOW_HAS_RMLUI

RmlUISystem::~RmlUISystem() {
    shutdown();
}

Result<void, UIError> RmlUISystem::initialize(const UIConfig& config) {
    if (initialized_) {
        return {};
    }

    config_ = config;

    // TODO: Create custom render and system interfaces
    // For now, RmlUi needs to be initialized by the graphics system
    // which has access to the OpenGL context

    // Create context
    context_ = Rml::CreateContext("main", Rml::Vector2i(viewportWidth_, viewportHeight_));
    if (!context_) {
        return std::unexpected(UIError::InternalError);
    }

    if (config.enableDebugMode) {
        Rml::Debugger::Initialise(context_);
        debugMode_ = true;
    }

    initialized_ = true;
    return {};
}

void RmlUISystem::shutdown() {
    if (!initialized_) {
        return;
    }

    // Clear all documents
    for (auto& [handle, doc] : documents_) {
        if (doc) {
            doc->Close();
        }
    }
    documents_.clear();
    documentHandles_.clear();
    elements_.clear();
    elementHandles_.clear();

    if (context_) {
        Rml::RemoveContext("main");
        context_ = nullptr;
    }

    Rml::Shutdown();
    initialized_ = false;
}

Result<UIDocumentHandle, UIError> RmlUISystem::loadDocument(
    const std::filesystem::path& path) {

    if (!context_) {
        return std::unexpected(UIError::InternalError);
    }

    Rml::ElementDocument* doc = context_->LoadDocument(path.string());
    if (!doc) {
        return std::unexpected(UIError::ParseError);
    }

    UIDocumentHandle handle = nextDocHandle_++;
    documents_[handle] = doc;
    documentHandles_[doc] = handle;

    return handle;
}

Result<UIDocumentHandle, UIError> RmlUISystem::loadDocumentFromString(
    std::string_view content,
    const std::string& sourceName) {

    if (!context_) {
        return std::unexpected(UIError::InternalError);
    }

    Rml::ElementDocument* doc = context_->LoadDocumentFromMemory(
        Rml::String(content.data(), content.size()), sourceName);

    if (!doc) {
        return std::unexpected(UIError::ParseError);
    }

    UIDocumentHandle handle = nextDocHandle_++;
    documents_[handle] = doc;
    documentHandles_[doc] = handle;

    return handle;
}

void RmlUISystem::unloadDocument(UIDocumentHandle handle) {
    auto it = documents_.find(handle);
    if (it == documents_.end()) return;

    Rml::ElementDocument* doc = it->second;
    documentHandles_.erase(doc);
    doc->Close();
    documents_.erase(it);
}

void RmlUISystem::showDocument(UIDocumentHandle handle) {
    if (auto* doc = getDocument(handle)) {
        doc->Show();
    }
}

void RmlUISystem::hideDocument(UIDocumentHandle handle) {
    if (auto* doc = getDocument(handle)) {
        doc->Hide();
    }
}

bool RmlUISystem::isDocumentVisible(UIDocumentHandle handle) const {
    auto it = documents_.find(handle);
    if (it == documents_.end()) return false;
    return it->second->IsVisible();
}

std::vector<UIDocumentHandle> RmlUISystem::getLoadedDocuments() const {
    std::vector<UIDocumentHandle> result;
    result.reserve(documents_.size());
    for (const auto& [handle, doc] : documents_) {
        result.push_back(handle);
    }
    return result;
}

Result<UIStyleSheetHandle, UIError> RmlUISystem::loadStyleSheet(
    const std::filesystem::path& path) {
    // TODO: Implement stylesheet loading
    return std::unexpected(UIError::InternalError);
}

Result<void, UIError> RmlUISystem::applyStyleSheet(
    UIDocumentHandle doc,
    UIStyleSheetHandle styleSheet) {
    // TODO: Implement stylesheet application
    return std::unexpected(UIError::InternalError);
}

std::optional<UIElementHandle> RmlUISystem::getElementById(
    UIDocumentHandle doc,
    const std::string& id) {

    auto* document = getDocument(doc);
    if (!document) return std::nullopt;

    Rml::Element* elem = document->GetElementById(id);
    if (!elem) return std::nullopt;

    return registerElement(elem);
}

std::vector<UIElementHandle> RmlUISystem::getElementsByClass(
    UIDocumentHandle doc,
    const std::string& className) {

    std::vector<UIElementHandle> result;
    auto* document = getDocument(doc);
    if (!document) return result;

    Rml::ElementList elements;
    document->GetElementsByClassName(elements, className);

    for (Rml::Element* elem : elements) {
        result.push_back(registerElement(elem));
    }
    return result;
}

std::vector<UIElementHandle> RmlUISystem::getElementsByTag(
    UIDocumentHandle doc,
    const std::string& tagName) {

    std::vector<UIElementHandle> result;
    auto* document = getDocument(doc);
    if (!document) return result;

    Rml::ElementList elements;
    document->GetElementsByTagName(elements, tagName);

    for (Rml::Element* elem : elements) {
        result.push_back(registerElement(elem));
    }
    return result;
}

std::vector<UIElementHandle> RmlUISystem::getChildren(UIElementHandle element) {
    std::vector<UIElementHandle> result;
    auto* elem = getElement(element);
    if (!elem) return result;

    for (int i = 0; i < elem->GetNumChildren(); ++i) {
        result.push_back(registerElement(elem->GetChild(i)));
    }
    return result;
}

std::optional<UIElementHandle> RmlUISystem::getParent(UIElementHandle element) {
    auto* elem = getElement(element);
    if (!elem || !elem->GetParentNode()) return std::nullopt;
    return registerElement(elem->GetParentNode());
}

void RmlUISystem::setElementText(UIElementHandle elem, const std::string& text) {
    if (auto* e = getElement(elem)) {
        e->SetInnerRML(text);
    }
}

std::string RmlUISystem::getElementText(UIElementHandle elem) {
    if (auto* e = getElement(elem)) {
        return e->GetInnerRML();
    }
    return "";
}

void RmlUISystem::setElementVisible(UIElementHandle elem, UIVisibility visibility) {
    auto* e = getElement(elem);
    if (!e) return;

    switch (visibility) {
        case UIVisibility::Visible:
            e->SetProperty("visibility", "visible");
            e->SetProperty("display", "block");
            break;
        case UIVisibility::Hidden:
            e->SetProperty("visibility", "hidden");
            break;
        case UIVisibility::Collapsed:
            e->SetProperty("display", "none");
            break;
    }
}

UIVisibility RmlUISystem::getElementVisibility(UIElementHandle elem) {
    auto* e = getElement(elem);
    if (!e) return UIVisibility::Hidden;

    auto display = e->GetProperty("display");
    if (display && display->ToString() == "none") {
        return UIVisibility::Collapsed;
    }

    auto visibility = e->GetProperty("visibility");
    if (visibility && visibility->ToString() == "hidden") {
        return UIVisibility::Hidden;
    }

    return UIVisibility::Visible;
}

void RmlUISystem::addElementClass(UIElementHandle elem, const std::string& className) {
    if (auto* e = getElement(elem)) {
        e->SetClass(className, true);
    }
}

void RmlUISystem::removeElementClass(UIElementHandle elem, const std::string& className) {
    if (auto* e = getElement(elem)) {
        e->SetClass(className, false);
    }
}

bool RmlUISystem::hasElementClass(UIElementHandle elem, const std::string& className) {
    if (auto* e = getElement(elem)) {
        return e->IsClassSet(className);
    }
    return false;
}

void RmlUISystem::setElementAttribute(
    UIElementHandle elem,
    const std::string& name,
    const std::string& value) {
    if (auto* e = getElement(elem)) {
        e->SetAttribute(name, value);
    }
}

std::optional<std::string> RmlUISystem::getElementAttribute(
    UIElementHandle elem,
    const std::string& name) {
    if (auto* e = getElement(elem)) {
        if (e->HasAttribute(name)) {
            return e->GetAttribute<Rml::String>(name, "");
        }
    }
    return std::nullopt;
}

void RmlUISystem::setElementStyle(
    UIElementHandle elem,
    const std::string& property,
    const std::string& value) {
    if (auto* e = getElement(elem)) {
        e->SetProperty(property, value);
    }
}

UIRect RmlUISystem::getElementBounds(UIElementHandle elem) {
    if (auto* e = getElement(elem)) {
        auto box = e->GetAbsoluteOffset(Rml::BoxArea::Border);
        auto size = e->GetBox().GetSize(Rml::BoxArea::Border);
        return {box.x, box.y, size.x, size.y};
    }
    return {};
}

void RmlUISystem::focusElement(UIElementHandle elem) {
    if (auto* e = getElement(elem)) {
        e->Focus();
    }
}

void RmlUISystem::blurElement(UIElementHandle elem) {
    if (auto* e = getElement(elem)) {
        e->Blur();
    }
}

UIElementHandle RmlUISystem::createElement(
    UIDocumentHandle doc,
    const std::string& tagName) {

    auto* document = getDocument(doc);
    if (!document) return 0;

    Rml::ElementPtr elemPtr = document->CreateElement(tagName);
    if (!elemPtr) return 0;

    // Need to hold onto the element - append to body by default
    Rml::Element* elem = elemPtr.get();
    document->AppendChild(std::move(elemPtr));

    return registerElement(elem);
}

void RmlUISystem::appendChild(UIElementHandle parent, UIElementHandle child) {
    auto* p = getElement(parent);
    auto* c = getElement(child);
    if (p && c) {
        // TODO: Handle ownership transfer properly
    }
}

void RmlUISystem::removeElement(UIElementHandle element) {
    auto* e = getElement(element);
    if (e && e->GetParentNode()) {
        e->GetParentNode()->RemoveChild(e);
    }
}

void RmlUISystem::setInnerRml(UIElementHandle elem, const std::string& rml) {
    if (auto* e = getElement(elem)) {
        e->SetInnerRML(rml);
    }
}

void RmlUISystem::bindData(const std::string& name, int* value) {
    dataBindings_[name] = {UIDataType::Int, value};
}

void RmlUISystem::bindData(const std::string& name, float* value) {
    dataBindings_[name] = {UIDataType::Float, value};
}

void RmlUISystem::bindData(const std::string& name, bool* value) {
    dataBindings_[name] = {UIDataType::Bool, value};
}

void RmlUISystem::bindData(const std::string& name, std::string* value) {
    dataBindings_[name] = {UIDataType::String, value};
}

void RmlUISystem::unbindData(const std::string& name) {
    dataBindings_.erase(name);
}

void RmlUISystem::syncBindings() {
    // TODO: Implement data binding synchronization with RmlUi data models
}

void RmlUISystem::registerEventCallback(
    const std::string& eventType,
    UIEventCallback callback) {
    eventCallbacks_[eventType].push_back(std::move(callback));
}

void RmlUISystem::registerElementCallback(
    UIElementHandle elem,
    const std::string& eventType,
    UIEventCallback callback) {
    // TODO: Implement per-element callbacks using RmlUi event listeners
}

void RmlUISystem::unregisterEventCallback(const std::string& eventType) {
    eventCallbacks_.erase(eventType);
}

bool RmlUISystem::processInput(const UIInputEvent& event) {
    if (!context_) return false;

    switch (event.type) {
        case UIInputType::MouseMove:
            return context_->ProcessMouseMove(event.x, event.y, 0);

        case UIInputType::MouseDown:
            return context_->ProcessMouseButtonDown(event.button, 0);

        case UIInputType::MouseUp:
            return context_->ProcessMouseButtonUp(event.button, 0);

        case UIInputType::MouseScroll:
            return context_->ProcessMouseWheel(
                static_cast<float>(event.wheelDelta), 0);

        case UIInputType::KeyDown:
            return context_->ProcessKeyDown(
                static_cast<Rml::Input::KeyIdentifier>(event.keyCode), 0);

        case UIInputType::KeyUp:
            return context_->ProcessKeyUp(
                static_cast<Rml::Input::KeyIdentifier>(event.keyCode), 0);

        case UIInputType::TextInput:
            return context_->ProcessTextInput(event.character);
    }
    return false;
}

bool RmlUISystem::wantsKeyboardInput() const {
    // TODO: Check if any input element is focused
    return false;
}

bool RmlUISystem::wantsMouseInput() const {
    // TODO: Check if mouse is over any UI element
    return false;
}

void RmlUISystem::update(DeltaTime dt) {
    if (context_) {
        context_->Update();
    }
}

void RmlUISystem::render() {
    if (context_) {
        context_->Render();
    }
}

Result<void, UIError> RmlUISystem::loadFont(
    const std::filesystem::path& path,
    const std::string& familyName) {

    bool success = Rml::LoadFontFace(path.string());
    if (!success) {
        return std::unexpected(UIError::FontNotFound);
    }
    return {};
}

void RmlUISystem::setDebugMode(bool enabled) {
    debugMode_ = enabled;
    if (context_ && debugMode_) {
        Rml::Debugger::SetVisible(enabled);
    }
}

std::size_t RmlUISystem::getElementCount() const {
    return elements_.size();
}

void RmlUISystem::setViewportSize(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    if (context_) {
        context_->SetDimensions(Rml::Vector2i(width, height));
    }
}

void RmlUISystem::setDPIScale(float scale) {
    dpiScale_ = scale;
    if (context_) {
        context_->SetDensityIndependentPixelRatio(scale);
    }
}

UIElementHandle RmlUISystem::registerElement(Rml::Element* elem) {
    if (!elem) return 0;

    auto it = elementHandles_.find(elem);
    if (it != elementHandles_.end()) {
        return it->second;
    }

    UIElementHandle handle = nextElemHandle_++;
    elements_[handle] = elem;
    elementHandles_[elem] = handle;
    return handle;
}

Rml::Element* RmlUISystem::getElement(UIElementHandle handle) {
    auto it = elements_.find(handle);
    return it != elements_.end() ? it->second : nullptr;
}

Rml::ElementDocument* RmlUISystem::getDocument(UIDocumentHandle handle) {
    auto it = documents_.find(handle);
    return it != documents_.end() ? it->second : nullptr;
}

#endif // BESTOW_HAS_RMLUI

//==========================================================================
// Factory Functions
//==========================================================================

inline std::unique_ptr<IUISystem> createUISystem() {
#ifdef BESTOW_HAS_RMLUI
    return std::make_unique<RmlUISystem>();
#else
    return std::make_unique<StubUISystem>();
#endif
}

}  // namespace bestow
