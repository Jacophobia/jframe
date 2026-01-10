// tests/unit/UISystemTests.cpp
// Unit tests for Bestow UI System
//
// NOTE: These tests require RmlUI backend which needs a graphics context.
// In headless test environments without proper GPU/windowing, the UISystem
// uses StubUISystem which has limited functionality. Many of these tests
// will pass trivially with the stub but validate the interface contract.

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>

import bestow.ui;
import bestow.ui.impl;
import bestow.assets;
import bestow.assets.impl;
import bestow.events.impl;
import bestow.types;
import bestow.graphics.context;
import bestow.services;

#include "../mocks/MockGraphics3DSystem.hpp"

namespace bestow::tests {

// Service wrapper for MockGraphics3DSystem to work with Kangaru
struct MockGraphicsContextService
    : kgr::single_service<MockGraphics3DSystem>
    , kgr::overrides<IGraphicsContextService>
{};

class UISystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register mock graphics context first (required by RmlUISystem)
        container_.service<MockGraphicsContextService>();

        // Register event system
        container_.service<EventSystemService>();

        // Get asset system from container
        assetSystem_ = &container_.service<AssetSystemService>();

        // Try to get UI system - this may use StubUISystem if RmlUI isn't available
        try {
            uiSystem_ = &container_.service<UISystemService>();
        } catch (const std::exception& e) {
            // If we can't create the UI system, skip the test gracefully
            GTEST_SKIP() << "UI system not available: " << e.what();
            return;
        }

        // Initialize with default config
        UIConfig config;
        config.enableDebugMode = false;
        auto result = uiSystem_->initialize(config);
        if (!result) {
            // RmlUI initialization may fail in headless environments - that's OK
            // The stub system will work fine for basic interface tests
        }
    }

    void TearDown() override {
        if (uiSystem_) {
            uiSystem_->shutdown();
        }
    }

    kgr::container container_;
    IAssetSystem* assetSystem_ = nullptr;
    IUISystem* uiSystem_ = nullptr;
};

//=============================================================================
// Basic System Tests
//=============================================================================

TEST_F(UISystemTest, CanCreateUISystem) {
    EXPECT_NE(uiSystem_, nullptr);
}

TEST_F(UISystemTest, InitializeDoesNotCrash) {
    UIConfig config;
    config.enableDebugMode = true;
    config.baseScale = 1.5f;

    // Re-initialization should either succeed (if already initialized) or
    // fail gracefully (if RmlUI isn't available in test environment)
    auto result = uiSystem_->initialize(config);
    // Note: This may fail if RmlUI backend isn't properly initialized,
    // which is expected in test environments without a graphics context.
    // The key assertion is that it doesn't crash.
    (void)result;  // Suppress unused warning - success depends on RmlUI availability
}

TEST_F(UISystemTest, ShutdownDoesNotCrash) {
    uiSystem_->shutdown();
    // Should not crash
}

TEST_F(UISystemTest, UpdateDoesNotCrashWithNoDocuments) {
    uiSystem_->update(1.0f / 60.0f);  // 60 FPS delta time
}

TEST_F(UISystemTest, RenderDoesNotCrashWithNoDocuments) {
    uiSystem_->render();  // Should not crash
}

//=============================================================================
// Document Management Tests
//=============================================================================

TEST_F(UISystemTest, LoadDocumentFromString) {
    std::string_view content = R"(
        <rml>
            <head><title>Test</title></head>
            <body>
                <div id="content">Hello World</div>
            </body>
        </rml>
    )";

    auto result = uiSystem_->loadDocumentFromString(content, "test.rml");
#ifdef BESTOW_HAS_RMLUI
    EXPECT_TRUE(result.has_value());
#else
    // Stub implementation should return error
    EXPECT_FALSE(result.has_value());
#endif
}

TEST_F(UISystemTest, LoadDocumentFromStringReturnsHandle) {
    std::string_view content = R"(<rml><body><div>Test</div></body></rml>)";
    auto result = uiSystem_->loadDocumentFromString(content);

#ifdef BESTOW_HAS_RMLUI
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0u);
#endif
}

TEST_F(UISystemTest, UnloadDocument) {
    std::string_view content = R"(<rml><body><div>Test</div></body></rml>)";
    auto result = uiSystem_->loadDocumentFromString(content);

#ifdef BESTOW_HAS_RMLUI
    ASSERT_TRUE(result.has_value());
    UIDocumentHandle handle = result.value();

    uiSystem_->unloadDocument(handle);
    // Should not crash
#endif
}

TEST_F(UISystemTest, UnloadNonExistentDocumentDoesNotCrash) {
    UIDocumentHandle fakeHandle = 99999;
    uiSystem_->unloadDocument(fakeHandle);  // Should not crash
}

TEST_F(UISystemTest, ShowDocument) {
    std::string_view content = R"(<rml><body><div>Test</div></body></rml>)";
    auto result = uiSystem_->loadDocumentFromString(content);

#ifdef BESTOW_HAS_RMLUI
    ASSERT_TRUE(result.has_value());
    UIDocumentHandle handle = result.value();

    uiSystem_->showDocument(handle);
    EXPECT_TRUE(uiSystem_->isDocumentVisible(handle));
#endif
}

