// bestow-animation/src/AnimationSystem.cpp
// Animation System Full Implementation

module bestow.animation.impl;

import bestow.services;

import std;

namespace bestow {

namespace {

//==========================================================================
// Internal Helper Functions
//==========================================================================

/// Linear interpolation
template<typename T>
T lerp(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}

/// Spherical linear interpolation for quaternions
Quat slerp(const Quat& a, const Quat& b, float t) {
    Quat result = b;
    float dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;

    // If dot < 0, negate one quaternion to take shorter path
    if (dot < 0.0f) {
        result = Quat(-b.w, -b.x, -b.y, -b.z);
        dot = -dot;
    }

    // If quaternions are very close, use linear interpolation
    if (dot > 0.9995f) {
        return glm::normalize(Quat(
            lerp(a.w, result.w, t),
            lerp(a.x, result.x, t),
            lerp(a.y, result.y, t),
            lerp(a.z, result.z, t)
        ));
    }

    // Standard slerp
    float theta0 = std::acos(dot);
    float theta = theta0 * t;
    float sinTheta = std::sin(theta);
    float sinTheta0 = std::sin(theta0);

    float s0 = std::cos(theta) - dot * sinTheta / sinTheta0;
    float s1 = sinTheta / sinTheta0;

    return Quat(
        s0 * a.w + s1 * result.w,
        s0 * a.x + s1 * result.x,
        s0 * a.y + s1 * result.y,
        s0 * a.z + s1 * result.z
    );
}

/// Compose transform matrix from position, rotation, scale
Mat4 composeTransform(const Vec3& position, const Quat& rotation, const Vec3& scale) {
    Mat4 result{1.0f};

    // Scale
    result[0][0] = scale.x;
    result[1][1] = scale.y;
    result[2][2] = scale.z;

    // Rotation
    Mat4 rotMat = glm::mat4_cast(rotation);
    result = rotMat * result;

    // Translation
    result[3][0] = position.x;
    result[3][1] = position.y;
    result[3][2] = position.z;

    return result;
}

/// Decompose transform matrix into position, rotation, scale
void decomposeTransform(const Mat4& matrix, Vec3& position, Quat& rotation, Vec3& scale) {
    position = Vec3(matrix[3]);

    scale.x = glm::length(Vec3(matrix[0]));
    scale.y = glm::length(Vec3(matrix[1]));
    scale.z = glm::length(Vec3(matrix[2]));

    Mat3 rotMat{
        Vec3(matrix[0]) / scale.x,
        Vec3(matrix[1]) / scale.y,
        Vec3(matrix[2]) / scale.z
    };
    rotation = glm::quat_cast(rotMat);
}

/// Wrap time based on wrap mode
float wrapTime(float time, float duration, AnimationWrapMode mode) {
    if (duration <= 0.0f) return 0.0f;

    switch (mode) {
        case AnimationWrapMode::Once:
            return std::clamp(time, 0.0f, duration);

        case AnimationWrapMode::Loop:
            return std::fmod(std::fmod(time, duration) + duration, duration);

        case AnimationWrapMode::PingPong: {
            float t = std::fmod(time, duration * 2.0f);
            if (t < 0) t += duration * 2.0f;
            if (t > duration) t = duration * 2.0f - t;
            return t;
        }

        case AnimationWrapMode::ClampForever:
            return std::max(0.0f, time);
    }
    return time;
}

}  // anonymous namespace

//==========================================================================
// Internal Data Structures
//==========================================================================

struct SkeletonData {
    SkeletonHandle handle = 0;
    std::vector<BoneInfo> bones;
    std::unordered_map<std::string, std::int32_t> boneNameToIndex;
    std::int32_t rootBoneIndex = 0;
    AABB3D bounds;
};

struct AnimationKeyframe {
    float time = 0.0f;
    Vec3 position{0.0f};
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f};
};

struct AnimationChannel {
    std::int32_t boneIndex = -1;
    std::vector<AnimationKeyframe> keyframes;
};

struct AnimationClipData {
    AnimationClipHandle handle = 0;
    SkeletonHandle skeleton = 0;
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 30.0f;
    bool looping = true;
    bool hasRootMotion = false;
    std::vector<AnimationChannel> channels;
    std::vector<AnimationEventDef> events;
};

struct AnimatorLayerData {
    AnimationClipHandle clip = AnimationHandles::InvalidClip;
    std::string clipName;
    float time = 0.0f;
    float speed = 1.0f;
    float weight = 1.0f;
    float fadeWeight = 1.0f;
    float fadeSpeed = 0.0f;  // Weight change per second (for transitions)
    AnimationWrapMode wrapMode = AnimationWrapMode::Loop;
    AnimationBlendMode blendMode = AnimationBlendMode::Override;
    bool playing = false;
    bool paused = false;
    std::set<std::uint32_t> boneMask;  // Empty = all bones
};

struct AnimatorData {
    AnimatorHandle handle = 0;
    SkeletonHandle skeleton = 0;
    std::vector<AnimatorLayerData> layers;
    std::vector<Mat4> boneTransforms;
    std::vector<Mat4> localTransforms;  // Per-bone local transforms
    float globalSpeed = 1.0f;
    bool paused = false;

    // IK targets
    std::unordered_map<std::string, IKTwoBoneTarget> twoBoneTargets;
    std::unordered_map<std::string, IKAimTarget> aimTargets;

    // Root motion
    RootMotionConfig rootMotionConfig;
    RootMotion currentRootMotion;
    Vec3 lastRootPosition{0.0f};
    Quat lastRootRotation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct SocketData {
    SocketHandle handle = 0;
    SkeletonHandle skeleton = 0;
    std::string name;
    std::uint32_t boneIndex = 0;
    Vec3 localPosition{0.0f};
    Quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};
    Vec3 localScale{1.0f};
    SocketAttachMode attachMode = SocketAttachMode::FollowBone;
    bool enabled = true;
};

struct IKChainData {
    std::string name;
    std::int32_t rootBoneIndex = -1;
    std::int32_t midBoneIndex = -1;
    std::int32_t tipBoneIndex = -1;
};

struct IKAimData {
    std::string name;
    std::int32_t boneIndex = -1;
    Vec3 aimAxis{0.0f, 0.0f, 1.0f};
    Vec3 upAxis{0.0f, 1.0f, 0.0f};
    float horizontalLimit = 90.0f;
    float verticalLimit = 60.0f;
};

struct RagdollData {
    Entity entity;
    SkeletonHandle skeleton = 0;
    AnimatorHandle animator = 0;
    RagdollState state;
    RagdollDef definition;
};

struct EventSubscription {
    SubscriptionId id = 0;
    AnimatorHandle animator = 0;
    enum class Type { Event, Complete, LayerChange } type;
    std::variant<
        AnimationEventCallback,
        AnimationCompleteCallback,
        AnimationLayerCallback
    > callback;
};

//==========================================================================
// AnimationSystem Private Data
//==========================================================================

class AnimationSystemImpl {
public:
    // Handle generators
    SkeletonHandle nextSkeletonHandle_ = 1;
    AnimationClipHandle nextClipHandle_ = 1;
    AnimatorHandle nextAnimatorHandle_ = 1;
    SocketHandle nextSocketHandle_ = 1;
    SubscriptionId nextSubscriptionId_ = 1;

    // Storage
    std::unordered_map<SkeletonHandle, SkeletonData> skeletons_;
    std::unordered_map<AnimationClipHandle, AnimationClipData> clips_;
    std::unordered_map<AnimatorHandle, AnimatorData> animators_;
    std::unordered_map<SocketHandle, SocketData> sockets_;
    std::unordered_map<SkeletonHandle, std::vector<IKChainData>> ikChains_;
    std::unordered_map<SkeletonHandle, std::vector<IKAimData>> ikAims_;
    std::unordered_map<Entity, RagdollData> ragdolls_;
    std::vector<EventSubscription> subscriptions_;

    // Skeleton to clip/socket/animator mappings
    std::unordered_map<SkeletonHandle, std::vector<AnimationClipHandle>> skeletonClips_;
    std::unordered_map<SkeletonHandle, std::vector<SocketHandle>> skeletonSockets_;

    // Statistics
    AnimationStats stats_;
    bool debugVisualization_ = false;

    // Pending events to dispatch
    std::vector<AnimationEvent> pendingEvents_;
};

// Static storage for implementation data
static std::unique_ptr<AnimationSystemImpl> impl_;

//==========================================================================
// Lifecycle
//==========================================================================

AnimationSystem::~AnimationSystem() {
    shutdown();
}

bool AnimationSystem::initialize() {
    impl_ = std::make_unique<AnimationSystemImpl>();
    return true;
}

void AnimationSystem::shutdown() {
    if (impl_) {
        impl_->skeletons_.clear();
        impl_->clips_.clear();
        impl_->animators_.clear();
        impl_->sockets_.clear();
        impl_->ikChains_.clear();
        impl_->ikAims_.clear();
        impl_->ragdolls_.clear();
        impl_->subscriptions_.clear();
        impl_.reset();
    }
}

