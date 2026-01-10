// bestow-luabind/src/bindings/character_binding.cpp
// High-level Character API for Lua - user-friendly animated character management

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>

module bestow.luabind;

import std;

namespace bestow {

//=============================================================================
// LuaAnimationEvent - Event definition with timing and custom data
//=============================================================================

struct LuaAnimationEvent {
    std::string name;
    float time = 0.0f;           // Normalized time (0-1) or absolute time
    float duration = 0.0f;       // Duration for window-based events
    sol::table data;             // Custom data (foot="left", damage=50, etc.)
    bool isNormalized = true;    // If true, time is 0-1; if false, time is seconds
};

//=============================================================================
// LuaCharacter - High-level animated character for Lua
//=============================================================================

class LuaCharacter {
public:
    LuaCharacter(sol::state& lua, IAnimationSystem& animation, IGraphics3DSystem* graphics)
        : lua_(&lua), animation_(&animation), graphics_(graphics) {}

    ~LuaCharacter() {
        destroy();
    }

    //=========================================================================
    // Initialization
    //=========================================================================

    bool loadFromConfig(sol::table config) {
        // Get model path
        sol::optional<std::string> modelPath = config["model"];
        if (!modelPath) {
            spdlog::error("[Character] Missing 'model' in config");
            return false;
        }

        modelPath_ = *modelPath;

        // Get animations table
        sol::optional<sol::table> animations = config["animations"];
        sol::optional<std::string> animationDir = config["animationDir"];

        // Store for later loading (we need assets system)
        animationsConfig_ = animations ? *animations : sol::table();
        animationDir_ = animationDir.value_or("");

        return true;
    }

    bool initialize(IAssetSystem& assets) {
        assets_ = &assets;

        // Load model
        AssetHandle modelHandle = assets.loadModel(std::filesystem::path(modelPath_));
        if (!assets.isLoaded(modelHandle)) {
            spdlog::error("[Character] Failed to load model: {}", modelPath_);
            return false;
        }
        modelHandle_ = modelHandle;

        const ModelData* modelData = assets.getModelData(modelHandle);
        if (!modelData) {
            spdlog::error("[Character] No model data for: {}", modelPath_);
            return false;
        }

        // Create skeleton
        auto skeletonResult = animation_->createSkeleton(*modelData);
        if (!skeletonResult) {
            spdlog::error("[Character] Failed to create skeleton from: {}", modelPath_);
            return false;
        }
        skeleton_ = *skeletonResult;

        // Create animator
        auto animatorResult = animation_->createAnimator(skeleton_);
        if (!animatorResult) {
            spdlog::error("[Character] Failed to create animator for: {}", modelPath_);
            animation_->destroySkeleton(skeleton_);
            return false;
        }
        animator_ = *animatorResult;

        // Create mesh
        if (graphics_ && !modelData->meshes.empty()) {
            auto meshResult = graphics_->createMeshFromData(modelData->meshes[0]);
            if (meshResult) {
                mesh_ = *meshResult;
            }

            // Create material
            if (!modelData->materials.empty()) {
                auto matResult = graphics_->createMaterialsFromModel(*modelData);
                if (matResult && !matResult->empty()) {
                    material_ = (*matResult)[0];
                }
            }
            if (material_ == 0) {
                material_ = graphics_->getDefaultPBRMaterial();
            }
        }

        // Load animations from config
        if (animationsConfig_.valid()) {
            loadAnimationsFromTable(animationsConfig_);
        }

        // Auto-discover animations from directory
        if (!animationDir_.empty()) {
            autoDiscoverAnimations(animationDir_);
        }

        // Subscribe to animation events
        eventSubscription_ = animation_->subscribeToEvents(animator_,
            [this](const AnimationEvent& event) {
                onAnimationSystemEvent(event);
            });

        // Subscribe to animation completion
        completeSubscription_ = animation_->subscribeToComplete(animator_,
            [this](AnimatorHandle, AnimationClipHandle clip, std::uint32_t layer) {
                onAnimationComplete(clip, layer);
            });

        spdlog::info("[Character] Loaded '{}' with {} animations",
                    modelPath_, clips_.size());
        return true;
    }