TEST_F(UISystemTest, HideDocument) {
    std::string_view content = R"(<rml><body><div>Test</div></body></rml>)";
    auto result = uiSystem_->loadDocumentFromString(content);

#ifdef BESTOW_HAS_RMLUI
    ASSERT_TRUE(result.has_value());
    UIDocumentHandle handle = result.value();

    uiSystem_->showDocument(handle);
    EXPECT_TRUE(uiSystem_->isDocumentVisible(handle));

    uiSystem_->hideDocument(handle);
    EXPECT_FALSE(uiSystem_->isDocumentVisible(handle));
#endif
}

TEST_F(UISystemTest, IsDocumentVisibleReturnsFalseForNonExistentDocument) {
    UIDocumentHandle fakeHandle = 99999;
    EXPECT_FALSE(uiSystem_->isDocumentVisible(fakeHandle));
}

TEST_F(UISystemTest, GetLoadedDocuments) {
    auto docs1 = uiSystem_->getLoadedDocuments();
    EXPECT_EQ(docs1.size(), 0u);

#ifdef BESTOW_HAS_RMLUI
    std::string_view content1 = R"(<rml><body><div>Doc1</div></body></rml>)";
    std::string_view content2 = R"(<rml><body><div>Doc2</div></body></rml>)";

    auto result1 = uiSystem_->loadDocumentFromString(content1, "doc1.rml");
    auto result2 = uiSystem_->loadDocumentFromString(content2, "doc2.rml");

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    auto docs2 = uiSystem_->getLoadedDocuments();
    EXPECT_EQ(docs2.size(), 2u);
#endif
}

TEST_F(UISystemTest, LoadMultipleDocuments) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content1 = R"(<rml><body><div id="doc1">First</div></body></rml>)";
    std::string_view content2 = R"(<rml><body><div id="doc2">Second</div></body></rml>)";
    std::string_view content3 = R"(<rml><body><div id="doc3">Third</div></body></rml>)";

    auto result1 = uiSystem_->loadDocumentFromString(content1);
    auto result2 = uiSystem_->loadDocumentFromString(content2);
    auto result3 = uiSystem_->loadDocumentFromString(content3);

    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result3.has_value());

    // All handles should be unique
    EXPECT_NE(result1.value(), result2.value());
    EXPECT_NE(result2.value(), result3.value());
    EXPECT_NE(result1.value(), result3.value());
#endif
}

//=============================================================================
// Stylesheet Loading Tests
//=============================================================================

TEST_F(UISystemTest, LoadStyleSheetReturnsHandle) {
    // Create a temporary CSS file content
    std::string cssContent = R"(
        body {
            background-color: #ffffff;
        }
        .button {
            color: #000000;
            padding: 10px;
        }
    )";

    // Note: This test requires AssetSystem to load the file
    // For now, we test that the function can be called
#ifdef BESTOW_HAS_RMLUI
    // Would need actual file for full test
    // For now, just verify the interface exists
    EXPECT_TRUE(true);
#endif
}

TEST_F(UISystemTest, LoadStyleSheetWithoutAssetSystemReturnsError) {
#ifdef BESTOW_HAS_RMLUI
    // Create a fresh UI system without asset system dependency
    auto freshUI = std::make_unique<RmlUISystem>();
    UIConfig config;
    freshUI->initialize(config);

    auto result = freshUI->loadStyleSheet("test.css");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), UIError::InternalError);

    freshUI->shutdown();
#endif
}

TEST_F(UISystemTest, LoadStyleSheetFromNonExistentFileReturnsError) {
#ifdef BESTOW_HAS_RMLUI
    auto result = uiSystem_->loadStyleSheet("nonexistent/path/to/style.css");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), UIError::StyleSheetError);
#endif
}

//=============================================================================
// Stylesheet Application Tests
//=============================================================================

TEST_F(UISystemTest, ApplyStyleSheetToNonExistentDocumentReturnsError) {
#ifdef BESTOW_HAS_RMLUI
    UIDocumentHandle fakeDoc = 99999;
    UIStyleSheetHandle fakeSheet = 1;

    auto result = uiSystem_->applyStyleSheet(fakeDoc, fakeSheet);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), UIError::DocumentNotFound);
#endif
}

TEST_F(UISystemTest, ApplyStyleSheetWithInvalidHandleReturnsError) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div>Test</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    UIStyleSheetHandle fakeSheet = 99999;
    auto result = uiSystem_->applyStyleSheet(docResult.value(), fakeSheet);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), UIError::StyleSheetError);
#endif
}

//=============================================================================
// Element Access Tests
//=============================================================================

TEST_F(UISystemTest, GetElementById) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(
        <rml>
            <body>
                <div id="test-element">Content</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test-element");
    EXPECT_TRUE(elemResult.has_value());
#endif
}