void AnimationSystem::update(DeltaTime dt) {
    if (!impl_) return;

    auto startTime = std::chrono::high_resolution_clock::now();
    impl_->stats_.animatorsUpdated = 0;
    impl_->stats_.layersProcessed = 0;
    impl_->stats_.samplingJobs = 0;
    impl_->stats_.blendingJobs = 0;
    impl_->stats_.eventsDispatched = 0;
    impl_->pendingEvents_.clear();

    // Update all animators
    for (auto& [handle, animator] : impl_->animators_) {
        if (animator.paused) continue;

        auto* skeleton = &impl_->skeletons_[animator.skeleton];
        if (!skeleton) continue;

        impl_->stats_.animatorsUpdated++;

        // Initialize transforms to identity if needed
        if (animator.boneTransforms.size() != skeleton->bones.size()) {
            animator.boneTransforms.resize(skeleton->bones.size(), Mat4{1.0f});
            animator.localTransforms.resize(skeleton->bones.size(), Mat4{1.0f});

            // Set to bind pose
            for (std::uint32_t i = 0; i < skeleton->bones.size(); ++i) {
                animator.localTransforms[i] = skeleton->bones[i].localBindPose;
            }
        }

        // Process each layer
        std::vector<std::vector<Mat4>> layerTransforms;
        std::vector<float> layerWeights;

        for (auto& layer : animator.layers) {
            impl_->stats_.layersProcessed++;

            if (!layer.playing || layer.clip == AnimationHandles::InvalidClip) continue;

            // Update fade
            if (layer.fadeSpeed != 0.0f) {
                layer.fadeWeight += layer.fadeSpeed * dt;
                if (layer.fadeWeight <= 0.0f) {
                    layer.fadeWeight = 0.0f;
                    layer.playing = false;
                    layer.fadeSpeed = 0.0f;
                } else if (layer.fadeWeight >= 1.0f) {
                    layer.fadeWeight = 1.0f;
                    layer.fadeSpeed = 0.0f;
                }
            }

            // Advance time if not paused
            if (!layer.paused) {
                float prevTime = layer.time;
                layer.time += dt * layer.speed * animator.globalSpeed;

                auto* clip = &impl_->clips_[layer.clip];
                if (clip) {
                    // Check for events
                    for (const auto& event : clip->events) {
                        bool crossed = (prevTime <= event.time && layer.time > event.time) ||
                                       (layer.speed < 0 && prevTime >= event.time && layer.time < event.time);
                        if (crossed) {
                            AnimationEvent evt;
                            evt.animator = handle;
                            evt.clip = layer.clip;
                            evt.layer = static_cast<std::uint32_t>(&layer - animator.layers.data());
                            evt.name = event.name;
                            evt.clipTime = event.time;
                            evt.normalizedTime = clip->duration > 0 ? event.time / clip->duration : 0;
                            evt.stringParam = event.stringParam;
                            evt.floatParam = event.floatParam;
                            evt.intParam = event.intParam;
                            impl_->pendingEvents_.push_back(evt);
                        }
                    }

                    // Handle wrap mode
                    float wrappedTime = wrapTime(layer.time, clip->duration, layer.wrapMode);

                    // Check for completion
                    if (layer.wrapMode == AnimationWrapMode::Once &&
                        layer.time >= clip->duration && !layer.paused) {
                        layer.playing = false;
                        layer.time = clip->duration;

                        // Dispatch completion callbacks
                        std::uint32_t layerIdx = static_cast<std::uint32_t>(&layer - animator.layers.data());
                        for (auto& sub : impl_->subscriptions_) {
                            if (sub.animator == handle && sub.type == EventSubscription::Type::Complete) {
                                auto* cb = std::get_if<AnimationCompleteCallback>(&sub.callback);
                                if (cb && *cb) {
                                    (*cb)(handle, layer.clip, layerIdx);
                                }
                            }
                        }
                    }

                    layer.time = wrappedTime;

                    // Sample animation
                    impl_->stats_.samplingJobs++;
                    std::vector<Mat4> sampledTransforms(skeleton->bones.size(), Mat4{1.0f});

                    for (const auto& channel : clip->channels) {
                        if (channel.boneIndex < 0 ||
                            static_cast<std::size_t>(channel.boneIndex) >= skeleton->bones.size()) continue;

                        // Skip if bone is masked out
                        if (!layer.boneMask.empty() &&
                            layer.boneMask.find(static_cast<std::uint32_t>(channel.boneIndex)) == layer.boneMask.end()) {
                            continue;
                        }

                        // Find surrounding keyframes
                        const AnimationKeyframe* prev = nullptr;
                        const AnimationKeyframe* next = nullptr;
                        float t = 0.0f;

                        for (std::size_t k = 0; k < channel.keyframes.size(); ++k) {
                            if (channel.keyframes[k].time <= layer.time) {
                                prev = &channel.keyframes[k];
                            }
                            if (channel.keyframes[k].time >= layer.time && !next) {
                                next = &channel.keyframes[k];
                                break;
                            }
                        }

                        if (!prev && !next && !channel.keyframes.empty()) {
                            prev = &channel.keyframes[0];
                            next = &channel.keyframes[0];
                        } else if (!prev) {
                            prev = next;
                        } else if (!next) {
                            next = prev;
                        }

                        if (prev && next) {
                            float duration = next->time - prev->time;
                            t = duration > 0.0001f ? (layer.time - prev->time) / duration : 0.0f;
                            t = std::clamp(t, 0.0f, 1.0f);

                            Vec3 position = lerp(prev->position, next->position, t);
                            Quat rotation = slerp(prev->rotation, next->rotation, t);
                            Vec3 scale = lerp(prev->scale, next->scale, t);

                            sampledTransforms[channel.boneIndex] = composeTransform(position, rotation, scale);
                        }
                    }

                    float effectiveWeight = layer.weight * layer.fadeWeight;
                    if (effectiveWeight > 0.001f) {
                        layerTransforms.push_back(std::move(sampledTransforms));
                        layerWeights.push_back(effectiveWeight);
                    }
                }
            }
        }

        // Blend layers
        if (!layerTransforms.empty()) {
            impl_->stats_.blendingJobs++;

            // Start with bind pose
            for (std::uint32_t i = 0; i < skeleton->bones.size(); ++i) {
                animator.localTransforms[i] = skeleton->bones[i].localBindPose;
            }

            // Apply layers
            for (std::size_t layerIdx = 0; layerIdx < layerTransforms.size(); ++layerIdx) {
                float weight = layerWeights[layerIdx];
                const auto& transforms = layerTransforms[layerIdx];

                for (std::uint32_t boneIdx = 0; boneIdx < skeleton->bones.size(); ++boneIdx) {
                    // Blend local transforms
                    Vec3 pos1, pos2, scale1, scale2;
                    Quat rot1, rot2;
                    decomposeTransform(animator.localTransforms[boneIdx], pos1, rot1, scale1);
                    decomposeTransform(transforms[boneIdx], pos2, rot2, scale2);

                    Vec3 blendPos = lerp(pos1, pos2, weight);
                    Quat blendRot = slerp(rot1, rot2, weight);
                    Vec3 blendScale = lerp(scale1, scale2, weight);

                    animator.localTransforms[boneIdx] = composeTransform(blendPos, blendRot, blendScale);
                }
            }
        }

        // Apply IK
        // Two-bone IK
        for (const auto& [name, target] : animator.twoBoneTargets) {
            if (!target.enabled || target.weight <= 0.001f) continue;

            // Find chain
            auto it = impl_->ikChains_.find(animator.skeleton);
            if (it == impl_->ikChains_.end()) continue;

            for (const auto& chain : it->second) {
                if (chain.name != target.chainName) continue;
                if (chain.rootBoneIndex < 0 || chain.midBoneIndex < 0 || chain.tipBoneIndex < 0) continue;

                impl_->stats_.ikSolves++;

                // Simple two-bone IK implementation
                // Get current bone positions in model space
                std::vector<Mat4> modelSpace(skeleton->bones.size(), Mat4{1.0f});
                for (std::uint32_t i = 0; i < skeleton->bones.size(); ++i) {
                    std::int32_t parentIdx = skeleton->bones[i].parentIndex;
                    if (parentIdx >= 0) {
                        modelSpace[i] = modelSpace[parentIdx] * animator.localTransforms[i];
                    } else {
                        modelSpace[i] = animator.localTransforms[i];
                    }
                }

                Vec3 rootPos = Vec3(modelSpace[chain.rootBoneIndex][3]);
                Vec3 midPos = Vec3(modelSpace[chain.midBoneIndex][3]);
                Vec3 tipPos = Vec3(modelSpace[chain.tipBoneIndex][3]);

                float upperLen = glm::length(midPos - rootPos);
                float lowerLen = glm::length(tipPos - midPos);
                float totalLen = upperLen + lowerLen;

                Vec3 targetPos = target.targetPosition;
                Vec3 toTarget = targetPos - rootPos;
                float targetDist = glm::length(toTarget);

                // Clamp target distance
                if (targetDist > totalLen * 0.999f) {
                    targetDist = totalLen * 0.999f;
                    targetPos = rootPos + glm::normalize(toTarget) * targetDist;
                }

                // Law of cosines for elbow angle
                float cosAngle = (upperLen * upperLen + targetDist * targetDist - lowerLen * lowerLen) /
                                 (2.0f * upperLen * targetDist);
                cosAngle = std::clamp(cosAngle, -1.0f, 1.0f);
                float angle = std::acos(cosAngle);

                // Calculate new bone orientations (simplified)
                Vec3 targetDir = glm::normalize(toTarget);
                Vec3 poleDir = glm::normalize(target.poleVector - rootPos);

                // Apply weight
                // This is a simplified IK - a full implementation would properly
                // calculate bone rotations and blend with weight
                break;
            }
        }

        // Aim IK
        for (const auto& [name, target] : animator.aimTargets) {
            if (!target.enabled || target.weight <= 0.001f) continue;

            auto it = impl_->ikAims_.find(animator.skeleton);
            if (it == impl_->ikAims_.end()) continue;

            for (const auto& config : it->second) {
                if (config.name != target.configName) continue;
                if (config.boneIndex < 0) continue;

                impl_->stats_.ikSolves++;

                // Calculate aim rotation
                std::vector<Mat4> modelSpace(skeleton->bones.size(), Mat4{1.0f});
                for (std::uint32_t i = 0; i < skeleton->bones.size(); ++i) {
                    std::int32_t parentIdx = skeleton->bones[i].parentIndex;
                    if (parentIdx >= 0) {
                        modelSpace[i] = modelSpace[parentIdx] * animator.localTransforms[i];
                    } else {
                        modelSpace[i] = animator.localTransforms[i];
                    }
                }

                Vec3 bonePos = Vec3(modelSpace[config.boneIndex][3]);
                Vec3 toTarget = glm::normalize(target.targetPosition - bonePos);

                // Calculate desired rotation to aim at target
                Quat aimRot = glm::quatLookAt(toTarget, target.worldUp);

                // Apply angle limits (simplified)
                // Full implementation would decompose into euler angles and clamp

                // Blend with weight
                Vec3 curPos;
                Quat curRot;
                Vec3 curScale;
                decomposeTransform(animator.localTransforms[config.boneIndex], curPos, curRot, curScale);

                Quat blendedRot = slerp(curRot, aimRot, target.weight);
                animator.localTransforms[config.boneIndex] = composeTransform(curPos, blendedRot, curScale);

                break;
            }
        }

        // Calculate world-space bone transforms
        for (std::uint32_t i = 0; i < skeleton->bones.size(); ++i) {
            std::int32_t parentIdx = skeleton->bones[i].parentIndex;
            if (parentIdx >= 0 && parentIdx < static_cast<std::int32_t>(skeleton->bones.size())) {
                animator.boneTransforms[i] = animator.boneTransforms[parentIdx] * animator.localTransforms[i];
            } else {
                animator.boneTransforms[i] = animator.localTransforms[i];
            }
        }

        // Apply inverse bind pose to get skinning matrices
        for (std::uint32_t i = 0; i < skeleton->bones.size(); ++i) {
            animator.boneTransforms[i] = animator.boneTransforms[i] * skeleton->bones[i].inverseBindPose;
        }

        // Extract root motion if enabled
        if (animator.rootMotionConfig.enabled) {
            std::int32_t rootIdx = animator.rootMotionConfig.rootBoneIndex;
            if (rootIdx < 0) rootIdx = skeleton->rootBoneIndex;

            if (rootIdx >= 0 && rootIdx < static_cast<std::int32_t>(animator.localTransforms.size())) {
                Vec3 pos;
                Quat rot;
                Vec3 scale;
                decomposeTransform(animator.localTransforms[rootIdx], pos, rot, scale);

                animator.currentRootMotion.deltaPosition = pos - animator.lastRootPosition;
                animator.currentRootMotion.deltaRotation = rot * glm::inverse(animator.lastRootRotation);
                animator.currentRootMotion.totalPosition = pos;
                animator.currentRootMotion.totalRotation = rot;

                // Apply extraction mask
                if (!animator.rootMotionConfig.extractTranslationX) {
                    animator.currentRootMotion.deltaPosition.x = 0.0f;
                }
                if (!animator.rootMotionConfig.extractTranslationY) {
                    animator.currentRootMotion.deltaPosition.y = 0.0f;
                }
                if (!animator.rootMotionConfig.extractTranslationZ) {
                    animator.currentRootMotion.deltaPosition.z = 0.0f;
                }

                animator.currentRootMotion.hasTranslation =
                    glm::length(animator.currentRootMotion.deltaPosition) > 0.0001f;
                animator.currentRootMotion.hasRotation = true;

                animator.lastRootPosition = pos;
                animator.lastRootRotation = rot;
            }
        }
    }

    // Update ragdoll blending
    for (auto& [entity, ragdoll] : impl_->ragdolls_) {
        if (ragdoll.state.blendDuration > 0.0f && ragdoll.state.blendTime < ragdoll.state.blendDuration) {
            ragdoll.state.blendTime += dt;
            float t = ragdoll.state.blendTime / ragdoll.state.blendDuration;
            t = std::clamp(t, 0.0f, 1.0f);
            ragdoll.state.blendWeight = lerp(
                ragdoll.state.blendWeight,
                ragdoll.state.blendTarget,
                t
            );
            if (t >= 1.0f) {
                ragdoll.state.blendWeight = ragdoll.state.blendTarget;
                ragdoll.state.active = ragdoll.state.blendTarget > 0.5f;
            }
        }
        impl_->stats_.ragdollSyncs++;
    }

    // Dispatch events
    for (const auto& evt : impl_->pendingEvents_) {
        for (auto& sub : impl_->subscriptions_) {
            if (sub.animator == evt.animator && sub.type == EventSubscription::Type::Event) {
                auto* cb = std::get_if<AnimationEventCallback>(&sub.callback);
                if (cb && *cb) {
                    (*cb)(evt);
                    impl_->stats_.eventsDispatched++;
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    impl_->stats_.updateTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

//==========================================================================
// Skeleton Management
//==========================================================================

Result<SkeletonHandle, AnimationError> AnimationSystem::createSkeleton(
    const ModelData& modelData) {
    if (!impl_) return std::unexpected(AnimationError::InvalidModelData);

    if (modelData.bones.empty()) {
        return std::unexpected(AnimationError::EmptyBoneData);
    }

    std::vector<BoneInfo> bones;
    bones.reserve(modelData.bones.size());

    for (std::size_t i = 0; i < modelData.bones.size(); ++i) {
        const auto& src = modelData.bones[i];
        BoneInfo bone;
        bone.name = src.name;
        bone.index = static_cast<std::int32_t>(i);
        bone.parentIndex = src.parentIndex;

        // Copy offset matrix (inverse bind pose)
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                bone.inverseBindPose[c][r] = src.offsetMatrix[r * 4 + c];
            }
        }

        // Calculate local bind pose (inverse of inverse bind)
        bone.localBindPose = glm::inverse(bone.inverseBindPose);
        decomposeTransform(bone.localBindPose, bone.localPosition, bone.localRotation, bone.localScale);

        bones.push_back(std::move(bone));
    }

    return createSkeleton(bones);
}

Result<SkeletonHandle, AnimationError> AnimationSystem::createSkeleton(
    std::span<const BoneInfo> bones) {
    if (!impl_) return std::unexpected(AnimationError::InvalidModelData);

    if (bones.empty()) {
        return std::unexpected(AnimationError::EmptyBoneData);
    }

    SkeletonHandle handle = impl_->nextSkeletonHandle_++;
    SkeletonData& data = impl_->skeletons_[handle];
    data.handle = handle;
    data.bones.assign(bones.begin(), bones.end());

    // Build name lookup
    for (std::size_t i = 0; i < data.bones.size(); ++i) {
        data.boneNameToIndex[data.bones[i].name] = static_cast<std::int32_t>(i);
        data.bones[i].index = static_cast<std::int32_t>(i);

        // Find root
        if (data.bones[i].parentIndex < 0) {
            data.rootBoneIndex = static_cast<std::int32_t>(i);
        }
    }

    impl_->stats_.skeletonCount++;
    return handle;
}

void AnimationSystem::destroySkeleton(SkeletonHandle skeleton) {
    if (!impl_ || skeleton == AnimationHandles::InvalidSkeleton) return;

    // Destroy associated clips
    if (auto it = impl_->skeletonClips_.find(skeleton); it != impl_->skeletonClips_.end()) {
        for (auto clip : it->second) {
            impl_->clips_.erase(clip);
            impl_->stats_.clipCount--;
        }
        impl_->skeletonClips_.erase(it);
    }

    // Destroy associated sockets
    if (auto it = impl_->skeletonSockets_.find(skeleton); it != impl_->skeletonSockets_.end()) {
        for (auto socket : it->second) {
            impl_->sockets_.erase(socket);
            impl_->stats_.socketDefCount--;
        }
        impl_->skeletonSockets_.erase(it);
    }

    // Destroy associated animators
    std::vector<AnimatorHandle> toRemove;
    for (const auto& [handle, animator] : impl_->animators_) {
        if (animator.skeleton == skeleton) {
            toRemove.push_back(handle);
        }
    }
    for (auto handle : toRemove) {
        impl_->animators_.erase(handle);
        impl_->stats_.animatorCount--;
    }

    // Remove IK definitions
    impl_->ikChains_.erase(skeleton);
    impl_->ikAims_.erase(skeleton);

    impl_->skeletons_.erase(skeleton);
    impl_->stats_.skeletonCount--;
}

bool AnimationSystem::isValidSkeleton(SkeletonHandle skeleton) const {
    if (!impl_) return false;
    return impl_->skeletons_.find(skeleton) != impl_->skeletons_.end();
}

SkeletonInfo AnimationSystem::getSkeletonInfo(SkeletonHandle skeleton) const {
    SkeletonInfo info;
    if (!impl_) return info;

    auto it = impl_->skeletons_.find(skeleton);
    if (it == impl_->skeletons_.end()) return info;

    const auto& data = it->second;
    info.boneCount = static_cast<std::uint32_t>(data.bones.size());
    info.bones = data.bones;
    info.rootBoneIndex = data.rootBoneIndex;
    info.bounds = data.bounds;

    return info;
}

std::int32_t AnimationSystem::findBoneIndex(
    SkeletonHandle skeleton, std::string_view boneName) const {
    if (!impl_) return -1;

    auto it = impl_->skeletons_.find(skeleton);
    if (it == impl_->skeletons_.end()) return -1;

    auto nameIt = it->second.boneNameToIndex.find(std::string(boneName));
    if (nameIt == it->second.boneNameToIndex.end()) return -1;

    return nameIt->second;
}

std::vector<std::string> AnimationSystem::getBoneNames(SkeletonHandle skeleton) const {
    std::vector<std::string> names;
    if (!impl_) return names;

    auto it = impl_->skeletons_.find(skeleton);
    if (it == impl_->skeletons_.end()) return names;

    names.reserve(it->second.bones.size());
    for (const auto& bone : it->second.bones) {
        names.push_back(bone.name);
    }

    return names;
}

std::uint32_t AnimationSystem::getBoneCount(SkeletonHandle skeleton) const {
    if (!impl_) return 0;

    auto it = impl_->skeletons_.find(skeleton);
    if (it == impl_->skeletons_.end()) return 0;

    return static_cast<std::uint32_t>(it->second.bones.size());
}

//==========================================================================
// Animation Clip Management
//==========================================================================

Result<AnimationClipHandle, AnimationError> AnimationSystem::createAnimationClip(
    SkeletonHandle skeleton, const ModelData& modelData, std::string_view clipName) {
    if (!impl_) return std::unexpected(AnimationError::InvalidModelData);

    if (!isValidSkeleton(skeleton)) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    // Find animation by name
    const ModelData::Animation* srcAnim = nullptr;
    for (const auto& anim : modelData.animations) {
        if (anim.name == clipName) {
            srcAnim = &anim;
            break;
        }
    }

    if (!srcAnim) {
        return std::unexpected(AnimationError::ClipNotFound);
    }

    AnimationClipHandle handle = impl_->nextClipHandle_++;
    AnimationClipData& data = impl_->clips_[handle];
    data.handle = handle;
    data.skeleton = skeleton;
    data.name = std::string(clipName);
    data.duration = srcAnim->duration;
    data.ticksPerSecond = srcAnim->ticksPerSecond;
    data.looping = true;

    // Convert channels
    data.channels.reserve(srcAnim->channels.size());
    for (const auto& srcChannel : srcAnim->channels) {
        AnimationChannel channel;
        channel.boneIndex = srcChannel.boneIndex;
        channel.keyframes.reserve(srcChannel.keyframes.size());

        for (const auto& srcKey : srcChannel.keyframes) {
            AnimationKeyframe key;
            key.time = srcKey.time;
            key.position = Vec3(srcKey.translation[0], srcKey.translation[1], srcKey.translation[2]);
            key.rotation = Quat(srcKey.rotation[3], srcKey.rotation[0], srcKey.rotation[1], srcKey.rotation[2]);
            key.scale = Vec3(srcKey.scale[0], srcKey.scale[1], srcKey.scale[2]);
            channel.keyframes.push_back(key);
        }

        data.channels.push_back(std::move(channel));
    }

    impl_->skeletonClips_[skeleton].push_back(handle);
    impl_->stats_.clipCount++;

    return handle;
}

std::vector<AnimationClipHandle> AnimationSystem::createAnimationClips(
    SkeletonHandle skeleton, const ModelData& modelData) {
    std::vector<AnimationClipHandle> handles;
    if (!impl_) return handles;

    for (const auto& anim : modelData.animations) {
        auto result = createAnimationClip(skeleton, modelData, anim.name);
        if (result) {
            handles.push_back(*result);
        }
    }

    return handles;
}

void AnimationSystem::destroyAnimationClip(AnimationClipHandle clip) {
    if (!impl_ || clip == AnimationHandles::InvalidClip) return;

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return;

    // Remove from skeleton mapping
    auto skeletonIt = impl_->skeletonClips_.find(it->second.skeleton);
    if (skeletonIt != impl_->skeletonClips_.end()) {
        auto& clips = skeletonIt->second;
        clips.erase(std::remove(clips.begin(), clips.end(), clip), clips.end());
    }

    impl_->clips_.erase(it);
    impl_->stats_.clipCount--;
}

bool AnimationSystem::isValidClip(AnimationClipHandle clip) const {
    if (!impl_) return false;
    return impl_->clips_.find(clip) != impl_->clips_.end();
}

AnimationClipInfo AnimationSystem::getAnimationClipInfo(AnimationClipHandle clip) const {
    AnimationClipInfo info;
    if (!impl_) return info;

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return info;

    const auto& data = it->second;
    info.name = data.name;
    info.skeleton = data.skeleton;
    info.duration = data.duration;
    info.ticksPerSecond = data.ticksPerSecond;
    info.defaultWrapMode = data.looping ? AnimationWrapMode::Loop : AnimationWrapMode::Once;
    info.channelCount = static_cast<std::uint32_t>(data.channels.size());
    info.hasRootMotion = data.hasRootMotion;
    info.looping = data.looping;

    std::uint32_t keyframeCount = 0;
    for (const auto& ch : data.channels) {
        keyframeCount += static_cast<std::uint32_t>(ch.keyframes.size());
    }
    info.keyframeCount = keyframeCount;

    return info;
}

AnimationClipHandle AnimationSystem::findClip(
    SkeletonHandle skeleton, std::string_view clipName) const {
    if (!impl_) return AnimationHandles::InvalidClip;

    auto it = impl_->skeletonClips_.find(skeleton);
    if (it == impl_->skeletonClips_.end()) return AnimationHandles::InvalidClip;

    for (auto clipHandle : it->second) {
        auto clipIt = impl_->clips_.find(clipHandle);
        if (clipIt != impl_->clips_.end() && clipIt->second.name == clipName) {
            return clipHandle;
        }
    }

    return AnimationHandles::InvalidClip;
}

std::vector<AnimationClipHandle> AnimationSystem::getClipsForSkeleton(
    SkeletonHandle skeleton) const {
    if (!impl_) return {};

    auto it = impl_->skeletonClips_.find(skeleton);
    if (it == impl_->skeletonClips_.end()) return {};

    return it->second;
}

std::vector<std::string> AnimationSystem::getClipNames(SkeletonHandle skeleton) const {
    std::vector<std::string> names;
    if (!impl_) return names;

    auto clips = getClipsForSkeleton(skeleton);
    names.reserve(clips.size());

    for (auto clip : clips) {
        auto it = impl_->clips_.find(clip);
        if (it != impl_->clips_.end()) {
            names.push_back(it->second.name);
        }
    }

    return names;
}

//==========================================================================
// Animation Events
//==========================================================================

Result<void, AnimationError> AnimationSystem::addClipEvent(
    AnimationClipHandle clip, const AnimationEventDef& event) {
    if (!impl_) return std::unexpected(AnimationError::InvalidClip);

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return std::unexpected(AnimationError::InvalidClip);

    it->second.events.push_back(event);
    return {};
}

Result<void, AnimationError> AnimationSystem::removeClipEvent(
    AnimationClipHandle clip, std::string_view eventName) {
    if (!impl_) return std::unexpected(AnimationError::InvalidClip);

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return std::unexpected(AnimationError::InvalidClip);

    auto& events = it->second.events;
    events.erase(
        std::remove_if(events.begin(), events.end(),
            [&](const auto& e) { return e.name == eventName; }),
        events.end()
    );

    return {};
}

void AnimationSystem::clearClipEvents(AnimationClipHandle clip) {
    if (!impl_) return;

    auto it = impl_->clips_.find(clip);
    if (it != impl_->clips_.end()) {
        it->second.events.clear();
    }
}

std::vector<AnimationEventDef> AnimationSystem::getClipEvents(
    AnimationClipHandle clip) const {
    if (!impl_) return {};

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return {};

    return it->second.events;
}

//==========================================================================
// Animator Management
//==========================================================================

Result<AnimatorHandle, AnimationError> AnimationSystem::createAnimator(
    SkeletonHandle skeleton) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSkeleton);

    if (!isValidSkeleton(skeleton)) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    AnimatorHandle handle = impl_->nextAnimatorHandle_++;
    AnimatorData& data = impl_->animators_[handle];
    data.handle = handle;
    data.skeleton = skeleton;

    // Initialize with one layer
    data.layers.resize(1);

    // Initialize bone transforms
    const auto& skelData = impl_->skeletons_[skeleton];
    data.boneTransforms.resize(skelData.bones.size(), Mat4{1.0f});
    data.localTransforms.resize(skelData.bones.size(), Mat4{1.0f});

    // Set to bind pose
    for (std::size_t i = 0; i < skelData.bones.size(); ++i) {
        data.localTransforms[i] = skelData.bones[i].localBindPose;
    }

    impl_->stats_.animatorCount++;
    return handle;
}

void AnimationSystem::destroyAnimator(AnimatorHandle animator) {
    if (!impl_ || animator == AnimationHandles::InvalidAnimator) return;

    // Remove subscriptions
    impl_->subscriptions_.erase(
        std::remove_if(impl_->subscriptions_.begin(), impl_->subscriptions_.end(),
            [animator](const auto& sub) { return sub.animator == animator; }),
        impl_->subscriptions_.end()
    );

    impl_->animators_.erase(animator);
    impl_->stats_.animatorCount--;
}

bool AnimationSystem::isValidAnimator(AnimatorHandle animator) const {
    if (!impl_) return false;
    return impl_->animators_.find(animator) != impl_->animators_.end();
}

SkeletonHandle AnimationSystem::getAnimatorSkeleton(AnimatorHandle animator) const {
    if (!impl_) return AnimationHandles::InvalidSkeleton;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return AnimationHandles::InvalidSkeleton;

    return it->second.skeleton;
}

//==========================================================================
// Animator Playback Control
//==========================================================================

void AnimationSystem::play(AnimatorHandle animator, AnimationClipHandle clip,
                           float transitionTime) {
    AnimationPlayConfig config;
    config.clip = clip;
    config.blendInTime = transitionTime;
    config.layer = 0;
    play(animator, config);
}

void AnimationSystem::play(AnimatorHandle animator, std::string_view clipName,
                           float transitionTime) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    auto clip = findClip(it->second.skeleton, clipName);
    if (clip != AnimationHandles::InvalidClip) {
        play(animator, clip, transitionTime);
    }
}

