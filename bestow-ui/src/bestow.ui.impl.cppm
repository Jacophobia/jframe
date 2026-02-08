// bestow-ui/src/bestow.ui.impl.cppm
// UI System implementation using RmlUi
// Renderer-agnostic: depends only on IGraphicsContext and IUIRenderBackend

module;

#ifdef BESTOW_HAS_RMLUI
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#endif

#include <spdlog/spdlog.h>
#include <chrono>

export module bestow.ui.impl;

import std;
import bestow.services;  // Re-exports all contracts including bestow.ui, bestow.assets, bestow.types

export namespace bestow {

//==========================================================================
// RmlUi Interface Adapters
//==========================================================================

#ifdef BESTOW_HAS_RMLUI

/// Adapter that bridges RmlUi's RenderInterface to our IUIRenderBackend.
/// This class translates RmlUi rendering calls to our backend-agnostic interface.
class BestowRmlRenderInterface : public Rml::RenderInterface {
public:
    explicit BestowRmlRenderInterface(IUIRenderBackend& backend)
        : backend_(&backend) {}

    //======================================================================
    // Geometry Compilation
    //======================================================================

    Rml::CompiledGeometryHandle CompileGeometry(
        Rml::Span<const Rml::Vertex> vertices,
        Rml::Span<const int> indices) override
    {
        // Convert RmlUi vertices to our format
        std::vector<UIVertex> uiVerts;
        uiVerts.reserve(vertices.size());
        for (const auto& v : vertices) {
            uiVerts.push_back(UIVertex{
                .position = {v.position.x, v.position.y},
                // RmlUi provides premultiplied colors - pass through
                .color = Color(v.colour.red, v.colour.green,
                              v.colour.blue, v.colour.alpha),
                .texCoord = {v.tex_coord.x, v.tex_coord.y}
            });
        }

        // Convert indices (int -> uint32_t)
        std::vector<std::uint32_t> uiIndices(indices.begin(), indices.end());

        UIGeometryHandle handle = backend_->compileGeometry(uiVerts, uiIndices);
        return static_cast<Rml::CompiledGeometryHandle>(handle);
    }

    void RenderGeometry(
        Rml::CompiledGeometryHandle geometry,
        Rml::Vector2f translation,
        Rml::TextureHandle texture) override
    {
        backend_->renderGeometry(
            static_cast<UIGeometryHandle>(geometry),
            Vec2{translation.x, translation.y},
            static_cast<UITextureHandle>(texture));
    }

    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override {
        backend_->releaseGeometry(static_cast<UIGeometryHandle>(geometry));
    }

    //======================================================================
    // Texture Management
    //======================================================================

    Rml::TextureHandle LoadTexture(
        Rml::Vector2i& texture_dimensions,
        const Rml::String& source) override
    {
        int width = 0, height = 0;
        UITextureHandle handle = backend_->loadTexture(source, width, height);
        texture_dimensions.x = width;
        texture_dimensions.y = height;
        return static_cast<Rml::TextureHandle>(handle);
    }

    Rml::TextureHandle GenerateTexture(
        Rml::Span<const Rml::byte> source,
        Rml::Vector2i dimensions) override
    {
        std::span<const std::uint8_t> data(source.data(), source.size());
        UITextureHandle handle = backend_->createTexture(
            data, dimensions.x, dimensions.y);
        return static_cast<Rml::TextureHandle>(handle);
    }

    void ReleaseTexture(Rml::TextureHandle texture) override {
        backend_->releaseTexture(static_cast<UITextureHandle>(texture));
    }

    //======================================================================
    // Scissor (Clipping)
    //======================================================================

    void EnableScissorRegion(bool enable) override {
        backend_->enableScissor(enable);
    }