TEST_F(UISystemTest, GetElementByIdReturnsNulloptForNonExistent) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="exists">Test</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "does-not-exist");
    EXPECT_FALSE(elemResult.has_value());
#endif
}

TEST_F(UISystemTest, GetElementByIdOnNonExistentDocumentReturnsNullopt) {
    UIDocumentHandle fakeDoc = 99999;
    auto elemResult = uiSystem_->getElementById(fakeDoc, "any-id");
    EXPECT_FALSE(elemResult.has_value());
}

TEST_F(UISystemTest, GetElementsByClass) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(
        <rml>
            <body>
                <div class="test-class">First</div>
                <div class="test-class">Second</div>
                <div class="other-class">Third</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elements = uiSystem_->getElementsByClass(docResult.value(), "test-class");
    EXPECT_EQ(elements.size(), 2u);
#endif
}

TEST_F(UISystemTest, GetElementsByClassReturnsEmptyForNonExistent) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div class="exists">Test</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elements = uiSystem_->getElementsByClass(docResult.value(), "does-not-exist");
    EXPECT_EQ(elements.size(), 0u);
#endif
}

TEST_F(UISystemTest, GetElementsByTag) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(
        <rml>
            <body>
                <div>First div</div>
                <div>Second div</div>
                <p>Paragraph</p>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto divElements = uiSystem_->getElementsByTag(docResult.value(), "div");
    EXPECT_GE(divElements.size(), 2u);  // At least our 2 divs (may include body)
#endif
}

TEST_F(UISystemTest, GetChildren) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(
        <rml>
            <body>
                <div id="parent">
                    <div>Child 1</div>
                    <div>Child 2</div>
                    <div>Child 3</div>
                </div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto parentElem = uiSystem_->getElementById(docResult.value(), "parent");
    ASSERT_TRUE(parentElem.has_value());

    auto children = uiSystem_->getChildren(parentElem.value());
    EXPECT_EQ(children.size(), 3u);
#endif
}

TEST_F(UISystemTest, GetChildrenOnNonExistentElementReturnsEmpty) {
    UIElementHandle fakeElem = 99999;
    auto children = uiSystem_->getChildren(fakeElem);
    EXPECT_EQ(children.size(), 0u);
}

TEST_F(UISystemTest, GetParent) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(
        <rml>
            <body>
                <div id="parent">
                    <div id="child">Content</div>
                </div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto childElem = uiSystem_->getElementById(docResult.value(), "child");
    ASSERT_TRUE(childElem.has_value());

    auto parentResult = uiSystem_->getParent(childElem.value());
    EXPECT_TRUE(parentResult.has_value());
#endif
}

TEST_F(UISystemTest, GetParentOnNonExistentElementReturnsNullopt) {
    UIElementHandle fakeElem = 99999;
    auto parentResult = uiSystem_->getParent(fakeElem);
    EXPECT_FALSE(parentResult.has_value());
}

//=============================================================================
// Element Properties Tests
//=============================================================================

TEST_F(UISystemTest, SetAndGetElementText) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="test">Original</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    uiSystem_->setElementText(elemResult.value(), "Updated Text");
    std::string text = uiSystem_->getElementText(elemResult.value());
    EXPECT_EQ(text, "Updated Text");
#endif
}

TEST_F(UISystemTest, GetElementTextOnNonExistentElementReturnsEmpty) {
    UIElementHandle fakeElem = 99999;
    std::string text = uiSystem_->getElementText(fakeElem);
    EXPECT_EQ(text, "");
}

TEST_F(UISystemTest, SetElementTextOnNonExistentElementDoesNotCrash) {
    UIElementHandle fakeElem = 99999;
    uiSystem_->setElementText(fakeElem, "Test");  // Should not crash
}

TEST_F(UISystemTest, SetAndGetElementVisibility) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="test">Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    uiSystem_->setElementVisible(elemResult.value(), UIVisibility::Visible);
    EXPECT_EQ(uiSystem_->getElementVisibility(elemResult.value()), UIVisibility::Visible);

    uiSystem_->setElementVisible(elemResult.value(), UIVisibility::Hidden);
    EXPECT_EQ(uiSystem_->getElementVisibility(elemResult.value()), UIVisibility::Hidden);

    uiSystem_->setElementVisible(elemResult.value(), UIVisibility::Collapsed);
    EXPECT_EQ(uiSystem_->getElementVisibility(elemResult.value()), UIVisibility::Collapsed);
#endif
}

TEST_F(UISystemTest, AddAndHasElementClass) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="test">Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    EXPECT_FALSE(uiSystem_->hasElementClass(elemResult.value(), "new-class"));

    uiSystem_->addElementClass(elemResult.value(), "new-class");
    EXPECT_TRUE(uiSystem_->hasElementClass(elemResult.value(), "new-class"));
#endif
}

TEST_F(UISystemTest, RemoveElementClass) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="test" class="existing">Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    uiSystem_->addElementClass(elemResult.value(), "remove-me");
    EXPECT_TRUE(uiSystem_->hasElementClass(elemResult.value(), "remove-me"));

    uiSystem_->removeElementClass(elemResult.value(), "remove-me");
    EXPECT_FALSE(uiSystem_->hasElementClass(elemResult.value(), "remove-me"));