void AnimationSystem::play(AnimatorHandle animator, const AnimationPlayConfig& config) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    auto& animData = it->second;

    // Resolve clip
    AnimationClipHandle clip = config.clip;
    if (clip == AnimationHandles::InvalidClip && !config.clipName.empty()) {
        clip = findClip(animData.skeleton, config.clipName);
    }
    if (clip == AnimationHandles::InvalidClip) return;

    // Ensure layer exists
    while (animData.layers.size() <= config.layer) {
        animData.layers.push_back(AnimatorLayerData{});
    }

    auto& layer = animData.layers[config.layer];

    // Check if same clip is already playing
    if (layer.clip == clip && layer.playing && !config.restartIfSame) {
        return;
    }

    AnimationClipHandle oldClip = layer.clip;

    // Start fade out of current animation
    if (layer.playing && config.blendInTime > 0.0f) {
        // Fade out current layer
        layer.fadeSpeed = -1.0f / config.blendInTime;
    }

    // Set up new animation
    layer.clip = clip;
    layer.clipName = impl_->clips_[clip].name;
    layer.time = config.startTime;
    layer.speed = config.speed;
    layer.weight = config.weight;
    layer.wrapMode = config.wrapMode;
    layer.blendMode = config.blendMode;
    layer.playing = true;
    layer.paused = false;

    if (config.blendInTime > 0.0f) {
        layer.fadeWeight = 0.0f;
        layer.fadeSpeed = 1.0f / config.blendInTime;
    } else {
        layer.fadeWeight = 1.0f;
        layer.fadeSpeed = 0.0f;
    }

    // Dispatch layer change callbacks
    for (auto& sub : impl_->subscriptions_) {
        if (sub.animator == animator && sub.type == EventSubscription::Type::LayerChange) {
            auto* cb = std::get_if<AnimationLayerCallback>(&sub.callback);
            if (cb && *cb) {
                (*cb)(animator, config.layer, oldClip, clip);
            }
        }
    }
}