    void SetScissorRegion(Rml::Rectanglei region) override {
        backend_->setScissorRegion(UIScissorRect{
            .x = region.Left(),
            .y = region.Top(),
            .width = region.Width(),
            .height = region.Height()
        });
    }

private:
    IUIRenderBackend* backend_;
};

/// Adapter that provides RmlUi's SystemInterface.
/// Provides timing and logging services.
class BestowRmlSystemInterface : public Rml::SystemInterface {
public:
    double GetElapsedTime() override {
        using namespace std::chrono;
        static auto start = steady_clock::now();
        auto now = steady_clock::now();
        return duration<double>(now - start).count();
    }

    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
        switch (type) {
            case Rml::Log::LT_ERROR:
                spdlog::error("[RmlUi] {}", message);
                break;
            case Rml::Log::LT_WARNING:
                spdlog::warn("[RmlUi] {}", message);
                break;
            case Rml::Log::LT_INFO:
                spdlog::info("[RmlUi] {}", message);
                break;
            case Rml::Log::LT_DEBUG:
                spdlog::debug("[RmlUi] {}", message);
                break;
            default:
                spdlog::trace("[RmlUi] {}", message);
                break;
        }
        return true;
    }
};

//==========================================================================
// RmlUISystem Implementation
//==========================================================================

class RmlUISystem : public IUISystem {
public:
    /// Constructor with dependency injection.
    /// Receives IGraphicsContext for render backend access.
    explicit RmlUISystem(IGraphicsContext& graphics, IAssetSystem& assets)
        : graphics_(&graphics), assetSystem_(&assets) {}

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

private:
    // Dependencies (injected)
    IGraphicsContext* graphics_ = nullptr;
    IAssetSystem* assetSystem_ = nullptr;

    // Render backend (obtained from graphics context)
    IUIRenderBackend* renderBackend_ = nullptr;

    // RmlUi interfaces (our adapters)
    std::unique_ptr<BestowRmlRenderInterface> rmlRenderInterface_;
    std::unique_ptr<BestowRmlSystemInterface> rmlSystemInterface_;

    // RmlUi context
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
    std::unordered_map<UIStyleSheetHandle, std::string> styleSheetContents_;

    // Element handle mappings (elements are owned by documents)
    std::unordered_map<UIElementHandle, Rml::Element*> elements_;
    std::unordered_map<Rml::Element*, UIElementHandle> elementHandles_;
    UIElementHandle nextElemHandle_ = 1;

    // Event callbacks
    std::unordered_map<std::string, std::vector<UIEventCallback>> eventCallbacks_;
    std::unordered_map<std::string, std::vector<UIEventCallback>> elementCallbacks_;

    // Data bindings
    struct DataBindingEntry {
        UIDataType type;
        void* ptr;
    };
    std::unordered_map<std::string, DataBindingEntry> dataBindings_;

    //======================================================================
    // Hot Reload Support
    //======================================================================

    // Track which asset handles correspond to which UI handles
    struct DocumentAssetInfo {
        AssetHandle assetHandle;
        SubscriptionId subscriptionId = 0;
        std::filesystem::path path;
        bool wasVisible = false;
    };
    std::unordered_map<UIDocumentHandle, DocumentAssetInfo> documentAssets_;
    std::unordered_map<UUID, UIDocumentHandle> assetToDocument_;

    struct StyleSheetAssetInfo {
        AssetHandle assetHandle;
        SubscriptionId subscriptionId = 0;
        std::filesystem::path path;
        std::vector<UIDocumentHandle> appliedTo;  // Documents using this stylesheet
    };
    std::unordered_map<UIStyleSheetHandle, StyleSheetAssetInfo> styleSheetAssets_;
    std::unordered_map<UUID, UIStyleSheetHandle> assetToStyleSheet_;

    bool hotReloadEnabled_ = false;

    // Hot reload callbacks
    void onDocumentAssetChanged(AssetHandle handle, AssetType type);
    void onStyleSheetAssetChanged(AssetHandle handle, AssetType type);
    void reloadDocument(UIDocumentHandle docHandle);
    void reloadStyleSheet(UIStyleSheetHandle styleHandle);

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

    // Get render backend from graphics context
    if (!graphics_) {
        spdlog::error("[RmlUISystem] No graphics context provided");
        return std::unexpected(UIError::InternalError);
    }