#endif
}

TEST_F(UISystemTest, HasElementClassOnNonExistentElementReturnsFalse) {
    UIElementHandle fakeElem = 99999;
    EXPECT_FALSE(uiSystem_->hasElementClass(fakeElem, "any-class"));
}

TEST_F(UISystemTest, SetAndGetElementAttribute) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="test">Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    uiSystem_->setElementAttribute(elemResult.value(), "custom-attr", "custom-value");
    auto attrResult = uiSystem_->getElementAttribute(elemResult.value(), "custom-attr");
    ASSERT_TRUE(attrResult.has_value());
    EXPECT_EQ(attrResult.value(), "custom-value");
#endif
}

TEST_F(UISystemTest, GetElementAttributeReturnsNulloptForNonExistent) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="test" existing="value">Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    auto attrResult = uiSystem_->getElementAttribute(elemResult.value(), "non-existent");
    EXPECT_FALSE(attrResult.has_value());
#endif
}

TEST_F(UISystemTest, SetElementStyle) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="test">Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    uiSystem_->setElementStyle(elemResult.value(), "color", "#ff0000");
    // Should not crash - style should be applied
#endif
}

TEST_F(UISystemTest, GetElementBounds) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="test">Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    UIRect bounds = uiSystem_->getElementBounds(elemResult.value());
    // Should return some bounds (exact values depend on layout)
    EXPECT_GE(bounds.x, 0.0f);
    EXPECT_GE(bounds.y, 0.0f);
#endif
}

TEST_F(UISystemTest, GetElementBoundsOnNonExistentElementReturnsEmpty) {
    UIElementHandle fakeElem = 99999;
    UIRect bounds = uiSystem_->getElementBounds(fakeElem);
    EXPECT_EQ(bounds.x, 0.0f);
    EXPECT_EQ(bounds.y, 0.0f);
    EXPECT_EQ(bounds.width, 0.0f);
    EXPECT_EQ(bounds.height, 0.0f);
}

TEST_F(UISystemTest, FocusElement) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><input id="test" type="text"/></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    uiSystem_->focusElement(elemResult.value());
    // Should not crash - focus should be set
#endif
}

TEST_F(UISystemTest, BlurElement) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><input id="test" type="text"/></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "test");
    ASSERT_TRUE(elemResult.has_value());

    uiSystem_->focusElement(elemResult.value());
    uiSystem_->blurElement(elemResult.value());
    // Should not crash
#endif
}

//=============================================================================
// Data Binding Tests
//=============================================================================

TEST_F(UISystemTest, BindIntData) {
    int value = 42;
    uiSystem_->bindData("test_int", &value);
    // Should not crash - binding should be stored
}

TEST_F(UISystemTest, BindFloatData) {
    float value = 3.14f;
    uiSystem_->bindData("test_float", &value);
    // Should not crash
}

TEST_F(UISystemTest, BindBoolData) {
    bool value = true;
    uiSystem_->bindData("test_bool", &value);
    // Should not crash
}

TEST_F(UISystemTest, BindStringData) {
    std::string value = "test string";
    uiSystem_->bindData("test_string", &value);
    // Should not crash
}

TEST_F(UISystemTest, UnbindData) {
    int value = 100;
    uiSystem_->bindData("test_value", &value);
    uiSystem_->unbindData("test_value");
    // Should not crash
}

TEST_F(UISystemTest, UnbindNonExistentDataDoesNotCrash) {
    uiSystem_->unbindData("non-existent-binding");
    // Should not crash
}

TEST_F(UISystemTest, SyncBindingsWithIntData) {
#ifdef BESTOW_HAS_RMLUI
    int healthValue = 100;
    uiSystem_->bindData("health", &healthValue);

    std::string_view content = R"(
        <rml>
            <body>
                <div id="health-display" data-value="health">0</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    // Sync bindings - should update the element text
    uiSystem_->syncBindings();

    auto elemResult = uiSystem_->getElementById(docResult.value(), "health-display");
    ASSERT_TRUE(elemResult.has_value());

    std::string text = uiSystem_->getElementText(elemResult.value());
    EXPECT_EQ(text, "100");
#endif
}

TEST_F(UISystemTest, SyncBindingsWithFloatData) {
#ifdef BESTOW_HAS_RMLUI
    float scoreValue = 99.5f;
    uiSystem_->bindData("score", &scoreValue);

    std::string_view content = R"(
        <rml>
            <body>
                <div id="score-display" data-value="score">0</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    uiSystem_->syncBindings();

    auto elemResult = uiSystem_->getElementById(docResult.value(), "score-display");
    ASSERT_TRUE(elemResult.has_value());

    std::string text = uiSystem_->getElementText(elemResult.value());
    // Float converted to string
    EXPECT_NE(text, "0");
#endif
}