void AnimationSystem::stop(AnimatorHandle animator, float fadeOutTime) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    for (std::uint32_t i = 0; i < it->second.layers.size(); ++i) {
        stopLayer(animator, i, fadeOutTime);
    }
}

void AnimationSystem::stopLayer(AnimatorHandle animator, std::uint32_t layer,
                                float fadeOutTime) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    if (layer >= it->second.layers.size()) return;

    auto& layerData = it->second.layers[layer];
    if (!layerData.playing) return;

    if (fadeOutTime > 0.0f) {
        layerData.fadeSpeed = -layerData.fadeWeight / fadeOutTime;
    } else {
        layerData.playing = false;
        layerData.fadeWeight = 0.0f;
        layerData.fadeSpeed = 0.0f;
    }
}

void AnimationSystem::setPaused(AnimatorHandle animator, bool paused) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.paused = paused;
    }
}

bool AnimationSystem::isPaused(AnimatorHandle animator) const {
    if (!impl_) return false;

    auto it = impl_->animators_.find(animator);
    return it != impl_->animators_.end() && it->second.paused;
}

void AnimationSystem::setSpeed(AnimatorHandle animator, float speed) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.globalSpeed = speed;
    }
}

float AnimationSystem::getSpeed(AnimatorHandle animator) const {
    if (!impl_) return 1.0f;

    auto it = impl_->animators_.find(animator);
    return it != impl_->animators_.end() ? it->second.globalSpeed : 1.0f;
}