    void destroy() {
        if (eventSubscription_ != 0) {
            animation_->unsubscribe(eventSubscription_);
            eventSubscription_ = 0;
        }
        if (completeSubscription_ != 0) {
            animation_->unsubscribe(completeSubscription_);
            completeSubscription_ = 0;
        }
        if (animator_ != 0) {
            animation_->destroyAnimator(animator_);
            animator_ = 0;
        }
        if (skeleton_ != 0) {
            animation_->destroySkeleton(skeleton_);
            skeleton_ = 0;
        }
        clips_.clear();
        clipEvents_.clear();
    }

    //=========================================================================
    // Animation Playback
    //=========================================================================

    void play(const std::string& animName, sol::optional<float> blendTime) {
        auto it = clips_.find(animName);
        if (it == clips_.end()) {
            spdlog::warn("[Character] Animation '{}' not found. Available: {}",
                        animName, getAvailableAnimationsString());
            return;
        }

        float blend = blendTime.value_or(defaultBlendTime_);
        animation_->play(animator_, it->second, blend);
        currentAnimation_ = animName;
    }

    void playOnce(const std::string& animName,
                  sol::object thenArg1,
                  sol::optional<sol::object> thenArg2,
                  sol::optional<float> blendTime) {
        // Parse callback: either string (next anim) or table+methodName
        OneShotCallback callback;

        if (thenArg1.is<std::string>()) {
            // playOnce("attack", "idle") - play idle after
            callback.nextAnimation = thenArg1.as<std::string>();
        } else if (thenArg1.is<sol::table>() && thenArg2 && thenArg2->is<std::string>()) {
            // playOnce("attack", self, "onComplete") - hot-reload safe callback
            callback.callbackTable = thenArg1.as<sol::table>();
            callback.callbackMethod = thenArg2->as<std::string>();
        }

        oneShotCallbacks_[animName] = callback;
        play(animName, blendTime);
    }

    void stop(sol::optional<float> fadeTime) {
        animation_->stop(animator_, fadeTime.value_or(0.0f));
        currentAnimation_ = "";
    }

    void pause() {
        animation_->setPaused(animator_, true);
    }

    void resume() {
        animation_->setPaused(animator_, false);
    }

    void setSpeed(float speed) {
        animation_->setSpeed(animator_, speed);
    }

    float getSpeed() const {
        return animation_->getSpeed(animator_);
    }

    void setDefaultBlendTime(float time) {
        defaultBlendTime_ = time;
    }

    //=========================================================================
    // Transform
    //=========================================================================

    void setPosition(float x, float y, float z) {
        position_ = Vec3{x, y, z};
    }

    void setPositionVec(const Vec3& pos) {
        position_ = pos;
    }

    Vec3 getPosition() const {
        return position_;
    }

    void setRotation(float yaw) {
        rotation_ = yaw;
    }

    float getRotation() const {
        return rotation_;
    }

    void setScale(float scale) {
        scale_ = scale;
    }

    float getScale() const {
        return scale_;
    }

    //=========================================================================
    // Rendering
    //=========================================================================

    void draw() {
        if (!graphics_ || mesh_ == 0 || animator_ == 0) return;

        Mat4 worldMatrix = buildWorldMatrix();
        auto boneTransforms = animation_->getBoneTransforms(animator_);
        graphics_->drawSkinnedMesh(mesh_, material_, worldMatrix, boneTransforms);
    }

    void drawAt(float x, float y, float z, float rotation) {
        position_ = Vec3{x, y, z};
        rotation_ = rotation;
        draw();
    }

    //=========================================================================
    // Query Helpers
    //=========================================================================

