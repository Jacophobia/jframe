// tests/mocks/MockSceneSystem.hpp
// Shared mock scene system for testing

#pragma once

#include <kangaru/kangaru.hpp>

import std;
import bestow;
import bestow.types;

namespace bestow::tests {

class MockSceneSystem : public ISceneSystem {
public:
    // Expose internal state for test assertions
    std::unordered_map<std::string, SceneMetadata> registry_;
    std::vector<std::string> stack_;

    // ISceneSystem overrides
    void update(DeltaTime) override {}
    void render() override {}

    Result<SceneId, std::error_code> registerScene(
        const std::string& name, AssetHandle sceneAsset) override {
        if (registry_.contains(name)) {
            return std::unexpected(std::make_error_code(std::errc::file_exists));
        }
        SceneId id = nextId_++;
        registry_[name] = SceneMetadata{
            .id = id,
            .name = name,
            .assetHandle = sceneAsset,
            .phase = "",
            .state = SceneState::Ready,
        };
        return id;
    }

    Result<void, std::error_code> unregisterScene(const std::string& name) override {
        if (!registry_.contains(name)) {
            return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
        }
        for (const auto& entry : stack_) {
            if (entry == name) {
                return std::unexpected(std::make_error_code(std::errc::device_or_resource_busy));
            }
        }
        registry_.erase(name);
        return {};
    }

    Result<void, std::error_code> pushScene(
        const std::string& name,
        const std::unordered_map<std::string, std::any>& params = {}) override {
        if (!registry_.contains(name)) {
            return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
        }
        if (!stack_.empty()) {
            registry_[stack_.back()].state = SceneState::Paused;
        }
        stack_.push_back(name);
        registry_[name].state = SceneState::Active;
        return {};
    }

    Result<void, std::error_code> popScene() override {
        if (stack_.empty()) {
            return std::unexpected(std::make_error_code(std::errc::operation_not_permitted));
        }
        registry_[stack_.back()].state = SceneState::Ready;
        stack_.pop_back();
        if (!stack_.empty()) {
            registry_[stack_.back()].state = SceneState::Active;
        }
        return {};
    }

    Result<void, std::error_code> replaceScene(
        const std::string& name,
        const std::unordered_map<std::string, std::any>& params = {}) override {
        if (!registry_.contains(name)) {
            return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
        }
        if (!stack_.empty()) {
            registry_[stack_.back()].state = SceneState::Ready;
            stack_.pop_back();
        }
        stack_.push_back(name);
        registry_[name].state = SceneState::Active;
        return {};
    }

    void clearStack() override {
        for (auto& name : stack_) {
            registry_[name].state = SceneState::Ready;
        }
        stack_.clear();
    }

    std::optional<std::string> getActiveSceneName() const override {
        if (stack_.empty()) return std::nullopt;
        return stack_.back();
    }

    std::vector<std::string> getSceneStack() const override { return stack_; }

    SceneState getSceneState(const std::string& name) const override {
        auto it = registry_.find(name);
        if (it == registry_.end()) return SceneState::Unloaded;
        return it->second.state;
    }

    std::optional<SceneMetadata> getSceneMetadata(const std::string& name) const override {
        auto it = registry_.find(name);
        if (it == registry_.end()) return std::nullopt;
        return it->second;
    }

    std::vector<std::string> getRegisteredScenes() const override {
        std::vector<std::string> result;
        for (const auto& [name, _] : registry_) {
            result.push_back(name);
        }
        return result;
    }

private:
    SceneId nextId_ = 1;
};

}  // namespace bestow::tests
