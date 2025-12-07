// bestow-contract/src/bestow.ui.cppm
// UI System interface for menus, HUDs, and overlays

module;

#include <functional>
#include <optional>
#include <vector>
#include <filesystem>

export module bestow.ui;

import bestow.types;

export namespace bestow {

//==========================================================================
// Handle Types
//==========================================================================

using UIDocumentHandle = std::uint64_t;
using UIElementHandle = std::uint64_t;
using UIStyleSheetHandle = std::uint64_t;

//==========================================================================
// Error Types
//==========================================================================

enum class UIError {
    Success,
    DocumentNotFound,
    ElementNotFound,
    InvalidDocument,
    ParseError,
    StyleSheetError,
    FontNotFound,
    TextureNotFound,
    InternalError
};

//==========================================================================
// Input Event Types (for UI consumption)
//==========================================================================

enum class UIInputType : std::uint8_t {
    MouseMove,
    MouseDown,
    MouseUp,
    MouseScroll,
    KeyDown,
    KeyUp,
    TextInput
};

struct UIInputEvent {
    UIInputType type;
    int x = 0;
    int y = 0;
    int button = 0;      // Mouse button (0=left, 1=right, 2=middle)
    int wheelDelta = 0;
    int keyCode = 0;
    int modifiers = 0;   // Shift, Ctrl, Alt flags
    char32_t character = 0;
};

//==========================================================================
// Element Types
//==========================================================================

struct UIRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

enum class UIVisibility : std::uint8_t {
    Visible,
    Hidden,
    Collapsed  // Hidden and takes no space
};

//==========================================================================
// Event Callback Types
//==========================================================================

struct UIEventData {
    UIDocumentHandle document;
    UIElementHandle element;
    std::string eventType;  // "click", "hover", "focus", etc.
    std::string targetId;
    std::string targetClass;
    int mouseX = 0;
    int mouseY = 0;
};

using UIEventCallback = std::function<void(const UIEventData&)>;

//==========================================================================
// Data Binding Types
//==========================================================================

enum class UIDataType : std::uint8_t {
    Int,
    Float,
    Bool,
    String
};

struct UIDataBinding {
    std::string name;
    UIDataType type;
    void* dataPtr = nullptr;
};

//==========================================================================
// UI Configuration
//==========================================================================

struct UIConfig {
    float baseScale = 1.0f;              // UI scale factor
    bool enableDebugMode = false;        // Show element outlines
    std::filesystem::path assetsPath;    // Base path for UI assets
    std::filesystem::path fontsPath;     // Path to fonts directory
};

//==========================================================================
// IUISystem Interface
//==========================================================================

class IUISystem {
public:
    virtual ~IUISystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    /// Initialize the UI system with configuration
    virtual Result<void, UIError> initialize(const UIConfig& config) = 0;

    /// Shutdown and cleanup
    virtual void shutdown() = 0;

    //======================================================================
    // Document Management
    //======================================================================

    /// Load a UI document from file (.rml, .html, or .xml)
    virtual Result<UIDocumentHandle, UIError> loadDocument(
        const std::filesystem::path& path) = 0;

    /// Load a UI document from string content
    virtual Result<UIDocumentHandle, UIError> loadDocumentFromString(
        std::string_view content,
        const std::string& sourceName = "inline") = 0;

    /// Unload a document and free resources
    virtual void unloadDocument(UIDocumentHandle doc) = 0;

    /// Show a document (makes it visible and interactive)
    virtual void showDocument(UIDocumentHandle doc) = 0;

    /// Hide a document (invisible but still loaded)
    virtual void hideDocument(UIDocumentHandle doc) = 0;

    /// Check if a document is currently visible
    virtual bool isDocumentVisible(UIDocumentHandle doc) const = 0;

    /// Get all loaded documents
    virtual std::vector<UIDocumentHandle> getLoadedDocuments() const = 0;

    //======================================================================
    // Style Sheet Management
    //======================================================================

    /// Load a stylesheet from file (.rcss or .css)
    virtual Result<UIStyleSheetHandle, UIError> loadStyleSheet(
        const std::filesystem::path& path) = 0;

    /// Apply a stylesheet to a document
    virtual Result<void, UIError> applyStyleSheet(
        UIDocumentHandle doc,
        UIStyleSheetHandle styleSheet) = 0;

    //======================================================================
    // Element Access (DOM-like)
    //======================================================================

    /// Get element by ID within a document
    virtual std::optional<UIElementHandle> getElementById(
        UIDocumentHandle doc,
        const std::string& id) = 0;

    /// Get elements by class name
    virtual std::vector<UIElementHandle> getElementsByClass(
        UIDocumentHandle doc,
        const std::string& className) = 0;

    /// Get elements by tag name
    virtual std::vector<UIElementHandle> getElementsByTag(
        UIDocumentHandle doc,
        const std::string& tagName) = 0;

    /// Get child elements
    virtual std::vector<UIElementHandle> getChildren(UIElementHandle element) = 0;

