// bestow-luabind/src/bindings/animation_binding.cpp
// Animation system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

// Helper to convert Result<T, AnimationError> to Lua (value) or nil on error
template<typename T>
sol::object animResultToLua(sol::state& lua, const Result<T, AnimationError>& result) {
    if (result.has_value()) {
        return sol::make_object(lua, result.value());
    }
    return sol::nil;
}

// Helper for Result<void, AnimationError>
sol::object animVoidResultToLua(sol::state& lua, const Result<void, AnimationError>& result) {
    if (result.has_value()) {
        return sol::make_object(lua, true);
    }
    return sol::make_object(lua, false);
}

void bindAnimationSystem(sol::state& lua, IAnimationSystem& animation,
                         IAssetSystem* assets, IGraphics3DSystem* graphics) {
    //=========================================================================
    // Animation-related types
    //=========================================================================

    // AnimationError enum
    lua.new_enum<AnimationError>("AnimationError",
        {
            {"Success", AnimationError::Success},
            {"InvalidSkeleton", AnimationError::InvalidSkeleton},
            {"SkeletonNotFound", AnimationError::SkeletonNotFound},
            {"InvalidBoneIndex", AnimationError::InvalidBoneIndex},
            {"BoneNotFound", AnimationError::BoneNotFound},
            {"EmptyBoneData", AnimationError::EmptyBoneData},
            {"InvalidClip", AnimationError::InvalidClip},
            {"ClipNotFound", AnimationError::ClipNotFound},
            {"IncompatibleSkeleton", AnimationError::IncompatibleSkeleton},
            {"NoAnimationData", AnimationError::NoAnimationData},
            {"DuplicateClipName", AnimationError::DuplicateClipName},
            {"InvalidAnimator", AnimationError::InvalidAnimator},
            {"AnimatorNotFound", AnimationError::AnimatorNotFound},
            {"AnimatorSkeletonMismatch", AnimationError::AnimatorSkeletonMismatch},
            {"InvalidSocket", AnimationError::InvalidSocket},
            {"SocketNotFound", AnimationError::SocketNotFound},
            {"SocketAlreadyExists", AnimationError::SocketAlreadyExists},
            {"SocketBoneNotFound", AnimationError::SocketBoneNotFound},
            {"IKChainNotFound", AnimationError::IKChainNotFound},
            {"IKChainInvalid", AnimationError::IKChainInvalid},
            {"IKSolveFailed", AnimationError::IKSolveFailed},
            {"SamplingFailed", AnimationError::SamplingFailed},
            {"BlendingFailed", AnimationError::BlendingFailed},
            {"InvalidTimeRange", AnimationError::InvalidTimeRange},
            {"InvalidWeight", AnimationError::InvalidWeight},
            {"PhysicsSystemRequired", AnimationError::PhysicsSystemRequired},
            {"RagdollCreationFailed", AnimationError::RagdollCreationFailed},
            {"RagdollNotFound", AnimationError::RagdollNotFound},
            {"RagdollAlreadyExists", AnimationError::RagdollAlreadyExists},
            {"RagdollInactive", AnimationError::RagdollInactive},
            {"AssetLoadFailed", AnimationError::AssetLoadFailed},
            {"InvalidModelData", AnimationError::InvalidModelData}
        }
    );

    // AnimationWrapMode enum
    lua.new_enum<AnimationWrapMode>("AnimationWrapMode",
        {
            {"Once", AnimationWrapMode::Once},
            {"Loop", AnimationWrapMode::Loop},
            {"PingPong", AnimationWrapMode::PingPong},
            {"ClampForever", AnimationWrapMode::ClampForever}
        }
    );

    // AnimationBlendMode enum
    lua.new_enum<AnimationBlendMode>("AnimationBlendMode",
        {
            {"Override", AnimationBlendMode::Override},
            {"Additive", AnimationBlendMode::Additive}
        }
    );

    // SocketAttachMode enum
    lua.new_enum<SocketAttachMode>("SocketAttachMode",
        {
            {"FollowBone", SocketAttachMode::FollowBone},
            {"FollowPosition", SocketAttachMode::FollowPosition},
            {"FollowRotation", SocketAttachMode::FollowRotation},
            {"WorldSpace", SocketAttachMode::WorldSpace}
        }
    );

    // BoneInfo struct
    lua.new_usertype<BoneInfo>("BoneInfo",
        sol::constructors<BoneInfo()>(),
        "name", &BoneInfo::name,
        "index", &BoneInfo::index,
        "parentIndex", &BoneInfo::parentIndex,
        "inverseBindPose", &BoneInfo::inverseBindPose,
        "localBindPose", &BoneInfo::localBindPose,
        "localPosition", &BoneInfo::localPosition,
        "localRotation", &BoneInfo::localRotation,
        "localScale", &BoneInfo::localScale,
        "isRoot", &BoneInfo::isRoot
    );

    // SkeletonInfo struct
    lua.new_usertype<SkeletonInfo>("SkeletonInfo",
        sol::constructors<SkeletonInfo()>(),
        "boneCount", &SkeletonInfo::boneCount,
        "bones", &SkeletonInfo::bones,
        "rootBoneIndex", &SkeletonInfo::rootBoneIndex,
        "bounds", &SkeletonInfo::bounds,
        "findBone", &SkeletonInfo::findBone,
        "getChildren", &SkeletonInfo::getChildren
    );

    // AnimationClipInfo struct
    lua.new_usertype<AnimationClipInfo>("AnimationClipInfo",
        sol::constructors<AnimationClipInfo()>(),
        "name", &AnimationClipInfo::name,
        "skeleton", &AnimationClipInfo::skeleton,
        "duration", &AnimationClipInfo::duration,
        "ticksPerSecond", &AnimationClipInfo::ticksPerSecond,
        "defaultWrapMode", &AnimationClipInfo::defaultWrapMode,
        "channelCount", &AnimationClipInfo::channelCount,
        "keyframeCount", &AnimationClipInfo::keyframeCount,
        "hasRootMotion", &AnimationClipInfo::hasRootMotion,
        "looping", &AnimationClipInfo::looping
    );

    // AnimationEventDef struct
    lua.new_usertype<AnimationEventDef>("AnimationEventDef",
        sol::constructors<AnimationEventDef()>(),
        "name", &AnimationEventDef::name,
        "time", &AnimationEventDef::time,
        "stringParam", &AnimationEventDef::stringParam,
        "floatParam", &AnimationEventDef::floatParam,
        "intParam", &AnimationEventDef::intParam
    );

    // AnimationEvent struct
    lua.new_usertype<AnimationEvent>("AnimationEvent",
        sol::constructors<AnimationEvent()>(),
        "animator", &AnimationEvent::animator,
        "clip", &AnimationEvent::clip,
        "layer", &AnimationEvent::layer,
        "name", &AnimationEvent::name,
        "clipTime", &AnimationEvent::clipTime,
        "normalizedTime", &AnimationEvent::normalizedTime,
        "stringParam", &AnimationEvent::stringParam,
        "floatParam", &AnimationEvent::floatParam,
        "intParam", &AnimationEvent::intParam
    );

    // SocketDef struct
    lua.new_usertype<SocketDef>("SocketDef",
        sol::constructors<SocketDef()>(),
        "name", &SocketDef::name,
        "boneName", &SocketDef::boneName,
        "localPosition", &SocketDef::localPosition,
        "localRotation", &SocketDef::localRotation,
        "localScale", &SocketDef::localScale,
        "attachMode", &SocketDef::attachMode
    );

    // SocketTransform struct
    lua.new_usertype<SocketTransform>("SocketTransform",
        sol::constructors<SocketTransform()>(),
        "worldMatrix", &SocketTransform::worldMatrix,
        "position", &SocketTransform::position,
        "rotation", &SocketTransform::rotation,
        "scale", &SocketTransform::scale,
        "forward", &SocketTransform::forward,
        "up", &SocketTransform::up,
        "right", &SocketTransform::right
    );

    // AnimationLayerState struct
    lua.new_usertype<AnimationLayerState>("AnimationLayerState",
        sol::constructors<AnimationLayerState()>(),
        "clip", &AnimationLayerState::clip,
        "clipName", &AnimationLayerState::clipName,
        "time", &AnimationLayerState::time,
        "normalizedTime", &AnimationLayerState::normalizedTime,
        "speed", &AnimationLayerState::speed,
        "weight", &AnimationLayerState::weight,
        "fadeWeight", &AnimationLayerState::fadeWeight,
        "wrapMode", &AnimationLayerState::wrapMode,
        "blendMode", &AnimationLayerState::blendMode,
        "playing", &AnimationLayerState::playing,
        "paused", &AnimationLayerState::paused,
        "finished", &AnimationLayerState::finished,
        "effectiveWeight", &AnimationLayerState::effectiveWeight,
        "isActive", &AnimationLayerState::isActive
    );

    // AnimationPlayConfig struct
    lua.new_usertype<AnimationPlayConfig>("AnimationPlayConfig",
        sol::constructors<AnimationPlayConfig()>(),
        "clip", &AnimationPlayConfig::clip,
        "clipName", &AnimationPlayConfig::clipName,
        "startTime", &AnimationPlayConfig::startTime,
        "speed", &AnimationPlayConfig::speed,
        "weight", &AnimationPlayConfig::weight,
        "blendInTime", &AnimationPlayConfig::blendInTime,
        "blendOutTime", &AnimationPlayConfig::blendOutTime,
        "wrapMode", &AnimationPlayConfig::wrapMode,
        "layer", &AnimationPlayConfig::layer,
        "blendMode", &AnimationPlayConfig::blendMode,
        "restartIfSame", &AnimationPlayConfig::restartIfSame
    );

    // IKTwoBoneChain struct
    lua.new_usertype<IKTwoBoneChain>("IKTwoBoneChain",
        sol::constructors<IKTwoBoneChain()>(),
        "name", &IKTwoBoneChain::name,
        "rootBoneName", &IKTwoBoneChain::rootBoneName,
        "midBoneName", &IKTwoBoneChain::midBoneName,
        "tipBoneName", &IKTwoBoneChain::tipBoneName
    );

    // IKTwoBoneTarget struct
    lua.new_usertype<IKTwoBoneTarget>("IKTwoBoneTarget",
        sol::constructors<IKTwoBoneTarget()>(),
        "chainName", &IKTwoBoneTarget::chainName,
        "targetPosition", &IKTwoBoneTarget::targetPosition,
        "poleVector", &IKTwoBoneTarget::poleVector,
        "weight", &IKTwoBoneTarget::weight,
        "enabled", &IKTwoBoneTarget::enabled
    );

    // IKAimConfig struct
    lua.new_usertype<IKAimConfig>("IKAimConfig",
        sol::constructors<IKAimConfig()>(),
        "name", &IKAimConfig::name,
        "boneName", &IKAimConfig::boneName,
        "aimAxis", &IKAimConfig::aimAxis,
        "upAxis", &IKAimConfig::upAxis,
        "horizontalLimit", &IKAimConfig::horizontalLimit,
        "verticalLimit", &IKAimConfig::verticalLimit
    );

    // IKAimTarget struct
    lua.new_usertype<IKAimTarget>("IKAimTarget",
        sol::constructors<IKAimTarget()>(),
        "configName", &IKAimTarget::configName,
        "targetPosition", &IKAimTarget::targetPosition,
        "worldUp", &IKAimTarget::worldUp,
        "weight", &IKAimTarget::weight,
        "enabled", &IKAimTarget::enabled
    );

    // RootMotion struct
    lua.new_usertype<RootMotion>("RootMotion",
        sol::constructors<RootMotion()>(),
        "deltaPosition", &RootMotion::deltaPosition,
        "deltaRotation", &RootMotion::deltaRotation,
        "totalPosition", &RootMotion::totalPosition,
        "totalRotation", &RootMotion::totalRotation,
        "hasTranslation", &RootMotion::hasTranslation,
        "hasRotation", &RootMotion::hasRotation
    );

    // RootMotionConfig struct
    lua.new_usertype<RootMotionConfig>("RootMotionConfig",
        sol::constructors<RootMotionConfig()>(),
        "enabled", &RootMotionConfig::enabled,
        "extractTranslationX", &RootMotionConfig::extractTranslationX,
        "extractTranslationY", &RootMotionConfig::extractTranslationY,
        "extractTranslationZ", &RootMotionConfig::extractTranslationZ,
        "extractRotationY", &RootMotionConfig::extractRotationY,
        "extractRotationXZ", &RootMotionConfig::extractRotationXZ,
        "rootBoneName", &RootMotionConfig::rootBoneName
    );

    // RagdollState struct
    lua.new_usertype<RagdollState>("RagdollState",
        sol::constructors<RagdollState()>(),
        "created", &RagdollState::created,
        "active", &RagdollState::active,
        "blendWeight", &RagdollState::blendWeight,
        "blendTarget", &RagdollState::blendTarget,
        "blendDuration", &RagdollState::blendDuration,
        "blendTime", &RagdollState::blendTime
    );

    // AnimationStats struct
    lua.new_usertype<AnimationStats>("AnimationStats",
        sol::constructors<AnimationStats()>(),
        "skeletonCount", &AnimationStats::skeletonCount,
        "clipCount", &AnimationStats::clipCount,
        "animatorCount", &AnimationStats::animatorCount,
        "socketDefCount", &AnimationStats::socketDefCount,
        "ikChainCount", &AnimationStats::ikChainCount,
        "ragdollCount", &AnimationStats::ragdollCount,
        "animatorsUpdated", &AnimationStats::animatorsUpdated,
        "layersProcessed", &AnimationStats::layersProcessed,
        "samplingJobs", &AnimationStats::samplingJobs,
        "blendingJobs", &AnimationStats::blendingJobs,
        "ikSolves", &AnimationStats::ikSolves,
        "socketQueries", &AnimationStats::socketQueries,
        "eventsDispatched", &AnimationStats::eventsDispatched,
        "ragdollSyncs", &AnimationStats::ragdollSyncs,
        "updateTimeMs", &AnimationStats::updateTimeMs,
        "samplingTimeMs", &AnimationStats::samplingTimeMs,
        "blendingTimeMs", &AnimationStats::blendingTimeMs,
        "ikTimeMs", &AnimationStats::ikTimeMs,
        "ragdollSyncTimeMs", &AnimationStats::ragdollSyncTimeMs,
        "skeletonMemoryBytes", &AnimationStats::skeletonMemoryBytes,
        "clipMemoryBytes", &AnimationStats::clipMemoryBytes,
        "animatorMemoryBytes", &AnimationStats::animatorMemoryBytes,
        "totalMemoryBytes", &AnimationStats::totalMemoryBytes
    );

    //=========================================================================
    // bestow.animation table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table animTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Skeleton Creation (from model data)
    //-------------------------------------------------------------------------

    // Create a skeleton from a loaded model
    // Usage: local skeleton = bestow.animation.createSkeletonFromModel(modelHandle)
    if (assets) {
        animTable["createSkeletonFromModel"] = [&animation, assets, &lua](const AssetHandle& modelHandle) -> sol::object {
            const ModelData* modelData = assets->getModelData(modelHandle);
            if (!modelData) {
                spdlog::warn("[Animation] createSkeletonFromModel: No model data for handle");
                return sol::nil;
            }

            auto result = animation.createSkeleton(*modelData);
            return animResultToLua(lua, result);
        };

        // Load animation clips from a model file into an existing skeleton
        // Usage: local clips = bestow.animation.loadClipsFromModel(skeleton, modelHandle)
        // Returns a table mapping clip names to handles: { idle = handle1, walk = handle2, ... }
        animTable["loadClipsFromModel"] = [&animation, assets, &lua](
            SkeletonHandle skeleton, const AssetHandle& modelHandle) -> sol::object
        {
            const ModelData* modelData = assets->getModelData(modelHandle);
            if (!modelData) {
                spdlog::warn("[Animation] loadClipsFromModel: No model data for handle");
                return sol::nil;
            }

            auto clips = animation.createAnimationClips(skeleton, *modelData);
            if (clips.empty()) {
                return sol::nil;
            }

            // Create a table mapping clip names to handles
            sol::table clipTable = lua.create_table();
            for (std::size_t i = 0; i < clips.size() && i < modelData->animations.size(); ++i) {
                const std::string& name = modelData->animations[i].name;
                clipTable[name] = clips[i];
            }
            return clipTable;
        };

        // Load a single animation clip by name from a model file
        // Usage: local clip = bestow.animation.loadClip(skeleton, ":library:/animations/idle.fbx", "idle")
        // If clipName is nil, uses the first animation in the file
        animTable["loadClip"] = [&animation, assets, &lua](
            SkeletonHandle skeleton, const std::string& path, sol::optional<std::string> clipName) -> sol::object
        {
            AssetHandle modelHandle = assets->loadModel(std::filesystem::path(path));
            if (!assets->isLoaded(modelHandle)) {
                spdlog::warn("[Animation] loadClip: Failed to load '{}'", path);
                return sol::nil;
            }

            const ModelData* modelData = assets->getModelData(modelHandle);
            if (!modelData || modelData->animations.empty()) {
                spdlog::warn("[Animation] loadClip: No animations in '{}'", path);
                return sol::nil;
            }

            auto clips = animation.createAnimationClips(skeleton, *modelData);
            if (clips.empty()) {
                return sol::nil;
            }

            // If a specific clip name was requested, find it
            if (clipName) {
                for (std::size_t i = 0; i < clips.size() && i < modelData->animations.size(); ++i) {
                    if (modelData->animations[i].name == *clipName) {
                        return sol::make_object(lua, clips[i]);
                    }
                }
                spdlog::warn("[Animation] loadClip: Clip '{}' not found in '{}'", *clipName, path);
                return sol::nil;
            }

            // Return the first clip
            return sol::make_object(lua, clips[0]);
        };

        // High-level: Load a character with skeleton, animator, and mesh ready to use
        // Usage: local char = bestow.animation.loadCharacter(":library:/characters/hero.fbx")
        // Returns: { skeleton = handle, animator = handle, clips = { ... }, mesh = handle } or nil
        animTable["loadCharacter"] = [&animation, assets, graphics, &lua](const std::string& path) -> sol::object {
            AssetHandle modelHandle = assets->loadModel(std::filesystem::path(path));
            if (!assets->isLoaded(modelHandle)) {
                spdlog::warn("[Animation] loadCharacter: Failed to load '{}'", path);
                return sol::nil;
            }

            const ModelData* modelData = assets->getModelData(modelHandle);
            if (!modelData) {
                spdlog::warn("[Animation] loadCharacter: No model data for '{}'", path);
                return sol::nil;
            }

            // Create skeleton
            auto skeletonResult = animation.createSkeleton(*modelData);
            if (!skeletonResult) {
                spdlog::warn("[Animation] loadCharacter: Failed to create skeleton from '{}'", path);
                return sol::nil;
            }
            SkeletonHandle skeleton = *skeletonResult;

            // Create animator
            auto animatorResult = animation.createAnimator(skeleton);
            if (!animatorResult) {
                spdlog::warn("[Animation] loadCharacter: Failed to create animator for '{}'", path);
                animation.destroySkeleton(skeleton);
                return sol::nil;
            }
            AnimatorHandle animator = *animatorResult;

            // Load any embedded clips
            sol::table clipTable = lua.create_table();
            if (!modelData->animations.empty()) {
                auto clips = animation.createAnimationClips(skeleton, *modelData);
                for (std::size_t i = 0; i < clips.size() && i < modelData->animations.size(); ++i) {
                    clipTable[modelData->animations[i].name] = clips[i];
                }
            }

            // Create mesh from model data (if graphics system available)
            MeshHandle mesh = 0;
            MaterialHandle material = 0;
            if (graphics && !modelData->meshes.empty()) {
                auto meshResult = graphics->createMeshFromData(modelData->meshes[0]);
                if (meshResult) {
                    mesh = *meshResult;
                }

                // Create material from model data or use default
                if (!modelData->materials.empty()) {
                    auto matResult = graphics->createMaterialsFromModel(*modelData);
                    if (matResult && !matResult->empty()) {
                        material = (*matResult)[0];
                    }
                }
                if (material == 0) {
                    material = graphics->getDefaultPBRMaterial();
                }
            }

            // Return the character structure
            sol::table result = lua.create_table();
            result["skeleton"] = skeleton;
            result["animator"] = animator;
            result["clips"] = clipTable;
            result["modelHandle"] = modelHandle;
            result["mesh"] = mesh;
            result["material"] = material;

            spdlog::info("[Animation] Loaded character '{}' with {} embedded clips, mesh={}",
                        path, clipTable.size(), mesh);
            return result;
        };
    }

    //-------------------------------------------------------------------------
    // Character Rendering (requires graphics system)
    //-------------------------------------------------------------------------

    if (graphics) {
        // Draw an animated character
        // Usage: bestow.animation.drawCharacter(char.animator, char.mesh, char.material, worldMatrix)
        animTable["drawCharacter"] = [&animation, graphics](
            AnimatorHandle animator, MeshHandle mesh, MaterialHandle material, const Mat4& worldMatrix)
        {
            if (!animation.isValidAnimator(animator)) {
                spdlog::warn("[Animation] drawCharacter: Invalid animator");
                return;
            }

            auto boneTransforms = animation.getBoneTransforms(animator);
            graphics->drawSkinnedMesh(mesh, material, worldMatrix, boneTransforms);
        };

        // Alternative: Draw with Transform3D instead of Mat4
        animTable["drawCharacterTransform"] = [&animation, graphics](
            AnimatorHandle animator, MeshHandle mesh, MaterialHandle material, const Transform3D& transform)
        {
            if (!animation.isValidAnimator(animator)) {
                spdlog::warn("[Animation] drawCharacterTransform: Invalid animator");
                return;
            }

            Mat4 worldMatrix = transform.toMatrix();
            auto boneTransforms = animation.getBoneTransforms(animator);
            graphics->drawSkinnedMesh(mesh, material, worldMatrix, boneTransforms);
        };
    }

    //-------------------------------------------------------------------------
    // Skeleton Management
    //-------------------------------------------------------------------------

    animTable["destroySkeleton"] = [&animation](SkeletonHandle skeleton) {
        animation.destroySkeleton(skeleton);
    };

    animTable["isValidSkeleton"] = [&animation](SkeletonHandle skeleton) {
        return animation.isValidSkeleton(skeleton);
    };

    animTable["getSkeletonInfo"] = [&animation](SkeletonHandle skeleton) {
        return animation.getSkeletonInfo(skeleton);
    };

    animTable["findBoneIndex"] = [&animation](SkeletonHandle skeleton, const std::string& boneName) {
        return animation.findBoneIndex(skeleton, boneName);
    };

    animTable["getBoneNames"] = [&animation](SkeletonHandle skeleton) {
        return animation.getBoneNames(skeleton);
    };

    animTable["getBoneCount"] = [&animation](SkeletonHandle skeleton) {
        return animation.getBoneCount(skeleton);
    };

    //-------------------------------------------------------------------------
    // Animation Clip Management
    //-------------------------------------------------------------------------

    animTable["destroyAnimationClip"] = [&animation](AnimationClipHandle clip) {
        animation.destroyAnimationClip(clip);
    };

    animTable["isValidClip"] = [&animation](AnimationClipHandle clip) {
        return animation.isValidClip(clip);
    };

    animTable["getAnimationClipInfo"] = [&animation](AnimationClipHandle clip) {
        return animation.getAnimationClipInfo(clip);
    };

    animTable["findClip"] = [&animation](SkeletonHandle skeleton, const std::string& clipName) {
        return animation.findClip(skeleton, clipName);
    };

    animTable["getClipsForSkeleton"] = [&animation](SkeletonHandle skeleton) {
        return animation.getClipsForSkeleton(skeleton);
    };

    animTable["getClipNames"] = [&animation](SkeletonHandle skeleton) {
        return animation.getClipNames(skeleton);
    };

    //-------------------------------------------------------------------------
    // Animation Events
    //-------------------------------------------------------------------------

    animTable["addClipEvent"] = [&animation, &lua](AnimationClipHandle clip, const AnimationEventDef& event) {
        auto result = animation.addClipEvent(clip, event);
        return animVoidResultToLua(lua, result);
    };

    animTable["removeClipEvent"] = [&animation, &lua](AnimationClipHandle clip, const std::string& eventName) {
        auto result = animation.removeClipEvent(clip, eventName);
        return animVoidResultToLua(lua, result);
    };

    animTable["clearClipEvents"] = [&animation](AnimationClipHandle clip) {
        animation.clearClipEvents(clip);
    };

    animTable["getClipEvents"] = [&animation](AnimationClipHandle clip) {
        return animation.getClipEvents(clip);
    };

    //-------------------------------------------------------------------------
    // Animator Management
    //-------------------------------------------------------------------------

    animTable["createAnimator"] = [&animation, &lua](SkeletonHandle skeleton) -> sol::object {
        auto result = animation.createAnimator(skeleton);
        return animResultToLua(lua, result);
    };

    animTable["destroyAnimator"] = [&animation](AnimatorHandle animator) {
        animation.destroyAnimator(animator);
    };

    animTable["isValidAnimator"] = [&animation](AnimatorHandle animator) {
        return animation.isValidAnimator(animator);
    };

    animTable["getAnimatorSkeleton"] = [&animation](AnimatorHandle animator) {
        return animation.getAnimatorSkeleton(animator);
    };

    //-------------------------------------------------------------------------
    // Animator Playback Control
    //-------------------------------------------------------------------------

    animTable["play"] = sol::overload(
        [&animation](AnimatorHandle animator, AnimationClipHandle clip, sol::optional<float> transitionTime) {
            animation.play(animator, clip, transitionTime.value_or(0.25f));
        },
        [&animation](AnimatorHandle animator, const std::string& clipName, sol::optional<float> transitionTime) {
            animation.play(animator, clipName, transitionTime.value_or(0.25f));
        },
        [&animation](AnimatorHandle animator, const AnimationPlayConfig& config) {
            animation.play(animator, config);
        }
    );

    animTable["stop"] = [&animation](AnimatorHandle animator, sol::optional<float> fadeOutTime) {
        animation.stop(animator, fadeOutTime.value_or(0.0f));
    };

    animTable["stopLayer"] = [&animation](AnimatorHandle animator, std::uint32_t layer, sol::optional<float> fadeOutTime) {
        animation.stopLayer(animator, layer, fadeOutTime.value_or(0.0f));
    };

    animTable["setPaused"] = [&animation](AnimatorHandle animator, bool paused) {
        animation.setPaused(animator, paused);
    };

    animTable["isPaused"] = [&animation](AnimatorHandle animator) {
        return animation.isPaused(animator);
    };

    animTable["setSpeed"] = [&animation](AnimatorHandle animator, float speed) {
        animation.setSpeed(animator, speed);
    };

    animTable["getSpeed"] = [&animation](AnimatorHandle animator) {
        return animation.getSpeed(animator);
    };

    animTable["isPlaying"] = [&animation](AnimatorHandle animator) {
        return animation.isPlaying(animator);
    };

    animTable["isLayerPlaying"] = [&animation](AnimatorHandle animator, std::uint32_t layer) {
        return animation.isLayerPlaying(animator, layer);
    };

    //-------------------------------------------------------------------------
    // Animator Layer Control
    //-------------------------------------------------------------------------

    animTable["getLayerState"] = [&animation](AnimatorHandle animator, std::uint32_t layer) {
        return animation.getLayerState(animator, layer);
    };

    animTable["setLayerWeight"] = [&animation](AnimatorHandle animator, std::uint32_t layer, float weight) {
        animation.setLayerWeight(animator, layer, weight);
    };

    animTable["getLayerWeight"] = [&animation](AnimatorHandle animator, std::uint32_t layer) {
        return animation.getLayerWeight(animator, layer);
    };

    animTable["setLayerBlendMode"] = [&animation](AnimatorHandle animator, std::uint32_t layer, AnimationBlendMode mode) {
        animation.setLayerBlendMode(animator, layer, mode);
    };

    animTable["getLayerCount"] = [&animation](AnimatorHandle animator) {
        return animation.getLayerCount(animator);
    };

    //-------------------------------------------------------------------------
    // Animator Time Control
    //-------------------------------------------------------------------------

    animTable["getCurrentTime"] = [&animation](AnimatorHandle animator, sol::optional<std::uint32_t> layer) {
        return animation.getCurrentTime(animator, layer.value_or(0));
    };

    animTable["getNormalizedTime"] = [&animation](AnimatorHandle animator, sol::optional<std::uint32_t> layer) {
        return animation.getNormalizedTime(animator, layer.value_or(0));
    };

    animTable["setCurrentTime"] = [&animation](AnimatorHandle animator, float time, sol::optional<std::uint32_t> layer) {
        animation.setCurrentTime(animator, time, layer.value_or(0));
    };

    animTable["setNormalizedTime"] = [&animation](AnimatorHandle animator, float normalizedTime, sol::optional<std::uint32_t> layer) {
        animation.setNormalizedTime(animator, normalizedTime, layer.value_or(0));
    };

    animTable["getClipDuration"] = [&animation](AnimatorHandle animator, sol::optional<std::uint32_t> layer) {
        return animation.getClipDuration(animator, layer.value_or(0));
    };

    //-------------------------------------------------------------------------
    // Animator Bone Transforms
    //-------------------------------------------------------------------------

    animTable["getBoneTransform"] = sol::overload(
        [&animation](AnimatorHandle animator, std::uint32_t boneIndex) {
            return animation.getBoneTransform(animator, boneIndex);
        },
        [&animation](AnimatorHandle animator, const std::string& boneName) {
            return animation.getBoneTransform(animator, boneName);
        }
    );

    animTable["getBoneWorldTransform"] = [&animation](AnimatorHandle animator, std::uint32_t boneIndex, const Mat4& entityWorldMatrix) {
        return animation.getBoneWorldTransform(animator, boneIndex, entityWorldMatrix);
    };

    //-------------------------------------------------------------------------
    // Socket System
    //-------------------------------------------------------------------------

    animTable["defineSocket"] = [&animation, &lua](SkeletonHandle skeleton, const SocketDef& def) -> sol::object {
        auto result = animation.defineSocket(skeleton, def);
        return animResultToLua(lua, result);
    };

    animTable["removeSocket"] = sol::overload(
        [&animation](SocketHandle socket) {
            animation.removeSocket(socket);
        },
        [&animation](SkeletonHandle skeleton, const std::string& name) {
            animation.removeSocket(skeleton, name);
        }
    );

    animTable["hasSocket"] = [&animation](SkeletonHandle skeleton, const std::string& socketName) {
        return animation.hasSocket(skeleton, socketName);
    };

    animTable["findSocket"] = [&animation](SkeletonHandle skeleton, const std::string& socketName) {
        return animation.findSocket(skeleton, socketName);
    };

    animTable["getSockets"] = [&animation](SkeletonHandle skeleton) {
        return animation.getSockets(skeleton);
    };

    animTable["getSocketDef"] = [&animation, &lua](SocketHandle socket) -> sol::object {
        auto result = animation.getSocketDef(socket);
        if (result) {
            return sol::make_object(lua, *result);
        }
        return sol::nil;
    };

    animTable["getSocketTransform"] = sol::overload(
        [&animation, &lua](AnimatorHandle animator, const std::string& socketName, const Mat4& entityWorldMatrix) -> sol::object {
            auto result = animation.getSocketTransform(animator, socketName, entityWorldMatrix);
            return animResultToLua(lua, result);
        },
        [&animation, &lua](AnimatorHandle animator, SocketHandle socket, const Mat4& entityWorldMatrix) -> sol::object {
            auto result = animation.getSocketTransform(animator, socket, entityWorldMatrix);
            return animResultToLua(lua, result);
        }
    );

    animTable["setSocketLocalTransform"] = [&animation, &lua](SocketHandle socket, const Vec3& position, const Quat& rotation, sol::optional<Vec3> scale) {
        auto result = animation.setSocketLocalTransform(socket, position, rotation, scale.value_or(Vec3{1.0f}));
        return animVoidResultToLua(lua, result);
    };

    animTable["setSocketEnabled"] = [&animation](SocketHandle socket, bool enabled) {
        animation.setSocketEnabled(socket, enabled);
    };

    animTable["isSocketEnabled"] = [&animation](SocketHandle socket) {
        return animation.isSocketEnabled(socket);
    };

    //-------------------------------------------------------------------------
    // Inverse Kinematics: Chain Definition
    //-------------------------------------------------------------------------

    animTable["defineIKChain"] = [&animation, &lua](SkeletonHandle skeleton, const IKTwoBoneChain& chain) {
        auto result = animation.defineIKChain(skeleton, chain);
        return animVoidResultToLua(lua, result);
    };

    animTable["defineIKAim"] = [&animation, &lua](SkeletonHandle skeleton, const IKAimConfig& config) {
        auto result = animation.defineIKAim(skeleton, config);
        return animVoidResultToLua(lua, result);
    };

    animTable["removeIKChain"] = [&animation](SkeletonHandle skeleton, const std::string& name) {
        animation.removeIKChain(skeleton, name);
    };

    animTable["removeIKAim"] = [&animation](SkeletonHandle skeleton, const std::string& name) {
        animation.removeIKAim(skeleton, name);
    };

    animTable["getIKChainNames"] = [&animation](SkeletonHandle skeleton) {
        return animation.getIKChainNames(skeleton);
    };

    animTable["getIKAimNames"] = [&animation](SkeletonHandle skeleton) {
        return animation.getIKAimNames(skeleton);
    };

    //-------------------------------------------------------------------------
    // Inverse Kinematics: Runtime Control
    //-------------------------------------------------------------------------

    animTable["setIKTarget"] = sol::overload(
        [&animation](AnimatorHandle animator, const IKTwoBoneTarget& target) {
            animation.setIKTarget(animator, target);
        },
        [&animation](AnimatorHandle animator, const IKAimTarget& target) {
            animation.setIKTarget(animator, target);
        }
    );

    animTable["getIKTwoBoneTarget"] = [&animation, &lua](AnimatorHandle animator, const std::string& chainName) -> sol::object {
        auto result = animation.getIKTwoBoneTarget(animator, chainName);
        if (result) {
            return sol::make_object(lua, *result);
        }
        return sol::nil;
    };

    animTable["getIKAimTarget"] = [&animation, &lua](AnimatorHandle animator, const std::string& configName) -> sol::object {
        auto result = animation.getIKAimTarget(animator, configName);
        if (result) {
            return sol::make_object(lua, *result);
        }
        return sol::nil;
    };

    animTable["clearIKTarget"] = [&animation](AnimatorHandle animator, const std::string& targetName) {
        animation.clearIKTarget(animator, targetName);
    };

    animTable["clearAllIKTargets"] = [&animation](AnimatorHandle animator) {
        animation.clearAllIKTargets(animator);
    };

    animTable["setIKWeight"] = [&animation](AnimatorHandle animator, const std::string& targetName, float weight) {
        animation.setIKWeight(animator, targetName, weight);
    };

    animTable["getIKWeight"] = [&animation](AnimatorHandle animator, const std::string& targetName) {
        return animation.getIKWeight(animator, targetName);
    };

    //-------------------------------------------------------------------------
    // Root Motion
    //-------------------------------------------------------------------------

    animTable["setRootMotionConfig"] = [&animation](AnimatorHandle animator, const RootMotionConfig& config) {
        animation.setRootMotionConfig(animator, config);
    };

    animTable["getRootMotionConfig"] = [&animation](AnimatorHandle animator) {
        return animation.getRootMotionConfig(animator);
    };

    animTable["setRootMotionEnabled"] = [&animation](AnimatorHandle animator, bool enabled) {
        animation.setRootMotionEnabled(animator, enabled);
    };

    animTable["isRootMotionEnabled"] = [&animation](AnimatorHandle animator) {
        return animation.isRootMotionEnabled(animator);
    };

    animTable["getRootMotion"] = [&animation](AnimatorHandle animator) {
        return animation.getRootMotion(animator);
    };

    animTable["extractRootMotion"] = [&animation](AnimationClipHandle clip, float fromTime, float toTime) {
        return animation.extractRootMotion(clip, fromTime, toTime);
    };

    animTable["consumeRootMotion"] = [&animation](AnimatorHandle animator) {
        animation.consumeRootMotion(animator);
    };

    //-------------------------------------------------------------------------
    // Ragdoll (without physics parameter - requires separate physics3d call)
    //-------------------------------------------------------------------------

    animTable["hasRagdoll"] = [&animation](Entity entity) {
        return animation.hasRagdoll(entity);
    };

    animTable["getRagdollState"] = [&animation](Entity entity) {
        return animation.getRagdollState(entity);
    };

    animTable["setRagdollBlendWeight"] = [&animation](Entity entity, float weight) {
        animation.setRagdollBlendWeight(entity, weight);
    };

    animTable["getRagdollBlendWeight"] = [&animation](Entity entity) {
        return animation.getRagdollBlendWeight(entity);
    };

    animTable["isRagdollActive"] = [&animation](Entity entity) {
        return animation.isRagdollActive(entity);
    };

    //-------------------------------------------------------------------------
    // Event Subscriptions
    //-------------------------------------------------------------------------

    animTable["subscribeToEvents"] = [&animation](AnimatorHandle animator, sol::function callback) {
        return animation.subscribeToEvents(animator, [callback](const AnimationEvent& event) {
            callback(event);
        });
    };

    animTable["subscribeToComplete"] = [&animation](AnimatorHandle animator, sol::function callback) {
        return animation.subscribeToComplete(animator, [callback](AnimatorHandle anim, AnimationClipHandle clip, std::uint32_t layer) {
            callback(anim, clip, layer);
        });
    };

    animTable["subscribeToLayerChanges"] = [&animation](AnimatorHandle animator, sol::function callback) {
        return animation.subscribeToLayerChanges(animator, [callback](AnimatorHandle anim, std::uint32_t layer, AnimationClipHandle prevClip, AnimationClipHandle newClip) {
            callback(anim, layer, prevClip, newClip);
        });
    };

    animTable["unsubscribe"] = [&animation](SubscriptionId id) {
        animation.unsubscribe(id);
    };

    //-------------------------------------------------------------------------
    // Statistics & Debugging
    //-------------------------------------------------------------------------

    animTable["getStats"] = [&animation]() {
        return animation.getStats();
    };

    animTable["resetFrameStats"] = [&animation]() {
        animation.resetFrameStats();
    };

    animTable["setDebugVisualization"] = [&animation](bool enabled) {
        animation.setDebugVisualization(enabled);
    };

    animTable["isDebugVisualizationEnabled"] = [&animation]() {
        return animation.isDebugVisualizationEnabled();
    };

    //-------------------------------------------------------------------------
    // Handle Constants
    //-------------------------------------------------------------------------

    sol::table handles = lua.create_table();
    handles["InvalidSkeleton"] = AnimationHandles::InvalidSkeleton;
    handles["InvalidClip"] = AnimationHandles::InvalidClip;
    handles["InvalidAnimator"] = AnimationHandles::InvalidAnimator;
    handles["InvalidSocket"] = AnimationHandles::InvalidSocket;

    animTable["Handle"] = handles;

    bestow["animation"] = animTable;
}

}  // namespace bestow