    bool isPlaying(sol::optional<std::string> animName) const {
        if (!animation_->isPlaying(animator_)) {
            return false;
        }
        if (animName) {
            return currentAnimation_ == *animName;
        }
        return true;
    }

    std::string getCurrentAnimation() const {
        return currentAnimation_;
    }

    float getAnimationProgress() const {
        return animation_->getNormalizedTime(animator_, 0);
    }

    float getAnimationTime() const {
        return animation_->getCurrentTime(animator_, 0);
    }

    float getAnimationDuration() const {
        return animation_->getClipDuration(animator_, 0);
    }

    sol::table getAvailableAnimations() const {
        sol::table result = lua_->create_table();
        int i = 1;
        for (const auto& [name, handle] : clips_) {
            result[i++] = name;
        }
        return result;
    }

    bool hasAnimation(const std::string& name) const {
        return clips_.find(name) != clips_.end();
    }

    //=========================================================================
    // Animation Events
    //=========================================================================

    void addEvent(const std::string& animName, float time, const std::string& eventName,
                  sol::optional<sol::table> data) {
        LuaAnimationEvent event;
        event.name = eventName;
        event.time = time;
        event.data = data.value_or(lua_->create_table());
        clipEvents_[animName].push_back(event);
    }

    void addEventWithDuration(const std::string& animName, float time,
                              const std::string& eventName, float duration,
                              sol::optional<sol::table> data) {
        LuaAnimationEvent event;
        event.name = eventName;
        event.time = time;
        event.duration = duration;
        event.data = data.value_or(lua_->create_table());
        clipEvents_[animName].push_back(event);
    }

    // Subscribe to all events (hot-reload safe)
    void onEvent(sol::table table, const std::string& methodName) {
        eventCallbackTable_ = table;
        eventCallbackMethod_ = methodName;
    }

    // Subscribe to specific event name
    void on(const std::string& eventName, sol::table table, const std::string& methodName) {
        specificEventCallbacks_[eventName] = {table, methodName};
    }

    // Check if a duration-based event is currently active
    bool isEventActive(const std::string& eventName) const {
        auto it = activeEvents_.find(eventName);
        return it != activeEvents_.end() && it->second > 0.0f;
    }

    sol::table getActiveEvents() const {
        sol::table result = lua_->create_table();
        int i = 1;
        for (const auto& [name, remaining] : activeEvents_) {
            if (remaining > 0.0f) {
                sol::table event = lua_->create_table();
                event["name"] = name;
                event["remainingTime"] = remaining;
                result[i++] = event;
            }
        }
        return result;
    }

    //=========================================================================
    // Update (call each frame to process events)
    //=========================================================================

    void update(float dt) {
        // Update active events (decay durations)
        for (auto it = activeEvents_.begin(); it != activeEvents_.end();) {
            it->second -= dt;
            if (it->second <= 0.0f) {
                it = activeEvents_.erase(it);
            } else {
                ++it;
            }
        }

        // Check for custom events based on animation progress
        if (!currentAnimation_.empty()) {
            float progress = getAnimationProgress();
            processCustomEvents(currentAnimation_, progress, dt);
        }
    }

    //=========================================================================
    // Debug
    //=========================================================================

    void showSkeleton(bool show) {
        showSkeleton_ = show;
        animation_->setDebugVisualization(show);
    }

    void showBounds(bool show) {
        showBounds_ = show;
    }

private:
    //=========================================================================
    // Private Helpers
    //=========================================================================