bool AnimationSystem::isPlaying(AnimatorHandle animator) const {
    if (!impl_) return false;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return false;

    for (const auto& layer : it->second.layers) {
        if (layer.playing) return true;
    }
    return false;
}

bool AnimationSystem::isLayerPlaying(AnimatorHandle animator, std::uint32_t layer) const {
    if (!impl_) return false;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return false;

    if (layer >= it->second.layers.size()) return false;
    return it->second.layers[layer].playing;
}

//==========================================================================
// Animator Layer Control
//==========================================================================

AnimationLayerState AnimationSystem::getLayerState(
    AnimatorHandle animator, std::uint32_t layer) const {
    AnimationLayerState state;
    if (!impl_) return state;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return state;

    if (layer >= it->second.layers.size()) return state;

    const auto& layerData = it->second.layers[layer];
    state.clip = layerData.clip;
    state.clipName = layerData.clipName;
    state.time = layerData.time;
    state.speed = layerData.speed;
    state.weight = layerData.weight;
    state.fadeWeight = layerData.fadeWeight;
    state.wrapMode = layerData.wrapMode;
    state.blendMode = layerData.blendMode;
    state.playing = layerData.playing;
    state.paused = layerData.paused;

    // Calculate normalized time
    auto clipIt = impl_->clips_.find(layerData.clip);
    if (clipIt != impl_->clips_.end() && clipIt->second.duration > 0) {
        state.normalizedTime = layerData.time / clipIt->second.duration;
    }

    return state;
}

void AnimationSystem::setLayerWeight(AnimatorHandle animator, std::uint32_t layer,
                                     float weight) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    if (layer < it->second.layers.size()) {
        it->second.layers[layer].weight = std::clamp(weight, 0.0f, 1.0f);
    }
}

float AnimationSystem::getLayerWeight(AnimatorHandle animator, std::uint32_t layer) const {
    if (!impl_) return 1.0f;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return 1.0f;

    if (layer >= it->second.layers.size()) return 1.0f;
    return it->second.layers[layer].weight;
}

void AnimationSystem::setLayerBlendMode(AnimatorHandle animator, std::uint32_t layer,
                                        AnimationBlendMode mode) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    if (layer < it->second.layers.size()) {
        it->second.layers[layer].blendMode = mode;
    }
}

std::uint32_t AnimationSystem::getLayerCount(AnimatorHandle animator) const {
    if (!impl_) return 0;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return 0;

    return static_cast<std::uint32_t>(it->second.layers.size());
}

void AnimationSystem::setLayerBoneMask(AnimatorHandle animator, std::uint32_t layer,
                                       const std::set<std::uint32_t>& boneMask) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    while (it->second.layers.size() <= layer) {
        it->second.layers.push_back(AnimatorLayerData{});
    }

    it->second.layers[layer].boneMask = boneMask;
}

//==========================================================================
// Animator Time Control
//==========================================================================

float AnimationSystem::getCurrentTime(AnimatorHandle animator, std::uint32_t layer) const {
    if (!impl_) return 0.0f;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return 0.0f;

    if (layer >= it->second.layers.size()) return 0.0f;
    return it->second.layers[layer].time;
}

float AnimationSystem::getNormalizedTime(AnimatorHandle animator, std::uint32_t layer) const {
    if (!impl_) return 0.0f;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return 0.0f;

    if (layer >= it->second.layers.size()) return 0.0f;

    const auto& layerData = it->second.layers[layer];
    auto clipIt = impl_->clips_.find(layerData.clip);
    if (clipIt == impl_->clips_.end() || clipIt->second.duration <= 0) return 0.0f;

    return layerData.time / clipIt->second.duration;
}

void AnimationSystem::setCurrentTime(AnimatorHandle animator, float time,
                                     std::uint32_t layer) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    if (layer < it->second.layers.size()) {
        it->second.layers[layer].time = time;
    }
}

void AnimationSystem::setNormalizedTime(AnimatorHandle animator, float normalizedTime,
                                        std::uint32_t layer) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    if (layer >= it->second.layers.size()) return;

    const auto& layerData = it->second.layers[layer];
    auto clipIt = impl_->clips_.find(layerData.clip);
    if (clipIt != impl_->clips_.end()) {
        setCurrentTime(animator, normalizedTime * clipIt->second.duration, layer);
    }
}

float AnimationSystem::getClipDuration(AnimatorHandle animator, std::uint32_t layer) const {
    if (!impl_) return 0.0f;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return 0.0f;

    if (layer >= it->second.layers.size()) return 0.0f;

    auto clipIt = impl_->clips_.find(it->second.layers[layer].clip);
    if (clipIt == impl_->clips_.end()) return 0.0f;

    return clipIt->second.duration;
}

//==========================================================================
// Animator Bone Transforms
//==========================================================================

std::span<const Mat4> AnimationSystem::getBoneTransforms(AnimatorHandle animator) const {
    if (!impl_) return {};

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return {};

    return it->second.boneTransforms;
}

Mat4 AnimationSystem::getBoneTransform(AnimatorHandle animator,
                                       std::uint32_t boneIndex) const {
    if (!impl_) return Mat4{1.0f};

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return Mat4{1.0f};

    if (boneIndex >= it->second.boneTransforms.size()) return Mat4{1.0f};
    return it->second.boneTransforms[boneIndex];
}

Mat4 AnimationSystem::getBoneTransform(AnimatorHandle animator,
                                       std::string_view boneName) const {
    if (!impl_) return Mat4{1.0f};

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return Mat4{1.0f};

    std::int32_t boneIndex = findBoneIndex(it->second.skeleton, boneName);
    if (boneIndex < 0) return Mat4{1.0f};

    return getBoneTransform(animator, static_cast<std::uint32_t>(boneIndex));
}

Mat4 AnimationSystem::getBoneWorldTransform(AnimatorHandle animator,
                                            std::uint32_t boneIndex,
                                            const Mat4& entityWorldMatrix) const {
    return entityWorldMatrix * getBoneTransform(animator, boneIndex);
}

//==========================================================================
// Socket System
//==========================================================================

Result<SocketHandle, AnimationError> AnimationSystem::defineSocket(
    SkeletonHandle skeleton, const SocketDef& def) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSkeleton);

    if (!isValidSkeleton(skeleton)) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    // Check if socket already exists
    if (hasSocket(skeleton, def.name)) {
        return std::unexpected(AnimationError::SocketAlreadyExists);
    }

    // Find bone index
    std::int32_t boneIndex = findBoneIndex(skeleton, def.boneName);
    if (boneIndex < 0) {
        return std::unexpected(AnimationError::SocketBoneNotFound);
    }

    SocketHandle handle = impl_->nextSocketHandle_++;
    SocketData& data = impl_->sockets_[handle];
    data.handle = handle;
    data.skeleton = skeleton;
    data.name = def.name;
    data.boneIndex = static_cast<std::uint32_t>(boneIndex);
    data.localPosition = def.localPosition;
    data.localRotation = def.localRotation;
    data.localScale = def.localScale;
    data.attachMode = def.attachMode;
    data.enabled = true;

    impl_->skeletonSockets_[skeleton].push_back(handle);
    impl_->stats_.socketDefCount++;

    return handle;
}

std::vector<Result<SocketHandle, AnimationError>> AnimationSystem::defineSockets(
    SkeletonHandle skeleton, std::span<const SocketDef> defs) {
    std::vector<Result<SocketHandle, AnimationError>> results;
    results.reserve(defs.size());

    for (const auto& def : defs) {
        results.push_back(defineSocket(skeleton, def));
    }

    return results;
}

void AnimationSystem::removeSocket(SocketHandle socket) {
    if (!impl_ || socket == AnimationHandles::InvalidSocket) return;

    auto it = impl_->sockets_.find(socket);
    if (it == impl_->sockets_.end()) return;

    // Remove from skeleton mapping
    auto skeletonIt = impl_->skeletonSockets_.find(it->second.skeleton);
    if (skeletonIt != impl_->skeletonSockets_.end()) {
        auto& sockets = skeletonIt->second;
        sockets.erase(std::remove(sockets.begin(), sockets.end(), socket), sockets.end());
    }

    impl_->sockets_.erase(it);
    impl_->stats_.socketDefCount--;
}