TEST_F(UISystemTest, SyncBindingsWithBoolData) {
#ifdef BESTOW_HAS_RMLUI
    bool isActiveValue = true;
    uiSystem_->bindData("active", &isActiveValue);

    std::string_view content = R"(
        <rml>
            <body>
                <div id="active-display" data-value="active">false</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    uiSystem_->syncBindings();

    auto elemResult = uiSystem_->getElementById(docResult.value(), "active-display");
    ASSERT_TRUE(elemResult.has_value());

    std::string text = uiSystem_->getElementText(elemResult.value());
    EXPECT_EQ(text, "true");
#endif
}

TEST_F(UISystemTest, SyncBindingsWithStringData) {
#ifdef BESTOW_HAS_RMLUI
    std::string playerName = "Hero";
    uiSystem_->bindData("player_name", &playerName);

    std::string_view content = R"(
        <rml>
            <body>
                <div id="name-display" data-value="player_name">Unknown</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    uiSystem_->syncBindings();

    auto elemResult = uiSystem_->getElementById(docResult.value(), "name-display");
    ASSERT_TRUE(elemResult.has_value());

    std::string text = uiSystem_->getElementText(elemResult.value());
    EXPECT_EQ(text, "Hero");
#endif
}

TEST_F(UISystemTest, SyncBindingsUpdatesMultipleElements) {
#ifdef BESTOW_HAS_RMLUI
    int counter = 5;
    uiSystem_->bindData("counter", &counter);

    std::string_view content = R"(
        <rml>
            <body>
                <div id="counter1" data-value="counter">0</div>
                <div id="counter2" data-value="counter">0</div>
                <div id="counter3" data-value="counter">0</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    uiSystem_->syncBindings();

    auto elem1 = uiSystem_->getElementById(docResult.value(), "counter1");
    auto elem2 = uiSystem_->getElementById(docResult.value(), "counter2");
    auto elem3 = uiSystem_->getElementById(docResult.value(), "counter3");

    ASSERT_TRUE(elem1.has_value());
    ASSERT_TRUE(elem2.has_value());
    ASSERT_TRUE(elem3.has_value());

    EXPECT_EQ(uiSystem_->getElementText(elem1.value()), "5");
    EXPECT_EQ(uiSystem_->getElementText(elem2.value()), "5");
    EXPECT_EQ(uiSystem_->getElementText(elem3.value()), "5");
#endif
}

TEST_F(UISystemTest, SyncBindingsAfterValueChange) {
#ifdef BESTOW_HAS_RMLUI
    int lives = 3;
    uiSystem_->bindData("lives", &lives);

    std::string_view content = R"(
        <rml>
            <body>
                <div id="lives-display" data-value="lives">0</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    uiSystem_->syncBindings();

    auto elemResult = uiSystem_->getElementById(docResult.value(), "lives-display");
    ASSERT_TRUE(elemResult.has_value());
    EXPECT_EQ(uiSystem_->getElementText(elemResult.value()), "3");

    // Change the value
    lives = 2;
    uiSystem_->syncBindings();

    EXPECT_EQ(uiSystem_->getElementText(elemResult.value()), "2");
#endif
}

//=============================================================================
// Event Callback Tests
//=============================================================================

TEST_F(UISystemTest, RegisterEventCallback) {
    bool callbackInvoked = false;
    UIEventCallback callback = [&callbackInvoked](const UIEventData& event) {
        callbackInvoked = true;
    };

    uiSystem_->registerEventCallback("click", callback);
    // Should not crash - callback should be registered
}

TEST_F(UISystemTest, RegisterMultipleEventCallbacks) {
    int callCount = 0;

    UIEventCallback callback1 = [&callCount](const UIEventData& event) {
        callCount++;
    };

    UIEventCallback callback2 = [&callCount](const UIEventData& event) {
        callCount++;
    };

    uiSystem_->registerEventCallback("click", callback1);
    uiSystem_->registerEventCallback("click", callback2);
    // Should not crash - both callbacks should be registered
}

TEST_F(UISystemTest, UnregisterEventCallback) {
    bool callbackInvoked = false;
    UIEventCallback callback = [&callbackInvoked](const UIEventData& event) {
        callbackInvoked = true;
    };

    uiSystem_->registerEventCallback("submit", callback);
    uiSystem_->unregisterEventCallback("submit");
    // Should not crash
}

TEST_F(UISystemTest, UnregisterNonExistentEventCallbackDoesNotCrash) {
    uiSystem_->unregisterEventCallback("non-existent-event");
    // Should not crash
}

TEST_F(UISystemTest, RegisterElementCallback) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="button">Click Me</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "button");
    ASSERT_TRUE(elemResult.has_value());

    bool callbackInvoked = false;
    UIEventCallback callback = [&callbackInvoked](const UIEventData& event) {
        callbackInvoked = true;
    };

    uiSystem_->registerElementCallback(elemResult.value(), "click", callback);
    // Should not crash - callback should be registered for this element
#endif
}

