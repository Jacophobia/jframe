// bestow-luabind/src/docs/ui_binding_doc.cpp
// API documentation for bestow.ui

module bestow.luabind;

import std;

namespace bestow {

void registerUIDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "ui";
    sys.qualifiedName = "bestow.ui";
    sys.description = "UI system for menus, HUDs, and overlays. Provides DOM-like document management, element manipulation, event handling, data binding, and input processing.";

    //=========================================================================
    // Enums
    //=========================================================================

    sys.enums.push_back(EnumDoc{
        .name = "UIError",
        .qualifiedName = "UIError",
        .description = "Error codes returned by UI system operations.",
        .values = {
            {.name = "Success", .description = "Operation completed successfully"},
            {.name = "DocumentNotFound", .description = "The specified document handle is invalid or not loaded"},
            {.name = "ElementNotFound", .description = "The specified element handle is invalid or not found"},
            {.name = "InvalidDocument", .description = "The document content is malformed or invalid"},
            {.name = "ParseError", .description = "Failed to parse the document or stylesheet content"},
            {.name = "StyleSheetError", .description = "Error in stylesheet loading or application"},
            {.name = "FontNotFound", .description = "The specified font file could not be found"},
            {.name = "TextureNotFound", .description = "The specified texture file could not be found"},
            {.name = "InternalError", .description = "An unexpected internal error occurred"},
        },
    });

    sys.enums.push_back(EnumDoc{
        .name = "UIInputType",
        .qualifiedName = "UIInputType",
        .description = "Types of input events that can be sent to the UI system.",
        .values = {
            {.name = "MouseMove", .description = "Mouse cursor movement"},
            {.name = "MouseDown", .description = "Mouse button pressed"},
            {.name = "MouseUp", .description = "Mouse button released"},
            {.name = "MouseScroll", .description = "Mouse scroll wheel"},
            {.name = "KeyDown", .description = "Keyboard key pressed"},
            {.name = "KeyUp", .description = "Keyboard key released"},
            {.name = "TextInput", .description = "Text character input"},
        },
    });

    sys.enums.push_back(EnumDoc{
        .name = "UIVisibility",
        .qualifiedName = "UIVisibility",
        .description = "Visibility states for UI elements.",
        .values = {
            {.name = "Visible", .description = "Element is visible and takes up layout space"},
            {.name = "Hidden", .description = "Element is invisible but still takes up layout space"},
            {.name = "Collapsed", .description = "Element is invisible and takes no layout space"},
        },
    });

    sys.enums.push_back(EnumDoc{
        .name = "UIDataType",
        .qualifiedName = "UIDataType",
        .description = "Data types supported by the UI data binding system.",
        .values = {
            {.name = "Int", .description = "Integer value"},
            {.name = "Float", .description = "Floating-point value"},
            {.name = "Bool", .description = "Boolean value"},
            {.name = "String", .description = "String value"},
        },
    });

    //=========================================================================
    // Types
    //=========================================================================