void AnimationSystem::removeSocket(SkeletonHandle skeleton, std::string_view name) {
    SocketHandle socket = findSocket(skeleton, name);
    if (socket != AnimationHandles::InvalidSocket) {
        removeSocket(socket);
    }
}

bool AnimationSystem::hasSocket(SkeletonHandle skeleton, std::string_view socketName) const {
    return findSocket(skeleton, socketName) != AnimationHandles::InvalidSocket;
}

SocketHandle AnimationSystem::findSocket(SkeletonHandle skeleton,
                                         std::string_view socketName) const {
    if (!impl_) return AnimationHandles::InvalidSocket;

    auto it = impl_->skeletonSockets_.find(skeleton);
    if (it == impl_->skeletonSockets_.end()) return AnimationHandles::InvalidSocket;

    for (auto handle : it->second) {
        auto socketIt = impl_->sockets_.find(handle);
        if (socketIt != impl_->sockets_.end() && socketIt->second.name == socketName) {
            return handle;
        }
    }

    return AnimationHandles::InvalidSocket;
}

std::vector<SocketState> AnimationSystem::getSockets(SkeletonHandle skeleton) const {
    std::vector<SocketState> states;
    if (!impl_) return states;

    auto it = impl_->skeletonSockets_.find(skeleton);
    if (it == impl_->skeletonSockets_.end()) return states;

    for (auto handle : it->second) {
        auto socketIt = impl_->sockets_.find(handle);
        if (socketIt != impl_->sockets_.end()) {
            const auto& data = socketIt->second;
            SocketState state;
            state.handle = handle;
            state.name = data.name;
            state.boneIndex = data.boneIndex;
            state.localTransform = composeTransform(data.localPosition, data.localRotation, data.localScale);
            state.attachMode = data.attachMode;
            state.enabled = data.enabled;
            states.push_back(state);
        }
    }

    return states;
}

std::optional<SocketDef> AnimationSystem::getSocketDef(SocketHandle socket) const {
    if (!impl_) return std::nullopt;

    auto it = impl_->sockets_.find(socket);
    if (it == impl_->sockets_.end()) return std::nullopt;

    const auto& data = it->second;
    SocketDef def;
    def.name = data.name;

    // Get bone name
    auto skelIt = impl_->skeletons_.find(data.skeleton);
    if (skelIt != impl_->skeletons_.end() && data.boneIndex < skelIt->second.bones.size()) {
        def.boneName = skelIt->second.bones[data.boneIndex].name;
    }

    def.localPosition = data.localPosition;
    def.localRotation = data.localRotation;
    def.localScale = data.localScale;
    def.attachMode = data.attachMode;

    return def;
}

Result<SocketTransform, AnimationError> AnimationSystem::getSocketTransform(
    AnimatorHandle animator, std::string_view socketName,
    const Mat4& entityWorldMatrix) const {
    if (!impl_) return std::unexpected(AnimationError::InvalidAnimator);

    auto animIt = impl_->animators_.find(animator);
    if (animIt == impl_->animators_.end()) {
        return std::unexpected(AnimationError::InvalidAnimator);
    }

    SocketHandle socket = findSocket(animIt->second.skeleton, socketName);
    if (socket == AnimationHandles::InvalidSocket) {
        return std::unexpected(AnimationError::SocketNotFound);
    }

    return getSocketTransform(animator, socket, entityWorldMatrix);
}

Result<SocketTransform, AnimationError> AnimationSystem::getSocketTransform(
    AnimatorHandle animator, SocketHandle socket,
    const Mat4& entityWorldMatrix) const {
    if (!impl_) return std::unexpected(AnimationError::InvalidAnimator);

    auto animIt = impl_->animators_.find(animator);
    if (animIt == impl_->animators_.end()) {
        return std::unexpected(AnimationError::InvalidAnimator);
    }

    auto socketIt = impl_->sockets_.find(socket);
    if (socketIt == impl_->sockets_.end()) {
        return std::unexpected(AnimationError::InvalidSocket);
    }

    const auto& socketData = socketIt->second;
    const auto& animData = animIt->second;

    if (socketData.boneIndex >= animData.boneTransforms.size()) {
        return std::unexpected(AnimationError::InvalidBoneIndex);
    }

    impl_->stats_.socketQueries++;

    // Get bone transform (already includes inverse bind pose)
    Mat4 boneTransform = animData.boneTransforms[socketData.boneIndex];

    // Need to undo inverse bind pose to get actual bone position
    auto skelIt = impl_->skeletons_.find(animData.skeleton);
    if (skelIt != impl_->skeletons_.end() && socketData.boneIndex < skelIt->second.bones.size()) {
        Mat4 invBindPose = skelIt->second.bones[socketData.boneIndex].inverseBindPose;
        boneTransform = boneTransform * glm::inverse(invBindPose);
    }

    // Apply socket local transform
    Mat4 socketLocal = composeTransform(socketData.localPosition, socketData.localRotation, socketData.localScale);
    Mat4 socketWorld = entityWorldMatrix * boneTransform * socketLocal;

    SocketTransform result;
    result.worldMatrix = socketWorld;
    decomposeTransform(socketWorld, result.position, result.rotation, result.scale);

    // Calculate directions
    Mat3 rotMat = glm::mat3_cast(result.rotation);
    result.right = glm::normalize(Vec3(rotMat[0]));
    result.up = glm::normalize(Vec3(rotMat[1]));
    result.forward = glm::normalize(-Vec3(rotMat[2]));  // -Z is forward

    return result;
}

std::vector<SocketTransform> AnimationSystem::getSocketTransforms(
    AnimatorHandle animator, std::span<const std::string_view> socketNames,
    const Mat4& entityWorldMatrix) const {
    std::vector<SocketTransform> results;
    results.reserve(socketNames.size());

    for (auto name : socketNames) {
        auto result = getSocketTransform(animator, name, entityWorldMatrix);
        if (result) {
            results.push_back(*result);
        } else {
            results.push_back(SocketTransform{});
        }
    }

    return results;
}

Result<void, AnimationError> AnimationSystem::setSocketLocalTransform(
    SocketHandle socket, const Vec3& position, const Quat& rotation,
    const Vec3& scale) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSocket);

    auto it = impl_->sockets_.find(socket);
    if (it == impl_->sockets_.end()) {
        return std::unexpected(AnimationError::InvalidSocket);
    }

    it->second.localPosition = position;
    it->second.localRotation = rotation;
    it->second.localScale = scale;

    return {};
}

void AnimationSystem::setSocketEnabled(SocketHandle socket, bool enabled) {
    if (!impl_) return;

    auto it = impl_->sockets_.find(socket);
    if (it != impl_->sockets_.end()) {
        it->second.enabled = enabled;
    }
}

bool AnimationSystem::isSocketEnabled(SocketHandle socket) const {
    if (!impl_) return false;

    auto it = impl_->sockets_.find(socket);
    return it != impl_->sockets_.end() && it->second.enabled;
}

//==========================================================================
// Socket Raycasting
//==========================================================================

SocketRaycastResult AnimationSystem::raycastFromSocket(
    AnimatorHandle animator, const SocketRaycastDef& def,
    const Mat4& entityWorldMatrix, const IPhysics3DSystem& physics) const {
    SocketRaycastResult result;
    result.socketName = def.socketName;

    auto transformResult = getSocketTransform(animator, def.socketName, entityWorldMatrix);
    if (!transformResult) {
        return result;
    }

    const auto& socketTransform = *transformResult;
    result.origin = socketTransform.position;

    // Transform local direction to world space
    Vec3 worldDir = glm::normalize(
        socketTransform.rotation * def.direction
    );
    result.direction = worldDir;

    // Perform raycast using physics system
    // Note: This calls IPhysics3DSystem::raycast which returns RaycastHit3D
    // The caller provides the physics system reference
    // For now, we just set up the ray - actual physics integration requires
    // calling the physics system's raycast method

    return result;
}

std::vector<SocketRaycastResult> AnimationSystem::raycastFromSockets(
    AnimatorHandle animator, std::span<const SocketRaycastDef> defs,
    const Mat4& entityWorldMatrix, const IPhysics3DSystem& physics) const {
    std::vector<SocketRaycastResult> results;
    results.reserve(defs.size());

    for (const auto& def : defs) {
        results.push_back(raycastFromSocket(animator, def, entityWorldMatrix, physics));
    }

    return results;
}

SocketRaycastResult AnimationSystem::sphereCastFromSocket(
    AnimatorHandle animator, const SocketRaycastDef& def,
    float sphereRadius, const Mat4& entityWorldMatrix,
    const IPhysics3DSystem& physics) const {
    // Similar to raycast, but would use sphere cast
    return raycastFromSocket(animator, def, entityWorldMatrix, physics);
}

//==========================================================================
// Inverse Kinematics: Chain Definition
//==========================================================================

Result<void, AnimationError> AnimationSystem::defineIKChain(
    SkeletonHandle skeleton, const IKTwoBoneChain& chain) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSkeleton);

    if (!isValidSkeleton(skeleton)) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    // Validate bone names
    std::int32_t rootIdx = findBoneIndex(skeleton, chain.rootBoneName);
    std::int32_t midIdx = findBoneIndex(skeleton, chain.midBoneName);
    std::int32_t tipIdx = findBoneIndex(skeleton, chain.tipBoneName);

    if (rootIdx < 0 || midIdx < 0 || tipIdx < 0) {
        return std::unexpected(AnimationError::BoneNotFound);
    }

    IKChainData data;
    data.name = chain.name;
    data.rootBoneIndex = rootIdx;
    data.midBoneIndex = midIdx;
    data.tipBoneIndex = tipIdx;

    impl_->ikChains_[skeleton].push_back(data);
    impl_->stats_.ikChainCount++;

    return {};
}