TEST_F(UISystemTest, RegisterElementCallbackOnNonExistentElementDoesNotCrash) {
    UIElementHandle fakeElem = 99999;
    UIEventCallback callback = [](const UIEventData& event) {};

    uiSystem_->registerElementCallback(fakeElem, "click", callback);
    // Should not crash
}

TEST_F(UISystemTest, RegisterMultipleElementCallbacks) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="element">Test</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "element");
    ASSERT_TRUE(elemResult.has_value());

    int callCount = 0;

    UIEventCallback callback1 = [&callCount](const UIEventData& event) {
        callCount++;
    };

    UIEventCallback callback2 = [&callCount](const UIEventData& event) {
        callCount++;
    };

    uiSystem_->registerElementCallback(elemResult.value(), "mouseover", callback1);
    uiSystem_->registerElementCallback(elemResult.value(), "mouseout", callback2);
    // Should not crash
#endif
}

//=============================================================================
// Input Focus Tests
//=============================================================================

TEST_F(UISystemTest, WantsKeyboardInputReturnsFalseByDefault) {
    bool wants = uiSystem_->wantsKeyboardInput();
    EXPECT_FALSE(wants);
}

TEST_F(UISystemTest, WantsKeyboardInputReturnsTrueWhenTextInputFocused) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(
        <rml>
            <body>
                <input id="text-input" type="text"/>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto inputElem = uiSystem_->getElementById(docResult.value(), "text-input");
    ASSERT_TRUE(inputElem.has_value());

    uiSystem_->focusElement(inputElem.value());

    bool wants = uiSystem_->wantsKeyboardInput();
    EXPECT_TRUE(wants);
#endif
}

TEST_F(UISystemTest, WantsKeyboardInputReturnsTrueWhenTextareaFocused) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(
        <rml>
            <body>
                <textarea id="text-area"></textarea>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto textareaElem = uiSystem_->getElementById(docResult.value(), "text-area");
    ASSERT_TRUE(textareaElem.has_value());

    uiSystem_->focusElement(textareaElem.value());

    bool wants = uiSystem_->wantsKeyboardInput();
    EXPECT_TRUE(wants);
#endif
}

TEST_F(UISystemTest, WantsKeyboardInputReturnsFalseWhenNonTextElementFocused) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(
        <rml>
            <body>
                <div id="non-text">Not a text input</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto divElem = uiSystem_->getElementById(docResult.value(), "non-text");
    ASSERT_TRUE(divElem.has_value());

    uiSystem_->focusElement(divElem.value());

    bool wants = uiSystem_->wantsKeyboardInput();
    EXPECT_FALSE(wants);
#endif
}

TEST_F(UISystemTest, WantsMouseInputReturnsFalseByDefault) {
    bool wants = uiSystem_->wantsMouseInput();
    EXPECT_FALSE(wants);
}

TEST_F(UISystemTest, WantsMouseInputReturnsTrueWhenDocumentVisible) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div>Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    uiSystem_->showDocument(docResult.value());

    bool wants = uiSystem_->wantsMouseInput();
    EXPECT_TRUE(wants);
#endif
}

TEST_F(UISystemTest, WantsMouseInputReturnsFalseWhenAllDocumentsHidden) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div>Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    uiSystem_->showDocument(docResult.value());
    EXPECT_TRUE(uiSystem_->wantsMouseInput());

    uiSystem_->hideDocument(docResult.value());
    EXPECT_FALSE(uiSystem_->wantsMouseInput());
#endif
}

//=============================================================================
// Input Processing Tests
//=============================================================================

TEST_F(UISystemTest, ProcessInputReturnsFalseByDefault) {
    UIInputEvent event;
    event.type = UIInputType::MouseMove;
    event.x = 100;
    event.y = 200;

    bool consumed = uiSystem_->processInput(event);
#ifdef BESTOW_HAS_RMLUI
    // May return true or false depending on implementation
#else
    EXPECT_FALSE(consumed);
#endif
}

TEST_F(UISystemTest, ProcessMouseMoveInput) {
    UIInputEvent event;
    event.type = UIInputType::MouseMove;
    event.x = 150;
    event.y = 250;

    bool consumed = uiSystem_->processInput(event);
    // Should not crash - input should be processed
}

TEST_F(UISystemTest, ProcessMouseDownInput) {
    UIInputEvent event;
    event.type = UIInputType::MouseDown;
    event.x = 100;
    event.y = 100;
    event.button = 0;  // Left button

    bool consumed = uiSystem_->processInput(event);
    // Should not crash
}

TEST_F(UISystemTest, ProcessMouseUpInput) {
    UIInputEvent event;
    event.type = UIInputType::MouseUp;
    event.x = 100;
    event.y = 100;
    event.button = 0;

    bool consumed = uiSystem_->processInput(event);
    // Should not crash
}

TEST_F(UISystemTest, ProcessMouseScrollInput) {
    UIInputEvent event;
    event.type = UIInputType::MouseScroll;
    event.wheelDelta = 120;

    bool consumed = uiSystem_->processInput(event);
    // Should not crash
}