    /// Get parent element
    virtual std::optional<UIElementHandle> getParent(UIElementHandle element) = 0;

    //======================================================================
    // Element Properties
    //======================================================================

    /// Set inner text content of an element
    virtual void setElementText(UIElementHandle elem, const std::string& text) = 0;

    /// Get inner text content
    virtual std::string getElementText(UIElementHandle elem) = 0;

    /// Set element visibility
    virtual void setElementVisible(UIElementHandle elem, UIVisibility visibility) = 0;

    /// Get element visibility
    virtual UIVisibility getElementVisibility(UIElementHandle elem) = 0;

    /// Add a class to an element
    virtual void addElementClass(UIElementHandle elem, const std::string& className) = 0;

    /// Remove a class from an element
    virtual void removeElementClass(UIElementHandle elem, const std::string& className) = 0;

    /// Check if element has a class
    virtual bool hasElementClass(UIElementHandle elem, const std::string& className) = 0;

    /// Set an attribute on an element
    virtual void setElementAttribute(
        UIElementHandle elem,
        const std::string& name,
        const std::string& value) = 0;

    /// Get an attribute from an element
    virtual std::optional<std::string> getElementAttribute(
        UIElementHandle elem,
        const std::string& name) = 0;

    /// Set inline style property
    virtual void setElementStyle(
        UIElementHandle elem,
        const std::string& property,
        const std::string& value) = 0;

    /// Get element bounding box (in screen coordinates)
    virtual UIRect getElementBounds(UIElementHandle elem) = 0;

    /// Focus an element (for keyboard input)
    virtual void focusElement(UIElementHandle elem) = 0;

    /// Blur (unfocus) an element
    virtual void blurElement(UIElementHandle elem) = 0;

    //======================================================================
    // Dynamic Element Creation
    //======================================================================

    /// Create a new element
    virtual UIElementHandle createElement(
        UIDocumentHandle doc,
        const std::string& tagName) = 0;

    /// Append a child element
    virtual void appendChild(UIElementHandle parent, UIElementHandle child) = 0;

    /// Remove an element from the document
    virtual void removeElement(UIElementHandle element) = 0;

    /// Set inner HTML/RML content
    virtual void setInnerRml(UIElementHandle elem, const std::string& rml) = 0;

    //======================================================================
    // Data Binding (Model-View synchronization)
    //======================================================================

    /// Bind an integer variable to the UI
    virtual void bindData(const std::string& name, int* value) = 0;

    /// Bind a float variable to the UI
    virtual void bindData(const std::string& name, float* value) = 0;

    /// Bind a bool variable to the UI
    virtual void bindData(const std::string& name, bool* value) = 0;

    /// Bind a string variable to the UI
    virtual void bindData(const std::string& name, std::string* value) = 0;

    /// Unbind a data variable
    virtual void unbindData(const std::string& name) = 0;

    /// Synchronize all bound data (call after modifying bound variables)
    virtual void syncBindings() = 0;

    //======================================================================
    // Event Handling
    //======================================================================

    /// Register a callback for a specific event type
    /// eventType: "click", "submit", "change", "focus", "blur", "mouseover", etc.
    virtual void registerEventCallback(
        const std::string& eventType,
        UIEventCallback callback) = 0;

    /// Register a callback for events on a specific element
    virtual void registerElementCallback(
        UIElementHandle elem,
        const std::string& eventType,
        UIEventCallback callback) = 0;

    /// Unregister all callbacks for an event type
    virtual void unregisterEventCallback(const std::string& eventType) = 0;

    //======================================================================
    // Input Processing
    //======================================================================

    /// Process an input event
    /// Returns true if the UI consumed the event (don't pass to game)
    virtual bool processInput(const UIInputEvent& event) = 0;

    /// Check if the UI wants keyboard focus
    virtual bool wantsKeyboardInput() const = 0;

    /// Check if the UI wants mouse input (mouse is over UI)
    virtual bool wantsMouseInput() const = 0;

    //======================================================================
    // Update and Render
    //======================================================================

    /// Update UI logic (animations, transitions, etc.)
    virtual void update(DeltaTime dt) = 0;

    /// Render all visible UI documents
    virtual void render() = 0;

    //======================================================================
    // Fonts
    //======================================================================

    /// Load a font family
    virtual Result<void, UIError> loadFont(
        const std::filesystem::path& path,
        const std::string& familyName = "") = 0;

    //======================================================================
    // Debug
    //======================================================================

    /// Enable/disable debug overlay (shows element bounds)
    virtual void setDebugMode(bool enabled) = 0;

    /// Get number of active elements
    virtual std::size_t getElementCount() const = 0;

    //======================================================================
    // Window Integration
    //======================================================================

    /// Set the viewport size (call when window resizes)
    virtual void setViewportSize(int width, int height) = 0;

    /// Set the pixels-per-dp ratio (for DPI scaling)
    virtual void setDPIScale(float scale) = 0;
};

}  // namespace bestow