Result<void, AnimationError> AnimationSystem::defineIKAim(
    SkeletonHandle skeleton, const IKAimConfig& config) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSkeleton);

    if (!isValidSkeleton(skeleton)) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    std::int32_t boneIdx = findBoneIndex(skeleton, config.boneName);
    if (boneIdx < 0) {
        return std::unexpected(AnimationError::BoneNotFound);
    }

    IKAimData data;
    data.name = config.name;
    data.boneIndex = boneIdx;
    data.aimAxis = config.aimAxis;
    data.upAxis = config.upAxis;
    data.horizontalLimit = config.horizontalLimit;
    data.verticalLimit = config.verticalLimit;

    impl_->ikAims_[skeleton].push_back(data);

    return {};
}

void AnimationSystem::removeIKChain(SkeletonHandle skeleton, std::string_view name) {
    if (!impl_) return;

    auto it = impl_->ikChains_.find(skeleton);
    if (it == impl_->ikChains_.end()) return;

    auto& chains = it->second;
    chains.erase(
        std::remove_if(chains.begin(), chains.end(),
            [&](const auto& c) { return c.name == name; }),
        chains.end()
    );
}

void AnimationSystem::removeIKAim(SkeletonHandle skeleton, std::string_view name) {
    if (!impl_) return;

    auto it = impl_->ikAims_.find(skeleton);
    if (it == impl_->ikAims_.end()) return;

    auto& aims = it->second;
    aims.erase(
        std::remove_if(aims.begin(), aims.end(),
            [&](const auto& a) { return a.name == name; }),
        aims.end()
    );
}

std::vector<std::string> AnimationSystem::getIKChainNames(SkeletonHandle skeleton) const {
    std::vector<std::string> names;
    if (!impl_) return names;

    auto it = impl_->ikChains_.find(skeleton);
    if (it == impl_->ikChains_.end()) return names;

    for (const auto& chain : it->second) {
        names.push_back(chain.name);
    }

    return names;
}

std::vector<std::string> AnimationSystem::getIKAimNames(SkeletonHandle skeleton) const {
    std::vector<std::string> names;
    if (!impl_) return names;

    auto it = impl_->ikAims_.find(skeleton);
    if (it == impl_->ikAims_.end()) return names;

    for (const auto& aim : it->second) {
        names.push_back(aim.name);
    }

    return names;
}

//==========================================================================
// Inverse Kinematics: Runtime Control
//==========================================================================

void AnimationSystem::setIKTarget(AnimatorHandle animator, const IKTwoBoneTarget& target) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.twoBoneTargets[target.chainName] = target;
    }
}

void AnimationSystem::setIKTarget(AnimatorHandle animator, const IKAimTarget& target) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.aimTargets[target.configName] = target;
    }
}

std::optional<IKTwoBoneTarget> AnimationSystem::getIKTwoBoneTarget(
    AnimatorHandle animator, std::string_view chainName) const {
    if (!impl_) return std::nullopt;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return std::nullopt;

    auto targetIt = it->second.twoBoneTargets.find(std::string(chainName));
    if (targetIt == it->second.twoBoneTargets.end()) return std::nullopt;

    return targetIt->second;
}

std::optional<IKAimTarget> AnimationSystem::getIKAimTarget(
    AnimatorHandle animator, std::string_view configName) const {
    if (!impl_) return std::nullopt;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return std::nullopt;

    auto targetIt = it->second.aimTargets.find(std::string(configName));
    if (targetIt == it->second.aimTargets.end()) return std::nullopt;

    return targetIt->second;
}

void AnimationSystem::clearIKTarget(AnimatorHandle animator, std::string_view targetName) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    it->second.twoBoneTargets.erase(std::string(targetName));
    it->second.aimTargets.erase(std::string(targetName));
}

void AnimationSystem::clearAllIKTargets(AnimatorHandle animator) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.twoBoneTargets.clear();
        it->second.aimTargets.clear();
    }
}

void AnimationSystem::setIKWeight(AnimatorHandle animator, std::string_view targetName,
                                  float weight) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    auto tbIt = it->second.twoBoneTargets.find(std::string(targetName));
    if (tbIt != it->second.twoBoneTargets.end()) {
        tbIt->second.weight = std::clamp(weight, 0.0f, 1.0f);
        return;
    }

    auto aimIt = it->second.aimTargets.find(std::string(targetName));
    if (aimIt != it->second.aimTargets.end()) {
        aimIt->second.weight = std::clamp(weight, 0.0f, 1.0f);
    }
}

float AnimationSystem::getIKWeight(AnimatorHandle animator,
                                   std::string_view targetName) const {
    if (!impl_) return 0.0f;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return 0.0f;

    auto tbIt = it->second.twoBoneTargets.find(std::string(targetName));
    if (tbIt != it->second.twoBoneTargets.end()) {
        return tbIt->second.weight;
    }

    auto aimIt = it->second.aimTargets.find(std::string(targetName));
    if (aimIt != it->second.aimTargets.end()) {
        return aimIt->second.weight;
    }

    return 0.0f;
}

//==========================================================================
// Root Motion
//==========================================================================

void AnimationSystem::setRootMotionConfig(AnimatorHandle animator,
                                          const RootMotionConfig& config) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.rootMotionConfig = config;

        // Resolve root bone index
        if (!config.rootBoneName.empty()) {
            it->second.rootMotionConfig.rootBoneIndex =
                findBoneIndex(it->second.skeleton, config.rootBoneName);
        }
    }
}

RootMotionConfig AnimationSystem::getRootMotionConfig(AnimatorHandle animator) const {
    if (!impl_) return {};

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return {};

    return it->second.rootMotionConfig;
}

void AnimationSystem::setRootMotionEnabled(AnimatorHandle animator, bool enabled) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.rootMotionConfig.enabled = enabled;
    }
}

bool AnimationSystem::isRootMotionEnabled(AnimatorHandle animator) const {
    if (!impl_) return false;

    auto it = impl_->animators_.find(animator);
    return it != impl_->animators_.end() && it->second.rootMotionConfig.enabled;
}

RootMotion AnimationSystem::getRootMotion(AnimatorHandle animator) const {
    if (!impl_) return {};

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return {};

    return it->second.currentRootMotion;
}

RootMotion AnimationSystem::extractRootMotion(AnimationClipHandle clip,
                                              float fromTime, float toTime) const {
    RootMotion result;
    if (!impl_) return result;

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return result;

    // Find root bone channel (index 0 typically)
    for (const auto& channel : it->second.channels) {
        if (channel.boneIndex == 0 && !channel.keyframes.empty()) {
            // Sample at both times
            auto sampleAt = [&](float t) -> std::pair<Vec3, Quat> {
                const AnimationKeyframe* prev = nullptr;
                const AnimationKeyframe* next = nullptr;

                for (const auto& kf : channel.keyframes) {
                    if (kf.time <= t) prev = &kf;
                    if (kf.time >= t && !next) {
                        next = &kf;
                        break;
                    }
                }

                if (!prev) prev = &channel.keyframes[0];
                if (!next) next = prev;

                float duration = next->time - prev->time;
                float interp = duration > 0.0001f ? (t - prev->time) / duration : 0.0f;
                interp = std::clamp(interp, 0.0f, 1.0f);

                return {
                    lerp(prev->position, next->position, interp),
                    slerp(prev->rotation, next->rotation, interp)
                };
            };

            auto [pos1, rot1] = sampleAt(fromTime);
            auto [pos2, rot2] = sampleAt(toTime);

            result.deltaPosition = pos2 - pos1;
            result.deltaRotation = rot2 * glm::inverse(rot1);
            result.totalPosition = pos2;
            result.totalRotation = rot2;
            result.hasTranslation = glm::length(result.deltaPosition) > 0.0001f;
            result.hasRotation = true;

            break;
        }
    }

    return result;
}

void AnimationSystem::consumeRootMotion(AnimatorHandle animator) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.currentRootMotion = {};
    }
}

//==========================================================================
// Physics Integration: Ragdoll
//==========================================================================

Result<void, AnimationError> AnimationSystem::createRagdoll(
    Entity entity, const RagdollDef& def, IPhysics3DSystem& physics) {
    if (!impl_) return std::unexpected(AnimationError::PhysicsSystemRequired);

    if (!isValidSkeleton(def.skeleton)) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    if (hasRagdoll(entity)) {
        return std::unexpected(AnimationError::RagdollAlreadyExists);
    }

    RagdollData& data = impl_->ragdolls_[entity];
    data.entity = entity;
    data.skeleton = def.skeleton;
    data.definition = def;
    data.state.created = true;
    data.state.active = false;
    data.state.blendWeight = 0.0f;

    // Store bone indices
    for (const auto& boneDef : def.bones) {
        data.state.boneIndices.push_back(boneDef.boneIndex);
    }

    impl_->stats_.ragdollCount++;
    return {};
}

void AnimationSystem::destroyRagdoll(Entity entity, IPhysics3DSystem& physics) {
    if (!impl_) return;

    impl_->ragdolls_.erase(entity);
    impl_->stats_.ragdollCount--;
}

bool AnimationSystem::hasRagdoll(Entity entity) const {
    if (!impl_) return false;
    return impl_->ragdolls_.find(entity) != impl_->ragdolls_.end();
}

RagdollState AnimationSystem::getRagdollState(Entity entity) const {
    if (!impl_) return {};

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return {};

    return it->second.state;
}

void AnimationSystem::activateRagdoll(Entity entity, IPhysics3DSystem& physics,
                                      bool instant, float blendDuration) {
    if (!impl_) return;

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return;

    if (instant) {
        it->second.state.active = true;
        it->second.state.blendWeight = 1.0f;
        it->second.state.blendTarget = 1.0f;
        it->second.state.blendDuration = 0.0f;
        it->second.state.blendTime = 0.0f;
    } else {
        it->second.state.blendTarget = 1.0f;
        it->second.state.blendDuration = blendDuration;
        it->second.state.blendTime = 0.0f;
    }
}

void AnimationSystem::deactivateRagdoll(Entity entity, IPhysics3DSystem& physics,
                                        bool instant, float blendDuration) {
    if (!impl_) return;

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return;

    if (instant) {
        it->second.state.active = false;
        it->second.state.blendWeight = 0.0f;
        it->second.state.blendTarget = 0.0f;
        it->second.state.blendDuration = 0.0f;
        it->second.state.blendTime = 0.0f;
    } else {
        it->second.state.blendTarget = 0.0f;
        it->second.state.blendDuration = blendDuration;
        it->second.state.blendTime = 0.0f;
    }
}