    void loadAnimationsFromTable(sol::table animations) {
        for (auto& [key, value] : animations) {
            if (!key.is<std::string>()) continue;
            std::string animName = key.as<std::string>();

            std::string path;
            sol::table events;

            if (value.is<std::string>()) {
                // Simple: { idle = "path/to/idle.fbx" }
                path = value.as<std::string>();
            } else if (value.is<sol::table>()) {
                // Complex: { idle = { path = "...", events = {...} } }
                sol::table animConfig = value.as<sol::table>();
                sol::optional<std::string> pathOpt = animConfig["path"];
                if (!pathOpt) continue;
                path = *pathOpt;

                sol::optional<sol::table> eventsOpt = animConfig["events"];
                if (eventsOpt) {
                    events = *eventsOpt;
                }
            } else {
                continue;
            }

            // Load the animation
            if (loadAnimation(animName, path)) {
                // Load events for this animation
                if (events.valid()) {
                    loadEventsFromTable(animName, events);
                }
            }
        }
    }

    bool loadAnimation(const std::string& name, const std::string& path) {
        if (!assets_) return false;

        AssetHandle animHandle = assets_->loadModel(std::filesystem::path(path));
        if (!assets_->isLoaded(animHandle)) {
            spdlog::warn("[Character] Failed to load animation '{}' from: {}", name, path);
            return false;
        }

        const ModelData* animData = assets_->getModelData(animHandle);
        if (!animData || animData->animations.empty()) {
            spdlog::warn("[Character] No animation data in: {}", path);
            return false;
        }

        auto clips = animation_->createAnimationClips(skeleton_, *animData);
        if (clips.empty()) {
            spdlog::warn("[Character] Failed to create clips from: {}", path);
            return false;
        }

        clips_[name] = clips[0];
        clipToName_[clips[0]] = name;
        spdlog::debug("[Character] Loaded animation: {}", name);
        return true;
    }

    void loadEventsFromTable(const std::string& animName, sol::table events) {
        for (auto& [idx, eventData] : events) {
            if (!eventData.is<sol::table>()) continue;
            sol::table ev = eventData.as<sol::table>();

            LuaAnimationEvent event;
            event.time = ev.get_or("time", 0.0f);
            event.name = ev.get_or<std::string>("name", "unnamed");
            event.duration = ev.get_or("duration", 0.0f);

            // Copy all extra fields to data table
            event.data = lua_->create_table();
            for (auto& [k, v] : ev) {
                if (k.is<std::string>()) {
                    std::string key = k.as<std::string>();
                    if (key != "time" && key != "name" && key != "duration") {
                        event.data[key] = v;
                    }
                }
            }

            clipEvents_[animName].push_back(event);
        }
    }

    void autoDiscoverAnimations(const std::string& dir) {
        if (!assets_) return;

        auto assetList = assets_->listLibraryAssetsRecursive(dir);
        for (const auto& asset : assetList) {
            if (asset.extension == ".fbx" || asset.extension == ".gltf" || asset.extension == ".glb") {
                // Use stem as animation name (e.g., "idle" from "idle.fbx")
                std::string animName = asset.stem;
                loadAnimation(animName, asset.libraryPath);
            }
        }
    }

    Mat4 buildWorldMatrix() const {
        Mat4 world = Mat4(1.0f);  // Identity
        world = glm::translate(world, position_);
        world = glm::rotate(world, rotation_, Vec3{0.0f, 1.0f, 0.0f});
        world = glm::scale(world, Vec3{scale_, scale_, scale_});
        return world;
    }

    std::string getAvailableAnimationsString() const {
        std::string result;
        for (const auto& [name, handle] : clips_) {
            if (!result.empty()) result += ", ";
            result += name;
        }
        return result.empty() ? "(none)" : result;
    }

    void onAnimationSystemEvent(const AnimationEvent& event) {
        // System events from C++ animation system (if defined in FBX, etc.)
        // Forward to Lua callbacks
        dispatchEvent(event.name, currentAnimation_, event.clipTime,
                     lua_->create_table());
    }