    renderBackend_ = graphics_->getUIRenderBackend();
    if (!renderBackend_) {
        spdlog::error("[RmlUISystem] Graphics context does not provide UI render backend");
        return std::unexpected(UIError::InternalError);
    }

    // Create RmlUi interface adapters
    rmlRenderInterface_ = std::make_unique<BestowRmlRenderInterface>(*renderBackend_);
    rmlSystemInterface_ = std::make_unique<BestowRmlSystemInterface>();

    // CRITICAL: Set interfaces BEFORE Rml::Initialise()
    Rml::SetRenderInterface(rmlRenderInterface_.get());
    Rml::SetSystemInterface(rmlSystemInterface_.get());

    // Initialize RmlUi
    if (!Rml::Initialise()) {
        spdlog::error("[RmlUISystem] Failed to initialize RmlUi");
        return std::unexpected(UIError::InternalError);
    }

    // Get viewport size from graphics context
    Size viewportSize = graphics_->getWindowSize();
    viewportWidth_ = viewportSize.width;
    viewportHeight_ = viewportSize.height;

    // Update render backend viewport
    renderBackend_->setViewportSize(viewportWidth_, viewportHeight_);

    // Create RmlUi context
    context_ = Rml::CreateContext("main", Rml::Vector2i(viewportWidth_, viewportHeight_));
    if (!context_) {
        spdlog::error("[RmlUISystem] Failed to create RmlUi context");
        Rml::Shutdown();
        return std::unexpected(UIError::InternalError);
    }

    if (config.enableDebugMode) {
        Rml::Debugger::Initialise(context_);
        debugMode_ = true;
    }

    // Enable hot reload for UI assets
    if (assetSystem_) {
        assetSystem_->enableHotReload(true);
        hotReloadEnabled_ = true;
        spdlog::info("[RmlUISystem] Hot reload enabled for UI assets");
    }

    spdlog::info("[RmlUISystem] Initialized with {}x{} viewport", viewportWidth_, viewportHeight_);
    initialized_ = true;
    return {};
}

void RmlUISystem::shutdown() {
    if (!initialized_) {
        return;
    }

    // Unsubscribe from all asset change notifications
    if (assetSystem_) {
        for (const auto& [docHandle, info] : documentAssets_) {
            if (info.subscriptionId != 0) {
                assetSystem_->unsubscribe(info.subscriptionId);
            }
        }
        for (const auto& [styleHandle, info] : styleSheetAssets_) {
            if (info.subscriptionId != 0) {
                assetSystem_->unsubscribe(info.subscriptionId);
            }
        }
    }
    documentAssets_.clear();
    assetToDocument_.clear();
    styleSheetAssets_.clear();
    assetToStyleSheet_.clear();

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

    // Clear all stylesheets
    for (auto& [handle, sheet] : styleSheets_) {
        delete sheet;
    }
    styleSheets_.clear();
    styleSheetContents_.clear();

    if (context_) {
        Rml::RemoveContext("main");
        context_ = nullptr;
    }

    Rml::Shutdown();

    // Release our interfaces
    rmlRenderInterface_.reset();
    rmlSystemInterface_.reset();

    initialized_ = false;
    spdlog::info("[RmlUISystem] Shutdown complete");
}