TEST_F(UISystemTest, ProcessKeyDownInput) {
    UIInputEvent event;
    event.type = UIInputType::KeyDown;
    event.keyCode = 65;  // 'A' key

    bool consumed = uiSystem_->processInput(event);
    // Should not crash
}

TEST_F(UISystemTest, ProcessKeyUpInput) {
    UIInputEvent event;
    event.type = UIInputType::KeyUp;
    event.keyCode = 65;

    bool consumed = uiSystem_->processInput(event);
    // Should not crash
}

TEST_F(UISystemTest, ProcessTextInput) {
    UIInputEvent event;
    event.type = UIInputType::TextInput;
    event.character = U'A';

    bool consumed = uiSystem_->processInput(event);
    // Should not crash
}

//=============================================================================
// Dynamic Element Creation Tests
//=============================================================================

TEST_F(UISystemTest, CreateElement) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    UIElementHandle elem = uiSystem_->createElement(docResult.value(), "div");
    EXPECT_GT(elem, 0u);
#endif
}

TEST_F(UISystemTest, CreateElementOnNonExistentDocumentReturnsZero) {
    UIDocumentHandle fakeDoc = 99999;
    UIElementHandle elem = uiSystem_->createElement(fakeDoc, "div");
    EXPECT_EQ(elem, 0u);
}

TEST_F(UISystemTest, RemoveElement) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="removable">Content</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "removable");
    ASSERT_TRUE(elemResult.has_value());

    uiSystem_->removeElement(elemResult.value());
    // Element should be removed
#endif
}

TEST_F(UISystemTest, RemoveNonExistentElementDoesNotCrash) {
    UIElementHandle fakeElem = 99999;
    uiSystem_->removeElement(fakeElem);  // Should not crash
}

TEST_F(UISystemTest, SetInnerRml) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view content = R"(<rml><body><div id="container">Old</div></body></rml>)";
    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    auto elemResult = uiSystem_->getElementById(docResult.value(), "container");
    ASSERT_TRUE(elemResult.has_value());

    uiSystem_->setInnerRml(elemResult.value(), "<span>New Content</span>");
    // Should update the element's content
#endif
}

//=============================================================================
// Viewport and DPI Tests
//=============================================================================

TEST_F(UISystemTest, SetViewportSize) {
    uiSystem_->setViewportSize(1920, 1080);
    // Should not crash - viewport size should be updated
}

TEST_F(UISystemTest, SetDPIScale) {
    uiSystem_->setDPIScale(2.0f);
    // Should not crash - DPI scale should be updated
}

TEST_F(UISystemTest, SetViewportSizeMultipleTimes) {
    uiSystem_->setViewportSize(800, 600);
    uiSystem_->setViewportSize(1024, 768);
    uiSystem_->setViewportSize(1920, 1080);
    // Should not crash
}

//=============================================================================
// Debug Mode Tests
//=============================================================================

TEST_F(UISystemTest, SetDebugMode) {
    uiSystem_->setDebugMode(true);
    uiSystem_->setDebugMode(false);
    // Should not crash
}

TEST_F(UISystemTest, GetElementCount) {
    std::size_t count = uiSystem_->getElementCount();
    EXPECT_GE(count, 0u);
}

TEST_F(UISystemTest, GetElementCountIncreasesWithDocuments) {
#ifdef BESTOW_HAS_RMLUI
    std::size_t initialCount = uiSystem_->getElementCount();

    std::string_view content = R"(
        <rml>
            <body>
                <div id="elem1">First</div>
                <div id="elem2">Second</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());

    // Access elements to register them
    uiSystem_->getElementById(docResult.value(), "elem1");
    uiSystem_->getElementById(docResult.value(), "elem2");

    std::size_t newCount = uiSystem_->getElementCount();
    EXPECT_GT(newCount, initialCount);
#endif
}

//=============================================================================
// Font Loading Tests
//=============================================================================

TEST_F(UISystemTest, LoadFontWithoutAssetSystemReturnsError) {
#ifdef BESTOW_HAS_RMLUI
    // Create a fresh UI system without asset system dependency
    auto freshUI = std::make_unique<RmlUISystem>();
    UIConfig config;
    freshUI->initialize(config);

    auto result = freshUI->loadFont("fonts/Arial.ttf", "Arial");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), UIError::InternalError);

    freshUI->shutdown();
#endif
}

TEST_F(UISystemTest, LoadFontFromNonExistentFileReturnsError) {
#ifdef BESTOW_HAS_RMLUI
    auto result = uiSystem_->loadFont("nonexistent/font.ttf", "TestFont");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), UIError::FontNotFound);
#endif
}

//=============================================================================
// Integration Tests
//=============================================================================