    void onAnimationComplete(AnimationClipHandle clip, std::uint32_t layer) {
        // Find animation name
        auto it = clipToName_.find(clip);
        std::string animName = (it != clipToName_.end()) ? it->second : "";

        // Check for one-shot callback
        auto callbackIt = oneShotCallbacks_.find(animName);
        if (callbackIt != oneShotCallbacks_.end()) {
            OneShotCallback& callback = callbackIt->second;

            if (!callback.nextAnimation.empty()) {
                // Play next animation
                play(callback.nextAnimation, std::nullopt);
            } else if (callback.callbackTable.valid()) {
                // Call method on table
                invokeCallback(callback.callbackTable, callback.callbackMethod, sol::make_object(*lua_, animName));
            }

            oneShotCallbacks_.erase(callbackIt);
        }
    }

    void processCustomEvents(const std::string& animName, float progress, float dt) {
        auto it = clipEvents_.find(animName);
        if (it == clipEvents_.end()) return;

        float lastProgress = lastEventProgress_;
        lastEventProgress_ = progress;

        // Handle looping (progress went from near 1 to near 0)
        bool looped = progress < lastProgress - 0.5f;
        if (looped) {
            lastProgress = 0.0f;
        }

        for (const auto& event : it->second) {
            // Check if we crossed this event's time this frame
            if (event.time > lastProgress && event.time <= progress) {
                // Fire the event
                dispatchEvent(event.name, animName, event.time, event.data);

                // If it has a duration, add to active events
                if (event.duration > 0.0f) {
                    activeEvents_[event.name] = event.duration;
                }
            }
        }
    }

    void dispatchEvent(const std::string& eventName, const std::string& animName,
                       float time, sol::table data) {
        // Build event table
        sol::table eventTable = lua_->create_table();
        eventTable["name"] = eventName;
        eventTable["animation"] = animName;
        eventTable["time"] = time;
        eventTable["data"] = data;

        // Call specific event callback if registered
        auto specificIt = specificEventCallbacks_.find(eventName);
        if (specificIt != specificEventCallbacks_.end()) {
            invokeCallback(specificIt->second.first, specificIt->second.second, eventTable);
        }

        // Call general event callback if registered
        if (eventCallbackTable_.valid() && !eventCallbackMethod_.empty()) {
            invokeCallback(eventCallbackTable_, eventCallbackMethod_, eventTable);
        }
    }

    void invokeCallback(sol::table table, const std::string& methodName, sol::object arg) {
        if (!table.valid()) return;

        sol::object methodObj = table[methodName];
        if (!methodObj.valid() || methodObj.get_type() != sol::type::function) {
            spdlog::warn("[Character] Callback method '{}' not found on table", methodName);
            return;
        }

        sol::protected_function method = methodObj;
        sol::protected_function_result result = method(table, arg);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Character] Error in callback '{}': {}", methodName, err.what());
        }
    }

    //=========================================================================
    // Member Data
    //=========================================================================

    // System references
    sol::state* lua_ = nullptr;
    IAnimationSystem* animation_ = nullptr;
    IGraphics3DSystem* graphics_ = nullptr;
    IAssetSystem* assets_ = nullptr;

    // Config (stored before initialize)
    std::string modelPath_;
    sol::table animationsConfig_;
    std::string animationDir_;

    // Handles
    AssetHandle modelHandle_ = {};
    SkeletonHandle skeleton_ = 0;
    AnimatorHandle animator_ = 0;
    MeshHandle mesh_ = 0;
    MaterialHandle material_ = 0;

    // Animation clips
    std::unordered_map<std::string, AnimationClipHandle> clips_;
    std::unordered_map<AnimationClipHandle, std::string> clipToName_;

    // Animation state
    std::string currentAnimation_;
    float defaultBlendTime_ = 0.25f;

    // Transform
    Vec3 position_{0.0f, 0.0f, 0.0f};
    float rotation_ = 0.0f;
    float scale_ = 1.0f;

    // Event subscriptions
    SubscriptionId eventSubscription_ = 0;
    SubscriptionId completeSubscription_ = 0;

    // Custom events
    std::unordered_map<std::string, std::vector<LuaAnimationEvent>> clipEvents_;
    std::unordered_map<std::string, float> activeEvents_;  // name -> remaining duration
    float lastEventProgress_ = 0.0f;

    // Event callbacks (hot-reload safe)
    sol::table eventCallbackTable_;
    std::string eventCallbackMethod_;
    std::unordered_map<std::string, std::pair<sol::table, std::string>> specificEventCallbacks_;

    // One-shot callbacks
    struct OneShotCallback {
        std::string nextAnimation;
        sol::table callbackTable;
        std::string callbackMethod;
    };
    std::unordered_map<std::string, OneShotCallback> oneShotCallbacks_;

    // Debug
    bool showSkeleton_ = false;
    bool showBounds_ = false;
};

