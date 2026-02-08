// tests/mocks/MockUISystem.hpp
// Shared mock UI system for testing

#pragma once

import std;
import bestow;
import bestow.types;

namespace bestow::tests {

class MockUISystem : public IUISystem {
public:
    // Tracking
    int shownCount = 0;
    int hiddenCount = 0;

    // Delegates
    std::function<void(UIDocumentHandle)> onShowDocument =
        [this](UIDocumentHandle) { shownCount++; };
    std::function<void(UIDocumentHandle)> onHideDocument =
        [this](UIDocumentHandle) { hiddenCount++; };

    // IUISystem overrides
    Result<void, UIError> initialize(const UIConfig&) override { return {}; }
    void shutdown() override {}
    Result<UIDocumentHandle, UIError> loadDocument(const std::filesystem::path&) override {
        return UIDocumentHandle{nextDocId_++};
    }
    Result<UIDocumentHandle, UIError> loadDocumentFromString(
        std::string_view, const std::string&) override {
        return UIDocumentHandle{nextDocId_++};
    }
    void unloadDocument(UIDocumentHandle) override {}
    void showDocument(UIDocumentHandle doc) override { onShowDocument(doc); }
    void hideDocument(UIDocumentHandle doc) override { onHideDocument(doc); }
    bool isDocumentVisible(UIDocumentHandle) const override { return true; }
    std::vector<UIDocumentHandle> getLoadedDocuments() const override { return {}; }
    Result<UIStyleSheetHandle, UIError> loadStyleSheet(const std::filesystem::path&) override {
        return UIStyleSheetHandle{};
    }
    Result<void, UIError> applyStyleSheet(UIDocumentHandle, UIStyleSheetHandle) override {
        return {};
    }
    std::optional<UIElementHandle> getElementById(UIDocumentHandle, const std::string&) override {
        return std::nullopt;
    }
    std::vector<UIElementHandle> getElementsByClass(UIDocumentHandle,
                                                     const std::string&) override {
        return {};
    }
    std::vector<UIElementHandle> getElementsByTag(UIDocumentHandle,
                                                   const std::string&) override {
        return {};
    }
    std::vector<UIElementHandle> getChildren(UIElementHandle) override { return {}; }
    std::optional<UIElementHandle> getParent(UIElementHandle) override { return std::nullopt; }
    void setElementText(UIElementHandle, const std::string&) override {}
    std::string getElementText(UIElementHandle) override { return ""; }
    void setElementVisible(UIElementHandle, UIVisibility) override {}
    UIVisibility getElementVisibility(UIElementHandle) override { return UIVisibility::Visible; }
    void addElementClass(UIElementHandle, const std::string&) override {}
    void removeElementClass(UIElementHandle, const std::string&) override {}
    bool hasElementClass(UIElementHandle, const std::string&) override { return false; }
    void setElementAttribute(UIElementHandle, const std::string&, const std::string&) override {}
    std::optional<std::string> getElementAttribute(UIElementHandle,
                                                    const std::string&) override {
        return std::nullopt;
    }
    void setElementStyle(UIElementHandle, const std::string&, const std::string&) override {}
    UIRect getElementBounds(UIElementHandle) override { return {}; }
    void focusElement(UIElementHandle) override {}
    void blurElement(UIElementHandle) override {}
    UIElementHandle createElement(UIDocumentHandle, const std::string&) override { return {}; }
    void appendChild(UIElementHandle, UIElementHandle) override {}
    void removeElement(UIElementHandle) override {}
    void setInnerRml(UIElementHandle, const std::string&) override {}
    void bindData(const std::string&, int*) override {}
    void bindData(const std::string&, float*) override {}
    void bindData(const std::string&, bool*) override {}
    void bindData(const std::string&, std::string*) override {}
    void unbindData(const std::string&) override {}
    void syncBindings() override {}
    void registerEventCallback(const std::string&, UIEventCallback) override {}
    void registerElementCallback(UIElementHandle, const std::string&, UIEventCallback) override {}
    void unregisterEventCallback(const std::string&) override {}
    bool processInput(const UIInputEvent&) override { return false; }
    bool wantsKeyboardInput() const override { return false; }
    bool wantsMouseInput() const override { return false; }
    void update(DeltaTime) override {}
    void render() override {}
    Result<void, UIError> loadFont(const std::filesystem::path&,
                                    const std::string&) override {
        return {};
    }
    void setDebugMode(bool) override {}
    std::size_t getElementCount() const override { return 0; }
    void setViewportSize(int, int) override {}
    void setDPIScale(float) override {}

private:
    std::uint64_t nextDocId_ = 1;
};

}  // namespace bestow::tests