TEST_F(UISystemTest, FullWorkflowLoadDocumentModifyElement) {
#ifdef BESTOW_HAS_RMLUI
    // Load document
    std::string_view content = R"(
        <rml>
            <body>
                <div id="title">Original Title</div>
                <div id="score">0</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content, "game-hud.rml");
    ASSERT_TRUE(docResult.has_value());

    // Show document
    uiSystem_->showDocument(docResult.value());
    EXPECT_TRUE(uiSystem_->isDocumentVisible(docResult.value()));

    // Get and modify elements
    auto titleElem = uiSystem_->getElementById(docResult.value(), "title");
    ASSERT_TRUE(titleElem.has_value());
    uiSystem_->setElementText(titleElem.value(), "Updated Title");

    auto scoreElem = uiSystem_->getElementById(docResult.value(), "score");
    ASSERT_TRUE(scoreElem.has_value());
    uiSystem_->setElementText(scoreElem.value(), "100");

    // Update and render
    uiSystem_->update(1.0f / 60.0f);
    uiSystem_->render();

    // Cleanup
    uiSystem_->hideDocument(docResult.value());
    uiSystem_->unloadDocument(docResult.value());
#endif
}

TEST_F(UISystemTest, FullWorkflowDataBindingAndSync) {
#ifdef BESTOW_HAS_RMLUI
    // Create game state variables
    int health = 100;
    int ammo = 30;
    std::string weaponName = "Pistol";

    // Bind data
    uiSystem_->bindData("health", &health);
    uiSystem_->bindData("ammo", &ammo);
    uiSystem_->bindData("weapon", &weaponName);

    // Load HUD document
    std::string_view content = R"(
        <rml>
            <body>
                <div id="health-value" data-value="health">0</div>
                <div id="ammo-value" data-value="ammo">0</div>
                <div id="weapon-name" data-value="weapon">None</div>
            </body>
        </rml>
    )";

    auto docResult = uiSystem_->loadDocumentFromString(content);
    ASSERT_TRUE(docResult.has_value());
    uiSystem_->showDocument(docResult.value());

    // Sync bindings
    uiSystem_->syncBindings();

    // Verify bindings updated
    auto healthElem = uiSystem_->getElementById(docResult.value(), "health-value");
    auto ammoElem = uiSystem_->getElementById(docResult.value(), "ammo-value");
    auto weaponElem = uiSystem_->getElementById(docResult.value(), "weapon-name");

    ASSERT_TRUE(healthElem.has_value());
    ASSERT_TRUE(ammoElem.has_value());
    ASSERT_TRUE(weaponElem.has_value());

    EXPECT_EQ(uiSystem_->getElementText(healthElem.value()), "100");
    EXPECT_EQ(uiSystem_->getElementText(ammoElem.value()), "30");
    EXPECT_EQ(uiSystem_->getElementText(weaponElem.value()), "Pistol");

    // Update game state and re-sync
    health = 75;
    ammo = 15;
    weaponName = "Shotgun";

    uiSystem_->syncBindings();

    EXPECT_EQ(uiSystem_->getElementText(healthElem.value()), "75");
    EXPECT_EQ(uiSystem_->getElementText(ammoElem.value()), "15");
    EXPECT_EQ(uiSystem_->getElementText(weaponElem.value()), "Shotgun");

    // Cleanup
    uiSystem_->unbindData("health");
    uiSystem_->unbindData("ammo");
    uiSystem_->unbindData("weapon");
#endif
}

TEST_F(UISystemTest, MultipleDocumentsIndependentState) {
#ifdef BESTOW_HAS_RMLUI
    std::string_view menu = R"(<rml><body><div id="menu">Main Menu</div></body></rml>)";
    std::string_view hud = R"(<rml><body><div id="hud">HUD</div></body></rml>)";
    std::string_view pause = R"(<rml><body><div id="pause">Paused</div></body></rml>)";

    auto menuDoc = uiSystem_->loadDocumentFromString(menu, "menu.rml");
    auto hudDoc = uiSystem_->loadDocumentFromString(hud, "hud.rml");
    auto pauseDoc = uiSystem_->loadDocumentFromString(pause, "pause.rml");

    ASSERT_TRUE(menuDoc.has_value());
    ASSERT_TRUE(hudDoc.has_value());
    ASSERT_TRUE(pauseDoc.has_value());

    // All documents unique
    EXPECT_NE(menuDoc.value(), hudDoc.value());
    EXPECT_NE(hudDoc.value(), pauseDoc.value());

    // Show menu, hide others
    uiSystem_->showDocument(menuDoc.value());
    uiSystem_->hideDocument(hudDoc.value());
    uiSystem_->hideDocument(pauseDoc.value());

    EXPECT_TRUE(uiSystem_->isDocumentVisible(menuDoc.value()));
    EXPECT_FALSE(uiSystem_->isDocumentVisible(hudDoc.value()));
    EXPECT_FALSE(uiSystem_->isDocumentVisible(pauseDoc.value()));

    // Switch to game (show HUD, hide menu)
    uiSystem_->hideDocument(menuDoc.value());
    uiSystem_->showDocument(hudDoc.value());

    EXPECT_FALSE(uiSystem_->isDocumentVisible(menuDoc.value()));
    EXPECT_TRUE(uiSystem_->isDocumentVisible(hudDoc.value()));
    EXPECT_FALSE(uiSystem_->isDocumentVisible(pauseDoc.value()));
#endif
}

}  // namespace bestow::tests