//=============================================================================
// Lua Binding for Character
//=============================================================================

void bindCharacterSystem(sol::state& lua, IAnimationSystem& animation,
                         IAssetSystem* assets, IGraphics3DSystem* graphics) {
    // Character usertype
    lua.new_usertype<LuaCharacter>("Character",
        sol::no_constructor,  // Created via bestow.animation.loadCharacter

        // Playback
        "play", &LuaCharacter::play,
        "playOnce", &LuaCharacter::playOnce,
        "stop", &LuaCharacter::stop,
        "pause", &LuaCharacter::pause,
        "resume", &LuaCharacter::resume,
        "setSpeed", &LuaCharacter::setSpeed,
        "getSpeed", &LuaCharacter::getSpeed,
        "setDefaultBlendTime", &LuaCharacter::setDefaultBlendTime,

        // Transform
        "setPosition", sol::overload(
            &LuaCharacter::setPosition,
            &LuaCharacter::setPositionVec
        ),
        "getPosition", &LuaCharacter::getPosition,
        "setRotation", &LuaCharacter::setRotation,
        "getRotation", &LuaCharacter::getRotation,
        "setScale", &LuaCharacter::setScale,
        "getScale", &LuaCharacter::getScale,

        // Rendering
        "draw", sol::overload(
            static_cast<void(LuaCharacter::*)()>(&LuaCharacter::draw),
            &LuaCharacter::drawAt
        ),

        // Queries
        "isPlaying", &LuaCharacter::isPlaying,
        "getCurrentAnimation", &LuaCharacter::getCurrentAnimation,
        "getAnimationProgress", &LuaCharacter::getAnimationProgress,
        "getAnimationTime", &LuaCharacter::getAnimationTime,
        "getAnimationDuration", &LuaCharacter::getAnimationDuration,
        "getAvailableAnimations", &LuaCharacter::getAvailableAnimations,
        "hasAnimation", &LuaCharacter::hasAnimation,

        // Events
        "addEvent", &LuaCharacter::addEvent,
        "addEventWithDuration", &LuaCharacter::addEventWithDuration,
        "onEvent", &LuaCharacter::onEvent,
        "on", &LuaCharacter::on,
        "isEventActive", &LuaCharacter::isEventActive,
        "getActiveEvents", &LuaCharacter::getActiveEvents,

        // Lifecycle
        "update", &LuaCharacter::update,
        "destroy", &LuaCharacter::destroy,

        // Debug
        "showSkeleton", &LuaCharacter::showSkeleton,
        "showBounds", &LuaCharacter::showBounds
    );

    // Factory function: bestow.animation.loadCharacter(config)
    sol::table bestow = lua["bestow"];
    sol::table animTable = bestow["animation"];

    if (assets && graphics) {
        animTable["loadCharacter"] = [&lua, &animation, assets, graphics](sol::table config)
            -> std::shared_ptr<LuaCharacter>
        {
            auto character = std::make_shared<LuaCharacter>(lua, animation, graphics);

            if (!character->loadFromConfig(config)) {
                return nullptr;
            }

            if (!character->initialize(*assets)) {
                return nullptr;
            }

            return character;
        };
    }

    spdlog::info("[Character] Lua bindings initialized");
}

}  // namespace bestow