Result<UIDocumentHandle, UIError> RmlUISystem::loadDocument(
    const std::filesystem::path& path) {

    if (!context_) {
        return std::unexpected(UIError::InternalError);
    }

    if (!assetSystem_) {
        return std::unexpected(UIError::InternalError);
    }

    // Register and load the UI document through AssetSystem
    AssetHandle assetHandle = assetSystem_->registerAsset(AssetType::Data, path);
    assetSystem_->loadAsset(assetHandle);

    if (!assetSystem_->isLoaded(assetHandle)) {
        return std::unexpected(UIError::ParseError);
    }

    // Get the raw file data from AssetSystem
    const auto* fileData = assetSystem_->getRawAsset(assetHandle);
    if (!fileData) {
        return std::unexpected(UIError::ParseError);
    }

    // Cast to string data (AssetType::Data loads as std::string)
    const auto* content = static_cast<const std::string*>(fileData);

    // Load document from memory using RmlUi's memory API
    Rml::ElementDocument* doc = context_->LoadDocumentFromMemory(
        Rml::String(content->data(), content->size()), path.string());

    if (!doc) {
        return std::unexpected(UIError::ParseError);
    }

    UIDocumentHandle handle = nextDocHandle_++;
    documents_[handle] = doc;
    documentHandles_[doc] = handle;

    // Set up hot reload: subscribe to asset changes
    if (hotReloadEnabled_) {
        DocumentAssetInfo assetInfo;
        assetInfo.assetHandle = assetHandle;
        assetInfo.path = path;
        assetInfo.wasVisible = false;

        // Subscribe to changes for this specific asset
        assetInfo.subscriptionId = assetSystem_->subscribe(assetHandle,
            [this](AssetHandle h, AssetType t) {
                onDocumentAssetChanged(h, t);
            });

        // Track mappings for fast lookup
        documentAssets_[handle] = std::move(assetInfo);
        assetToDocument_[assetHandle.uuid] = handle;

        spdlog::debug("[RmlUISystem] Subscribed to hot reload for document: {}", path.string());
    }

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

    // Clean up hot reload subscription
    auto assetIt = documentAssets_.find(handle);
    if (assetIt != documentAssets_.end()) {
        if (assetSystem_ && assetIt->second.subscriptionId != 0) {
            assetSystem_->unsubscribe(assetIt->second.subscriptionId);
        }
        assetToDocument_.erase(assetIt->second.assetHandle.uuid);
        documentAssets_.erase(assetIt);
    }

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

    if (!assetSystem_) {
        return std::unexpected(UIError::InternalError);
    }

    // Register and load the stylesheet through AssetSystem
    AssetHandle assetHandle = assetSystem_->registerAsset(AssetType::Data, path);
    assetSystem_->loadAsset(assetHandle);

    if (!assetSystem_->isLoaded(assetHandle)) {
        return std::unexpected(UIError::StyleSheetError);
    }

    // Get the raw file data from AssetSystem
    const auto* fileData = assetSystem_->getRawAsset(assetHandle);
    if (!fileData) {
        return std::unexpected(UIError::StyleSheetError);
    }

    // Cast to string data (AssetType::Data loads as std::string)
    const auto* content = static_cast<const std::string*>(fileData);

    // Create stylesheet using RmlUi's factory
    Rml::SharedPtr<Rml::StyleSheetContainer> container =
        Rml::Factory::InstanceStyleSheetString(Rml::String(content->data(), content->size()));

    if (!container) {
        return std::unexpected(UIError::StyleSheetError);
    }

    // Store the CSS content for re-application
    UIStyleSheetHandle handle = nextStyleHandle_++;
    styleSheetContents_[handle] = *content;

    // Set up hot reload: subscribe to asset changes
    if (hotReloadEnabled_) {
        StyleSheetAssetInfo assetInfo;
        assetInfo.assetHandle = assetHandle;
        assetInfo.path = path;

        // Subscribe to changes for this specific asset
        assetInfo.subscriptionId = assetSystem_->subscribe(assetHandle,
            [this](AssetHandle h, AssetType t) {
                onStyleSheetAssetChanged(h, t);
            });

        // Track mappings for fast lookup
        styleSheetAssets_[handle] = std::move(assetInfo);
        assetToStyleSheet_[assetHandle.uuid] = handle;

        spdlog::debug("[RmlUISystem] Subscribed to hot reload for stylesheet: {}", path.string());
    }

    return handle;
}