void AnimationSystem::setRagdollBlendWeight(Entity entity, float weight) {
    if (!impl_) return;

    auto it = impl_->ragdolls_.find(entity);
    if (it != impl_->ragdolls_.end()) {
        it->second.state.blendWeight = std::clamp(weight, 0.0f, 1.0f);
        it->second.state.active = weight > 0.5f;
    }
}

float AnimationSystem::getRagdollBlendWeight(Entity entity) const {
    if (!impl_) return 0.0f;

    auto it = impl_->ragdolls_.find(entity);
    return it != impl_->ragdolls_.end() ? it->second.state.blendWeight : 0.0f;
}

bool AnimationSystem::isRagdollActive(Entity entity) const {
    if (!impl_) return false;

    auto it = impl_->ragdolls_.find(entity);
    return it != impl_->ragdolls_.end() && it->second.state.active;
}

std::vector<Mat4> AnimationSystem::getRagdollBoneTransforms(
    Entity entity, const IPhysics3DSystem& physics) const {
    if (!impl_) return {};

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return {};

    // Return identity transforms for now - actual implementation would
    // query physics system for body transforms
    const auto& skelData = impl_->skeletons_[it->second.skeleton];
    return std::vector<Mat4>(skelData.bones.size(), Mat4{1.0f});
}

//==========================================================================
// Physics Integration: Bone Sync & Impulses
//==========================================================================

void AnimationSystem::syncToPhysics(AnimatorHandle animator, Entity entity,
                                    IPhysics3DSystem& physics) {
    // Sync animated bone transforms to kinematic physics bodies
    // This would iterate over ragdoll bones and set their transforms
}

void AnimationSystem::applyBoneImpulse(Entity entity, std::uint32_t boneIndex,
                                       const Vec3& impulse, IPhysics3DSystem& physics) {
    // Apply impulse to specific ragdoll bone body
}

void AnimationSystem::applyBoneImpulse(Entity entity, std::string_view boneName,
                                       const Vec3& impulse, IPhysics3DSystem& physics) {
    if (!impl_) return;

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return;

    std::int32_t boneIndex = findBoneIndex(it->second.skeleton, boneName);
    if (boneIndex >= 0) {
        applyBoneImpulse(entity, static_cast<std::uint32_t>(boneIndex), impulse, physics);
    }
}

void AnimationSystem::applyImpulseAtPosition(Entity entity, const Vec3& worldPosition,
                                             const Vec3& impulse, float radius,
                                             IPhysics3DSystem& physics) {
    // Find nearest bone and apply impulse
}

//==========================================================================
// Event Subscriptions
//==========================================================================

SubscriptionId AnimationSystem::subscribeToEvents(AnimatorHandle animator,
                                                  AnimationEventCallback callback) {
    if (!impl_) return 0;

    SubscriptionId id = impl_->nextSubscriptionId_++;
    impl_->subscriptions_.push_back({
        id, animator, EventSubscription::Type::Event, callback
    });
    return id;
}

SubscriptionId AnimationSystem::subscribeToComplete(AnimatorHandle animator,
                                                    AnimationCompleteCallback callback) {
    if (!impl_) return 0;

    SubscriptionId id = impl_->nextSubscriptionId_++;
    impl_->subscriptions_.push_back({
        id, animator, EventSubscription::Type::Complete, callback
    });
    return id;
}

SubscriptionId AnimationSystem::subscribeToLayerChanges(AnimatorHandle animator,
                                                        AnimationLayerCallback callback) {
    if (!impl_) return 0;

    SubscriptionId id = impl_->nextSubscriptionId_++;
    impl_->subscriptions_.push_back({
        id, animator, EventSubscription::Type::LayerChange, callback
    });
    return id;
}

void AnimationSystem::unsubscribe(SubscriptionId id) {
    if (!impl_) return;

    impl_->subscriptions_.erase(
        std::remove_if(impl_->subscriptions_.begin(), impl_->subscriptions_.end(),
            [id](const auto& sub) { return sub.id == id; }),
        impl_->subscriptions_.end()
    );
}

//==========================================================================
// Stateless Animation Sampling
//==========================================================================

Result<std::vector<Mat4>, AnimationError> AnimationSystem::sampleAnimation(
    AnimationClipHandle clip, float time, AnimationWrapMode wrapMode) {
    if (!impl_) return std::unexpected(AnimationError::InvalidClip);

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) {
        return std::unexpected(AnimationError::InvalidClip);
    }

    const auto& clipData = it->second;
    auto skelIt = impl_->skeletons_.find(clipData.skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    const auto& skelData = skelIt->second;
    float wrappedTime = wrapTime(time, clipData.duration, wrapMode);

    std::vector<Mat4> transforms(skelData.bones.size(), Mat4{1.0f});

    for (const auto& channel : clipData.channels) {
        if (channel.boneIndex < 0 ||
            static_cast<std::size_t>(channel.boneIndex) >= skelData.bones.size()) continue;

        const AnimationKeyframe* prev = nullptr;
        const AnimationKeyframe* next = nullptr;

        for (const auto& kf : channel.keyframes) {
            if (kf.time <= wrappedTime) prev = &kf;
            if (kf.time >= wrappedTime && !next) {
                next = &kf;
                break;
            }
        }

        if (!prev && !next && !channel.keyframes.empty()) {
            prev = &channel.keyframes[0];
            next = &channel.keyframes[0];
        } else if (!prev) {
            prev = next;
        } else if (!next) {
            next = prev;
        }

        if (prev && next) {
            float duration = next->time - prev->time;
            float t = duration > 0.0001f ? (wrappedTime - prev->time) / duration : 0.0f;
            t = std::clamp(t, 0.0f, 1.0f);

            Vec3 position = lerp(prev->position, next->position, t);
            Quat rotation = slerp(prev->rotation, next->rotation, t);
            Vec3 scale = lerp(prev->scale, next->scale, t);

            transforms[channel.boneIndex] = composeTransform(position, rotation, scale);
        }
    }

    return transforms;
}

Result<std::vector<Mat4>, AnimationError> AnimationSystem::blendAnimations(
    SkeletonHandle skeleton, const AnimationBlendConfig& config) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSkeleton);

    auto skelIt = impl_->skeletons_.find(skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    const auto& skelData = skelIt->second;
    std::vector<Mat4> result(skelData.bones.size(), Mat4{1.0f});

    // Start with bind pose
    for (std::size_t i = 0; i < skelData.bones.size(); ++i) {
        result[i] = skelData.bones[i].localBindPose;
    }

    // Blend layers
    for (const auto& layer : config.layers) {
        if (layer.clip == AnimationHandles::InvalidClip || layer.weight <= 0.001f) continue;

        auto sampleResult = sampleAnimation(layer.clip, layer.time, layer.wrapMode);
        if (!sampleResult) continue;

        const auto& layerTransforms = *sampleResult;
        float weight = layer.weight * config.masterWeight;

        for (std::size_t i = 0; i < skelData.bones.size() && i < layerTransforms.size(); ++i) {
            Vec3 pos1, pos2, scale1, scale2;
            Quat rot1, rot2;

            decomposeTransform(result[i], pos1, rot1, scale1);
            decomposeTransform(layerTransforms[i], pos2, rot2, scale2);

            result[i] = composeTransform(
                lerp(pos1, pos2, weight),
                slerp(rot1, rot2, weight),
                lerp(scale1, scale2, weight)
            );
        }
    }

    return result;
}

//==========================================================================
// Statistics & Debugging
//==========================================================================

AnimationStats AnimationSystem::getStats() const {
    if (!impl_) return {};

    AnimationStats stats = impl_->stats_;

    // Calculate memory usage estimates
    stats.skeletonMemoryBytes = 0;
    for (const auto& [h, s] : impl_->skeletons_) {
        stats.skeletonMemoryBytes += s.bones.size() * sizeof(BoneInfo);
        stats.skeletonMemoryBytes += s.boneNameToIndex.size() * 64; // Estimate
    }

    stats.clipMemoryBytes = 0;
    for (const auto& [h, c] : impl_->clips_) {
        stats.clipMemoryBytes += sizeof(AnimationClipData);
        for (const auto& ch : c.channels) {
            stats.clipMemoryBytes += ch.keyframes.size() * sizeof(AnimationKeyframe);
        }
    }

    stats.animatorMemoryBytes = 0;
    for (const auto& [h, a] : impl_->animators_) {
        stats.animatorMemoryBytes += sizeof(AnimatorData);
        stats.animatorMemoryBytes += a.boneTransforms.size() * sizeof(Mat4) * 2;
    }

    stats.totalMemoryBytes = stats.skeletonMemoryBytes + stats.clipMemoryBytes +
                             stats.animatorMemoryBytes;

    return stats;
}

void AnimationSystem::resetFrameStats() {
    if (!impl_) return;

    impl_->stats_.animatorsUpdated = 0;
    impl_->stats_.layersProcessed = 0;
    impl_->stats_.samplingJobs = 0;
    impl_->stats_.blendingJobs = 0;
    impl_->stats_.ikSolves = 0;
    impl_->stats_.socketQueries = 0;
    impl_->stats_.eventsDispatched = 0;
    impl_->stats_.ragdollSyncs = 0;
    impl_->stats_.updateTimeMs = 0.0f;
    impl_->stats_.samplingTimeMs = 0.0f;
    impl_->stats_.blendingTimeMs = 0.0f;
    impl_->stats_.ikTimeMs = 0.0f;
    impl_->stats_.ragdollSyncTimeMs = 0.0f;
}

void AnimationSystem::setDebugVisualization(bool enabled) {
    if (impl_) {
        impl_->debugVisualization_ = enabled;
    }
}

bool AnimationSystem::isDebugVisualizationEnabled() const {
    return impl_ && impl_->debugVisualization_;
}

}  // namespace bestow