    sys.types.push_back(TypeDoc{
        .name = "UIRect",
        .qualifiedName = "UIRect",
        .description = "Rectangle representing an element's bounding box in screen coordinates.",
        .fields = {
            {.name = "x", .type = "number", .description = "X position of the top-left corner"},
            {.name = "y", .type = "number", .description = "Y position of the top-left corner"},
            {.name = "width", .type = "number", .description = "Width of the rectangle"},
            {.name = "height", .type = "number", .description = "Height of the rectangle"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "UIConfig",
        .qualifiedName = "UIConfig",
        .description = "Configuration for initializing the UI system.",
        .fields = {
            {.name = "baseScale", .type = "number", .description = "UI scale factor (default: 1.0)"},
            {.name = "enableDebugMode", .type = "boolean", .description = "Show element outlines for debugging (default: false)"},
            {.name = "assetsPath", .type = "string", .description = "Base path for UI assets"},
            {.name = "fontsPath", .type = "string", .description = "Path to the fonts directory"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "UIInputEvent",
        .qualifiedName = "UIInputEvent",
        .description = "Input event for the UI system, wrapping mouse, keyboard, and text events.",
        .fields = {
            {.name = "type", .type = "UIInputType", .description = "The type of input event"},
            {.name = "x", .type = "number", .description = "Mouse X position"},
            {.name = "y", .type = "number", .description = "Mouse Y position"},
            {.name = "button", .type = "number", .description = "Mouse button (0=left, 1=right, 2=middle)"},
            {.name = "wheelDelta", .type = "number", .description = "Mouse scroll wheel delta"},
            {.name = "keyCode", .type = "number", .description = "Keyboard key code"},
            {.name = "modifiers", .type = "number", .description = "Modifier key flags (Shift, Ctrl, Alt)"},
            {.name = "character", .type = "string", .description = "Text input character"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "UIEventData",
        .qualifiedName = "UIEventData",
        .description = "Data passed to UI event callbacks, describing the event source and context.",
        .fields = {
            {.name = "document", .type = "UIDocumentHandle", .description = "Handle of the document containing the event source"},
            {.name = "element", .type = "UIElementHandle", .description = "Handle of the element that triggered the event"},
            {.name = "eventType", .type = "string", .description = "Event type name (e.g., \"click\", \"hover\", \"focus\")"},
            {.name = "targetId", .type = "string", .description = "ID attribute of the target element"},
            {.name = "targetClass", .type = "string", .description = "Class attribute of the target element"},
            {.name = "mouseX", .type = "number", .description = "Mouse X position at event time"},
            {.name = "mouseY", .type = "number", .description = "Mouse Y position at event time"},
        },
    });

    //=========================================================================
    // Lifecycle
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "initialize",
        .qualifiedName = "bestow.ui.initialize",
        .description = "Initialize the UI system with optional configuration. Must be called before any other UI operations.",
        .params = {
            {.name = "config", .type = "table", .description = "Configuration table with optional fields: baseScale, enableDebugMode, assetsPath, fontsPath", .optional = true},
        },
        .returns = {{.type = "boolean", .description = "true if initialization succeeded"}},
        .example = "bestow.ui.initialize({\n    baseScale = 1.0,\n    enableDebugMode = false,\n    assetsPath = \"assets/ui/\",\n    fontsPath = \"assets/fonts/\"\n})",
    });

    sys.methods.push_back(MethodDoc{
        .name = "shutdown",
        .qualifiedName = "bestow.ui.shutdown",
        .description = "Shut down the UI system and release all resources. Call during application cleanup.",
    });

    //=========================================================================
    // Document Management
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "loadDocument",
        .qualifiedName = "bestow.ui.loadDocument",
        .description = "Load a UI document from a file (.rml, .html, or .xml). The document is loaded but not shown until showDocument() is called.",
        .params = {
            {.name = "path", .type = "string", .description = "Path to the document file"},
        },
        .returns = {{.type = "UIDocumentHandle|nil", .description = "Document handle, or nil on failure"}},
        .example = "local doc = bestow.ui.loadDocument(\"ui/main_menu.rml\")\nif doc then\n    bestow.ui.showDocument(doc)\nend",
        .seeAlso = {"bestow.ui.loadDocumentFromString", "bestow.ui.showDocument"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "loadDocumentFromString",
        .qualifiedName = "bestow.ui.loadDocumentFromString",
        .description = "Load a UI document from a string of markup content.",
        .params = {
            {.name = "content", .type = "string", .description = "The RML/HTML markup content"},
            {.name = "sourceName", .type = "string", .description = "Name for error reporting (default: \"inline\")", .optional = true, .defaultVal = "\"inline\""},
        },
        .returns = {{.type = "UIDocumentHandle|nil", .description = "Document handle, or nil on failure"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "unloadDocument",
        .qualifiedName = "bestow.ui.unloadDocument",
        .description = "Unload a document and free its resources. The handle becomes invalid after this call.",
        .params = {
            {.name = "doc", .type = "UIDocumentHandle", .description = "Handle of the document to unload"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "showDocument",
        .qualifiedName = "bestow.ui.showDocument",
        .description = "Show a loaded document, making it visible and interactive.",
        .params = {
            {.name = "doc", .type = "UIDocumentHandle", .description = "Handle of the document to show"},
        },
        .seeAlso = {"bestow.ui.hideDocument", "bestow.ui.isDocumentVisible"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "hideDocument",
        .qualifiedName = "bestow.ui.hideDocument",
        .description = "Hide a document. The document remains loaded but is invisible and non-interactive.",
        .params = {
            {.name = "doc", .type = "UIDocumentHandle", .description = "Handle of the document to hide"},
        },
        .seeAlso = {"bestow.ui.showDocument"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isDocumentVisible",
        .qualifiedName = "bestow.ui.isDocumentVisible",
        .description = "Check if a document is currently visible.",
        .params = {
            {.name = "doc", .type = "UIDocumentHandle", .description = "Handle of the document to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the document is visible"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getLoadedDocuments",
        .qualifiedName = "bestow.ui.getLoadedDocuments",
        .description = "Get handles for all currently loaded documents.",
        .returns = {{.type = "UIDocumentHandle[]", .description = "Array of loaded document handles"}},
    });

    //=========================================================================
    // StyleSheet Management
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "loadStyleSheet",
        .qualifiedName = "bestow.ui.loadStyleSheet",
        .description = "Load a stylesheet from a file (.rcss or .css).",
        .params = {
            {.name = "path", .type = "string", .description = "Path to the stylesheet file"},
        },
        .returns = {{.type = "UIStyleSheetHandle|nil", .description = "Stylesheet handle, or nil on failure"}},
        .seeAlso = {"bestow.ui.applyStyleSheet"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "applyStyleSheet",
        .qualifiedName = "bestow.ui.applyStyleSheet",
        .description = "Apply a loaded stylesheet to a document.",
        .params = {
            {.name = "doc", .type = "UIDocumentHandle", .description = "Document to apply the stylesheet to"},
            {.name = "style", .type = "UIStyleSheetHandle", .description = "Stylesheet to apply"},
        },
        .returns = {{.type = "boolean", .description = "true if the stylesheet was applied successfully"}},
    });

    //=========================================================================
    // Element Access
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "getElementById",
        .qualifiedName = "bestow.ui.getElementById",
        .description = "Find an element by its ID attribute within a document.",
        .params = {
            {.name = "doc", .type = "UIDocumentHandle", .description = "Document to search in"},
            {.name = "id", .type = "string", .description = "Element ID to find"},
        },
        .returns = {{.type = "UIElementHandle|nil", .description = "Element handle, or nil if not found"}},
        .example = "local healthBar = bestow.ui.getElementById(hudDoc, \"health-bar\")\nif healthBar then\n    bestow.ui.setElementText(healthBar, tostring(health))\nend",
    });

    sys.methods.push_back(MethodDoc{
        .name = "getElementsByClass",
        .qualifiedName = "bestow.ui.getElementsByClass",
        .description = "Find all elements with a given class name within a document.",
        .params = {
            {.name = "doc", .type = "UIDocumentHandle", .description = "Document to search in"},
            {.name = "className", .type = "string", .description = "Class name to search for"},
        },
        .returns = {{.type = "UIElementHandle[]", .description = "Array of matching element handles"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getElementsByTag",
        .qualifiedName = "bestow.ui.getElementsByTag",
        .description = "Find all elements with a given tag name within a document.",
        .params = {
            {.name = "doc", .type = "UIDocumentHandle", .description = "Document to search in"},
            {.name = "tagName", .type = "string", .description = "Tag name to search for (e.g., \"div\", \"button\")"},
        },
        .returns = {{.type = "UIElementHandle[]", .description = "Array of matching element handles"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getChildren",
        .qualifiedName = "bestow.ui.getChildren",
        .description = "Get all child elements of a given element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Parent element handle"},
        },
        .returns = {{.type = "UIElementHandle[]", .description = "Array of child element handles"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getParent",
        .qualifiedName = "bestow.ui.getParent",
        .description = "Get the parent element of a given element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Child element handle"},
        },
        .returns = {{.type = "UIElementHandle|nil", .description = "Parent element handle, or nil if the element is the root"}},
    });

    //=========================================================================
    // Element Properties
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "setElementText",
        .qualifiedName = "bestow.ui.setElementText",
        .description = "Set the inner text content of an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to modify"},
            {.name = "text", .type = "string", .description = "New text content"},
        },
        .seeAlso = {"bestow.ui.getElementText"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getElementText",
        .qualifiedName = "bestow.ui.getElementText",
        .description = "Get the inner text content of an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to query"},
        },
        .returns = {{.type = "string", .description = "The element's text content"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setElementVisible",
        .qualifiedName = "bestow.ui.setElementVisible",
        .description = "Set the visibility of an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to modify"},
            {.name = "visibility", .type = "UIVisibility", .description = "New visibility state"},
        },
        .example = "bestow.ui.setElementVisible(panel, UIVisibility.Hidden)",
        .seeAlso = {"bestow.ui.getElementVisibility"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getElementVisibility",
        .qualifiedName = "bestow.ui.getElementVisibility",
        .description = "Get the visibility state of an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to query"},
        },
        .returns = {{.type = "UIVisibility", .description = "Current visibility state"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "addClass",
        .qualifiedName = "bestow.ui.addClass",
        .description = "Add a CSS class to an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to modify"},
            {.name = "className", .type = "string", .description = "Class name to add"},
        },
        .seeAlso = {"bestow.ui.removeClass", "bestow.ui.hasClass"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeClass",
        .qualifiedName = "bestow.ui.removeClass",
        .description = "Remove a CSS class from an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to modify"},
            {.name = "className", .type = "string", .description = "Class name to remove"},
        },
        .seeAlso = {"bestow.ui.addClass", "bestow.ui.hasClass"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasClass",
        .qualifiedName = "bestow.ui.hasClass",
        .description = "Check if an element has a specific CSS class.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to query"},
            {.name = "className", .type = "string", .description = "Class name to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the element has the class"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setAttribute",
        .qualifiedName = "bestow.ui.setAttribute",
        .description = "Set an attribute on an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to modify"},
            {.name = "name", .type = "string", .description = "Attribute name"},
            {.name = "value", .type = "string", .description = "Attribute value"},
        },
        .seeAlso = {"bestow.ui.getAttribute"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAttribute",
        .qualifiedName = "bestow.ui.getAttribute",
        .description = "Get an attribute value from an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to query"},
            {.name = "name", .type = "string", .description = "Attribute name"},
        },
        .returns = {{.type = "string|nil", .description = "Attribute value, or nil if not set"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setStyle",
        .qualifiedName = "bestow.ui.setStyle",
        .description = "Set an inline style property on an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to modify"},
            {.name = "property", .type = "string", .description = "CSS property name (e.g., \"color\", \"width\")"},
            {.name = "value", .type = "string", .description = "CSS property value (e.g., \"red\", \"100px\")"},
        },
        .example = "bestow.ui.setStyle(elem, \"background-color\", \"#ff0000\")\nbestow.ui.setStyle(elem, \"width\", \"200px\")",
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBounds",
        .qualifiedName = "bestow.ui.getBounds",
        .description = "Get the bounding box of an element in screen coordinates.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to query"},
        },
        .returns = {{.type = "UIRect", .description = "Bounding rectangle with x, y, width, height fields"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "focus",
        .qualifiedName = "bestow.ui.focus",
        .description = "Give keyboard focus to an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to focus"},
        },
        .seeAlso = {"bestow.ui.blur"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "blur",
        .qualifiedName = "bestow.ui.blur",
        .description = "Remove keyboard focus from an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to unfocus"},
        },
        .seeAlso = {"bestow.ui.focus"},
    });

    //=========================================================================
    // Dynamic Element Creation
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "createElement",
        .qualifiedName = "bestow.ui.createElement",
        .description = "Create a new element within a document.",
        .params = {
            {.name = "doc", .type = "UIDocumentHandle", .description = "Document to create the element in"},
            {.name = "tagName", .type = "string", .description = "Tag name for the new element (e.g., \"div\", \"span\", \"button\")"},
        },
        .returns = {{.type = "UIElementHandle", .description = "Handle of the newly created element"}},
        .seeAlso = {"bestow.ui.appendChild", "bestow.ui.removeElement"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "appendChild",
        .qualifiedName = "bestow.ui.appendChild",
        .description = "Append a child element to a parent element.",
        .params = {
            {.name = "parent", .type = "UIElementHandle", .description = "Parent element handle"},
            {.name = "child", .type = "UIElementHandle", .description = "Child element handle to append"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeElement",
        .qualifiedName = "bestow.ui.removeElement",
        .description = "Remove an element from the document. The handle becomes invalid after this call.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to remove"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setInnerContent",
        .qualifiedName = "bestow.ui.setInnerContent",
        .description = "Set the inner HTML/RML content of an element, replacing all children.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to modify"},
            {.name = "content", .type = "string", .description = "RML/HTML markup string"},
        },
        .example = "bestow.ui.setInnerContent(container, \"<div class='item'>New Item</div>\")",
        .seeAlso = {"bestow.ui.setInnerRml"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setInnerRml",
        .qualifiedName = "bestow.ui.setInnerRml",
        .description = "Alias for setInnerContent. Set the inner HTML/RML content of an element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to modify"},
            {.name = "content", .type = "string", .description = "RML/HTML markup string"},
        },
        .seeAlso = {"bestow.ui.setInnerContent"},
    });

    //=========================================================================
    // Data Binding
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "syncBindings",
        .qualifiedName = "bestow.ui.syncBindings",
        .description = "Synchronize all data bindings, updating UI elements to reflect current bound values. Call after modifying any bound data variables.",
    });

    //=========================================================================
    // Event Handling
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "onEvent",
        .qualifiedName = "bestow.ui.onEvent",
        .description = "Register a callback for a specific UI event type. The callback fires for any element that triggers this event type.",
        .params = {
            {.name = "eventType", .type = "string", .description = "Event type (e.g., \"click\", \"submit\", \"change\", \"focus\", \"blur\", \"mouseover\")"},
            {.name = "callback", .type = "function", .description = "Callback function(eventData) receiving a UIEventData table"},
        },
        .example = "bestow.ui.onEvent(\"click\", function(data)\n    print(\"Clicked element:\", data.targetId)\nend)",
        .seeAlso = {"bestow.ui.onElementEvent", "bestow.ui.offEvent"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "onElementEvent",
        .qualifiedName = "bestow.ui.onElementEvent",
        .description = "Register a callback for a specific event type on a specific element.",
        .params = {
            {.name = "elem", .type = "UIElementHandle", .description = "Element to listen on"},
            {.name = "eventType", .type = "string", .description = "Event type (e.g., \"click\", \"mouseover\")"},
            {.name = "callback", .type = "function", .description = "Callback function(eventData) receiving a UIEventData table"},
        },
        .example = "local btn = bestow.ui.getElementById(doc, \"start-button\")\nbestow.ui.onElementEvent(btn, \"click\", function(data)\n    bestow.gamestate.pushState(\"Playing\")\nend)",
        .seeAlso = {"bestow.ui.onEvent"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "offEvent",
        .qualifiedName = "bestow.ui.offEvent",
        .description = "Unregister all callbacks for a specific event type.",
        .params = {
            {.name = "eventType", .type = "string", .description = "Event type to unregister"},
        },
        .seeAlso = {"bestow.ui.onEvent"},
    });

    //=========================================================================
    // Input Processing
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "processInput",
        .qualifiedName = "bestow.ui.processInput",
        .description = "Process an input event through the UI system. Returns true if the UI consumed the event (meaning it should not be passed to the game).",
        .params = {
            {.name = "event", .type = "table", .description = "Input event table with fields: type (UIInputType), x, y, button, wheelDelta, keyCode, modifiers, character"},
        },
        .returns = {{.type = "boolean", .description = "true if the UI consumed the event"}},
        .example = "local consumed = bestow.ui.processInput({\n    type = UIInputType.MouseDown,\n    x = mouseX,\n    y = mouseY,\n    button = 0\n})",
    });

    sys.methods.push_back(MethodDoc{
        .name = "wantsKeyboardInput",
        .qualifiedName = "bestow.ui.wantsKeyboardInput",
        .description = "Check if the UI system wants keyboard input (e.g., a text field is focused). When true, keyboard events should be routed to the UI instead of the game.",
        .returns = {{.type = "boolean", .description = "true if the UI wants keyboard input"}},
        .seeAlso = {"bestow.ui.wantsMouseInput"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wantsMouseInput",
        .qualifiedName = "bestow.ui.wantsMouseInput",
        .description = "Check if the UI system wants mouse input (e.g., mouse is hovering over a UI element). When true, mouse events should be routed to the UI instead of the game.",
        .returns = {{.type = "boolean", .description = "true if the UI wants mouse input"}},
        .seeAlso = {"bestow.ui.wantsKeyboardInput"},
    });

    //=========================================================================
    // Update and Render
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "update",
        .qualifiedName = "bestow.ui.update",
        .description = "Update UI logic including animations, transitions, and data bindings. Call once per frame.",
        .params = {
            {.name = "dt", .type = "number", .description = "Delta time in seconds since the last frame"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "render",
        .qualifiedName = "bestow.ui.render",
        .description = "Render all visible UI documents. Call once per frame after update, typically as the last render step.",
    });

    //=========================================================================
    // Fonts
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "loadFont",
        .qualifiedName = "bestow.ui.loadFont",
        .description = "Load a font family from a font file (.ttf, .otf).",
        .params = {
            {.name = "path", .type = "string", .description = "Path to the font file"},
            {.name = "familyName", .type = "string", .description = "Font family name override (uses file name if empty)", .optional = true},
        },
        .returns = {{.type = "boolean", .description = "true if the font was loaded successfully"}},
        .example = "bestow.ui.loadFont(\"assets/fonts/Roboto-Regular.ttf\", \"Roboto\")",
    });

    //=========================================================================
    // Debug
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "setDebugMode",
        .qualifiedName = "bestow.ui.setDebugMode",
        .description = "Enable or disable the debug overlay that shows element outlines and bounds.",
        .params = {
            {.name = "enabled", .type = "boolean", .description = "true to enable debug mode"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getElementCount",
        .qualifiedName = "bestow.ui.getElementCount",
        .description = "Get the total number of active UI elements across all documents.",
        .returns = {{.type = "number", .description = "Total number of active elements"}},
    });

    //=========================================================================
    // Window/Viewport
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "setViewportSize",
        .qualifiedName = "bestow.ui.setViewportSize",
        .description = "Set the viewport size for UI rendering. Call when the window is resized.",
        .params = {
            {.name = "width", .type = "number", .description = "Viewport width in pixels"},
            {.name = "height", .type = "number", .description = "Viewport height in pixels"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setDPIScale",
        .qualifiedName = "bestow.ui.setDPIScale",
        .description = "Set the DPI scale factor for high-DPI displays.",
        .params = {
            {.name = "scale", .type = "number", .description = "DPI scale factor (e.g., 2.0 for Retina displays)"},
        },
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