Result<void, UIError> RmlUISystem::applyStyleSheet(
    UIDocumentHandle doc,
    UIStyleSheetHandle styleSheet) {

    auto* document = getDocument(doc);
    if (!document) {
        return std::unexpected(UIError::DocumentNotFound);
    }

    auto it = styleSheetContents_.find(styleSheet);
    if (it == styleSheetContents_.end()) {
        return std::unexpected(UIError::StyleSheetError);
    }

    // Inject the stylesheet into the document by adding a <style> element
    Rml::ElementPtr styleElem = document->CreateElement("style");
    if (!styleElem) {
        return std::unexpected(UIError::InternalError);
    }

    styleElem->SetInnerRML(it->second);

    // Insert at the beginning of the document's head (or body if no head)
    Rml::Element* head = document->GetElementById("head");
    if (!head) {
        head = document;
    }

    // Insert the style element
    if (head->GetNumChildren() > 0) {
        head->InsertBefore(std::move(styleElem), head->GetFirstChild());
    } else {
        head->AppendChild(std::move(styleElem));
    }

    // Force document to re-process styles
    document->UpdateDocument();

    // Track which documents use this stylesheet (for hot reload)
    auto assetIt = styleSheetAssets_.find(styleSheet);
    if (assetIt != styleSheetAssets_.end()) {
        // Add doc to the list if not already there
        auto& appliedTo = assetIt->second.appliedTo;
        if (std::find(appliedTo.begin(), appliedTo.end(), doc) == appliedTo.end()) {
            appliedTo.push_back(doc);
        }
    }

    return {};
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
        return std::string(e->GetInnerRML());
    }
    return "";
}

void RmlUISystem::setElementVisible(UIElementHandle elem, UIVisibility visibility) {
    if (auto* e = getElement(elem)) {
        switch (visibility) {
            case UIVisibility::Visible:
                e->SetProperty("visibility", "visible");
                break;
            case UIVisibility::Hidden:
                e->SetProperty("visibility", "hidden");
                break;
            case UIVisibility::Collapsed:
                e->SetProperty("display", "none");
                break;
        }
    }
}

UIVisibility RmlUISystem::getElementVisibility(UIElementHandle elem) {
    if (auto* e = getElement(elem)) {
        auto display = e->GetProperty<Rml::String>("display");
        if (display == "none") return UIVisibility::Collapsed;

        auto visibility = e->GetProperty<Rml::String>("visibility");
        if (visibility == "hidden") return UIVisibility::Hidden;
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
            return std::string(e->GetAttribute<Rml::String>(name, ""));
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
        auto box = e->GetAbsoluteOffset();
        auto size = e->GetBox().GetSize();
        return UIRect{
            static_cast<float>(box.x),
            static_cast<float>(box.y),
            static_cast<float>(size.x),
            static_cast<float>(size.y)
        };
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

    Rml::ElementPtr elem = document->CreateElement(tagName);
    if (!elem) return 0;

    // We need to keep the element alive - append to hidden container?
    // For now, register and return (caller should appendChild immediately)
    return registerElement(elem.get());
}

void RmlUISystem::appendChild(UIElementHandle parent, UIElementHandle child) {
    auto* parentElem = getElement(parent);
    auto* childElem = getElement(child);
    if (parentElem && childElem) {
        // Note: This might not work correctly if childElem was from CreateElement
        // since we don't own the pointer. In practice, caller should create
        // elements via document->CreateElement and manage ownership.
    }
}

void RmlUISystem::removeElement(UIElementHandle element) {
    if (auto* e = getElement(element)) {
        if (auto* parent = e->GetParentNode()) {
            parent->RemoveChild(e);
        }
        elements_.erase(element);
        elementHandles_.erase(e);
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
    if (!context_) return;

    // Find all elements with data-value attribute and update them
    for (const auto& [handle, doc] : documents_) {
        for (const auto& [name, binding] : dataBindings_) {
            std::string selector = "[data-value=\"" + name + "\"]";
            Rml::ElementList elements;
            doc->QuerySelectorAll(elements, selector);

            for (Rml::Element* elem : elements) {
                std::string valueStr;
                switch (binding.type) {
                    case UIDataType::Int:
                        valueStr = std::to_string(*static_cast<int*>(binding.ptr));
                        break;
                    case UIDataType::Float:
                        valueStr = std::to_string(*static_cast<float*>(binding.ptr));
                        break;
                    case UIDataType::Bool:
                        valueStr = *static_cast<bool*>(binding.ptr) ? "true" : "false";
                        break;
                    case UIDataType::String:
                        valueStr = *static_cast<std::string*>(binding.ptr);
                        break;
                }
                elem->SetInnerRML(valueStr);
            }
        }
    }
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
    std::string key = std::to_string(elem) + ":" + eventType;
    elementCallbacks_[key].push_back(std::move(callback));
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
                Rml::Vector2f(0, static_cast<float>(event.wheelDelta)), 0);

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
    if (!context_) return false;
    auto* focus = context_->GetFocusElement();
    return focus != nullptr && focus != context_->GetRootElement();
}

bool RmlUISystem::wantsMouseInput() const {
    if (!context_) return false;
    auto* hover = context_->GetHoverElement();
    return hover != nullptr && hover != context_->GetRootElement();
}

void RmlUISystem::update(DeltaTime dt) {
    // Process hot reload file changes (event-driven via efsw)
    if (hotReloadEnabled_ && assetSystem_) {
        assetSystem_->update();
    }

    if (context_) {
        context_->Update();
    }
}

void RmlUISystem::render() {
    if (!context_ || !renderBackend_) return;

    // Begin UI rendering pass (sets up 2D overlay state)
    renderBackend_->beginUIPass();

    // RmlUi will call our BestowRmlRenderInterface methods
    context_->Render();

    // End UI rendering pass (restores previous state)
    renderBackend_->endUIPass();
}

Result<void, UIError> RmlUISystem::loadFont(
    const std::filesystem::path& path,
    const std::string& familyName) {

    if (!assetSystem_) {
        return std::unexpected(UIError::InternalError);
    }

    // Register and load the font through AssetSystem
    AssetHandle fontHandle = assetSystem_->registerAsset(AssetType::Font, path);
    assetSystem_->loadAsset(fontHandle);

    if (!assetSystem_->isLoaded(fontHandle)) {
        return std::unexpected(UIError::FontNotFound);
    }

    // Get the font data from AssetSystem
    const FontData* fontData = assetSystem_->getAsset<FontData>(fontHandle);
    if (!fontData || fontData->fileData.empty()) {
        return std::unexpected(UIError::FontNotFound);
    }

    // Load font from memory using RmlUi's memory API
    Rml::Span<const Rml::byte> fontSpan(
        reinterpret_cast<const Rml::byte*>(fontData->fileData.data()),
        fontData->fileData.size());

    bool success = Rml::LoadFontFace(
        fontSpan,
        familyName.empty() ? path.stem().string() : familyName,
        Rml::Style::FontStyle::Normal,
        Rml::Style::FontWeight::Auto);

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
    if (renderBackend_) {
        renderBackend_->setViewportSize(width, height);
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

//==========================================================================
// Hot Reload Implementation
//==========================================================================

void RmlUISystem::onDocumentAssetChanged(AssetHandle handle, AssetType type) {
    // Find which document this asset corresponds to
    auto it = assetToDocument_.find(handle.uuid);
    if (it == assetToDocument_.end()) {
        return;  // Not a document we're tracking
    }

    UIDocumentHandle docHandle = it->second;
    spdlog::info("[RmlUISystem] Hot reload triggered for document (handle={})", docHandle);

    reloadDocument(docHandle);
}

void RmlUISystem::onStyleSheetAssetChanged(AssetHandle handle, AssetType type) {
    // Find which stylesheet this asset corresponds to
    auto it = assetToStyleSheet_.find(handle.uuid);
    if (it == assetToStyleSheet_.end()) {
        return;  // Not a stylesheet we're tracking
    }

    UIStyleSheetHandle styleHandle = it->second;
    spdlog::info("[RmlUISystem] Hot reload triggered for stylesheet (handle={})", styleHandle);

    reloadStyleSheet(styleHandle);
}

void RmlUISystem::reloadDocument(UIDocumentHandle docHandle) {
    auto assetIt = documentAssets_.find(docHandle);
    if (assetIt == documentAssets_.end()) {
        spdlog::warn("[RmlUISystem] Cannot reload document {}: no asset info", docHandle);
        return;
    }

    auto docIt = documents_.find(docHandle);
    if (docIt == documents_.end()) {
        spdlog::warn("[RmlUISystem] Cannot reload document {}: not found", docHandle);
        return;
    }

    Rml::ElementDocument* oldDoc = docIt->second;
    const auto& assetInfo = assetIt->second;

    // Remember visibility state
    bool wasVisible = oldDoc->IsVisible();

    // Reload the asset from disk
    assetSystem_->reloadAsset(assetInfo.assetHandle);

    if (!assetSystem_->isLoaded(assetInfo.assetHandle)) {
        spdlog::error("[RmlUISystem] Failed to reload document: {}", assetInfo.path.string());
        return;
    }

    // Get the new content
    const auto* fileData = assetSystem_->getRawAsset(assetInfo.assetHandle);
    if (!fileData) {
        spdlog::error("[RmlUISystem] Failed to get reloaded document data");
        return;
    }

    const auto* content = static_cast<const std::string*>(fileData);

    // Close old document and remove element handles
    documentHandles_.erase(oldDoc);
    for (auto elemIt = elements_.begin(); elemIt != elements_.end(); ) {
        if (elementHandles_.count(elemIt->second) > 0) {
            // Check if this element belongs to the old document
            // For safety, clear all element handles (they're document-owned anyway)
            elemIt = elements_.erase(elemIt);
        } else {
            ++elemIt;
        }
    }
    elementHandles_.clear();
    oldDoc->Close();

    // Load new document
    Rml::ElementDocument* newDoc = context_->LoadDocumentFromMemory(
        Rml::String(content->data(), content->size()), assetInfo.path.string());

    if (!newDoc) {
        spdlog::error("[RmlUISystem] Failed to parse reloaded document: {}", assetInfo.path.string());
        documents_.erase(docIt);
        return;
    }

    // Update mappings
    documents_[docHandle] = newDoc;
    documentHandles_[newDoc] = docHandle;

    // Restore visibility
    if (wasVisible) {
        newDoc->Show();
    }

    spdlog::info("[RmlUISystem] Successfully reloaded document: {}", assetInfo.path.string());
}

void RmlUISystem::reloadStyleSheet(UIStyleSheetHandle styleHandle) {
    auto assetIt = styleSheetAssets_.find(styleHandle);
    if (assetIt == styleSheetAssets_.end()) {
        spdlog::warn("[RmlUISystem] Cannot reload stylesheet {}: no asset info", styleHandle);
        return;
    }

    const auto& assetInfo = assetIt->second;

    // Reload the asset from disk
    assetSystem_->reloadAsset(assetInfo.assetHandle);

    if (!assetSystem_->isLoaded(assetInfo.assetHandle)) {
        spdlog::error("[RmlUISystem] Failed to reload stylesheet: {}", assetInfo.path.string());
        return;
    }

    // Get the new content
    const auto* fileData = assetSystem_->getRawAsset(assetInfo.assetHandle);
    if (!fileData) {
        spdlog::error("[RmlUISystem] Failed to get reloaded stylesheet data");
        return;
    }

    const auto* content = static_cast<const std::string*>(fileData);

    // Update stored content
    styleSheetContents_[styleHandle] = *content;

    // Re-apply to all documents that use this stylesheet
    for (UIDocumentHandle docHandle : assetInfo.appliedTo) {
        auto* document = getDocument(docHandle);
        if (!document) continue;

        // Inject the updated stylesheet
        Rml::ElementPtr styleElem = document->CreateElement("style");
        if (styleElem) {
            styleElem->SetInnerRML(*content);

            // Find existing style elements and replace or add
            Rml::Element* head = document->GetElementById("head");
            if (!head) {
                head = document;
            }

            if (head->GetNumChildren() > 0) {
                head->InsertBefore(std::move(styleElem), head->GetFirstChild());
            } else {
                head->AppendChild(std::move(styleElem));
            }

            document->UpdateDocument();
        }
    }

    spdlog::info("[RmlUISystem] Successfully reloaded stylesheet: {} (applied to {} documents)",
                 assetInfo.path.string(), assetInfo.appliedTo.size());
}

#endif // BESTOW_HAS_RMLUI

//==========================================================================
// Kangaru Service Definitions
//==========================================================================

}  // namespace bestow
