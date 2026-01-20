// bestow-animation/src/AnimationSystem.cpp
// Animation System Implementation using ozz-animation backend
//
// This implementation wraps ozz-animation to provide a clean, high-level API
// while leveraging ozz's battle-tested, SIMD-optimized runtime.

// Global module fragment - third-party includes go here
module;

#include <spdlog/spdlog.h>

// GLM math library - MUST be before ozz to ensure operator* resolution
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ozz-animation headers
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/ik_two_bone_job.h>
#include <ozz/animation/runtime/ik_aim_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/simd_quaternion.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/containers/vector.h>

module bestow.animation.impl;

import bestow.services;

import std;

namespace bestow {

namespace {

//==========================================================================
// Math Conversion Helpers (glm <-> ozz)
//==========================================================================

ozz::math::Float3 toOzz(const Vec3& v) {
    return ozz::math::Float3(v.x, v.y, v.z);
}

ozz::math::Quaternion toOzz(const Quat& q) {
    return ozz::math::Quaternion(q.x, q.y, q.z, q.w);
}

Vec3 fromOzz(const ozz::math::Float3& v) {
    return Vec3(v.x, v.y, v.z);
}

Quat fromOzz(const ozz::math::Quaternion& q) {
    return Quat(q.w, q.x, q.y, q.z);
}

// Convert ozz Float4x4 to glm Mat4
Mat4 fromOzz(const ozz::math::Float4x4& m) {
    Mat4 result;
    // ozz uses column-major like glm
    for (int col = 0; col < 4; ++col) {
        ozz::math::SimdFloat4 column = m.cols[col];
        float values[4];
        ozz::math::StorePtrU(column, values);
        result[col] = Vec4(values[0], values[1], values[2], values[3]);
    }
    return result;
}

// Convert glm Mat4 to ozz Float4x4
ozz::math::Float4x4 toOzz(const Mat4& m) {
    ozz::math::Float4x4 result;
    for (int col = 0; col < 4; ++col) {
        result.cols[col] = ozz::math::simd_float4::Load(m[col].x, m[col].y, m[col].z, m[col].w);
    }
    return result;
}

//==========================================================================
// Math Utility Helpers
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

    if (dot < 0.0f) {
        result = Quat(-b.w, -b.x, -b.y, -b.z);
        dot = -dot;
    }

    if (dot > 0.9995f) {
        return glm::normalize(Quat(
            lerp(a.w, result.w, t),
            lerp(a.x, result.x, t),
            lerp(a.y, result.y, t),
            lerp(a.z, result.z, t)
        ));
    }

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
    result[0][0] = scale.x;
    result[1][1] = scale.y;
    result[2][2] = scale.z;
    Mat4 rotMat = glm::mat4_cast(rotation);
    result = rotMat * result;
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

// Internal data structures are defined in bestow.animation.impl.cppm

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

    static int debugFrame = 0;
    bool shouldLog = (++debugFrame % 60 == 0);

    if (shouldLog) {
        spdlog::info("[AnimUpdate] dt={:.4f}, {} animators", dt, impl_->animators_.size());
    }

    for (auto& [handle, animator] : impl_->animators_) {
        if (animator.paused) {
            if (shouldLog) spdlog::info("[AnimUpdate] animator {} paused, skipping", handle);
            continue;
        }

        auto skelIt = impl_->skeletons_.find(animator.skeleton);
        if (skelIt == impl_->skeletons_.end()) {
            if (shouldLog) spdlog::info("[AnimUpdate] animator {} has no skeleton", handle);
            continue;
        }

        auto& skeleton = skelIt->second;
        impl_->stats_.animatorsUpdated++;

        if (shouldLog) {
            spdlog::info("[AnimUpdate] animator {} has {} layers", handle, animator.layers.size());
        }

        const std::size_t boneCount = skeleton.bones.size();

        // Initialize transform buffers if needed
        if (animator.boneTransforms.size() != boneCount) {
            animator.boneTransforms.resize(boneCount, Mat4{1.0f});
            animator.localTransforms.resize(boneCount, Mat4{1.0f});

            for (std::size_t i = 0; i < boneCount; ++i) {
                animator.localTransforms[i] = skeleton.bones[i].localBindPose;
            }

            const int numSoaJoints = skeleton.ozzSkeleton.num_soa_joints();
            animator.blendedLocals.resize(numSoaJoints);
            animator.modelMatrices.resize(skeleton.ozzSkeleton.num_joints());
        }

        // Prepare blending layers
        std::vector<ozz::animation::BlendingJob::Layer> blendLayers;

        for (auto& layer : animator.layers) {
            impl_->stats_.layersProcessed++;

            if (shouldLog) {
                spdlog::info("[AnimUpdate] layer: playing={}, clip={}, paused={}",
                    layer.playing, layer.clip, layer.paused);
            }

            if (!layer.playing || layer.clip == AnimationHandles::InvalidClip) continue;

            // Update fade
            if (layer.fadeSpeed != 0.0f) {
                layer.fadeWeight += layer.fadeSpeed * dt;
                if (layer.fadeWeight <= 0.0f) {
                    layer.fadeWeight = 0.0f;
                    layer.playing = false;
                    layer.fadeSpeed = 0.0f;
                    continue;
                } else if (layer.fadeWeight >= 1.0f) {
                    layer.fadeWeight = 1.0f;
                    layer.fadeSpeed = 0.0f;
                    // Crossfade complete - clear outgoing clip and root offset
                    if (layer.outgoingClip != AnimationHandles::InvalidClip) {
                        spdlog::info("[Crossfade] Complete - clearing outgoing clip {}", layer.outgoingClip);
                        layer.outgoingClip = AnimationHandles::InvalidClip;
                        layer.outgoingTime = 0.0f;
                        layer.outgoingSamplingContext.reset();
                        layer.outgoingLocalTransforms.clear();
                        layer.crossfadeRootOffset = Vec3{0.0f};
                        layer.hasCrossfadeRootOffset = false;
                    }
                }
            }

            if (layer.paused) continue;

            auto clipIt = impl_->clips_.find(layer.clip);
            if (clipIt == impl_->clips_.end()) continue;
            auto& clip = clipIt->second;

            float prevTime = layer.time;
            layer.time += dt * layer.speed * animator.globalSpeed;

            // Check for events
            for (const auto& event : clip.events) {
                bool crossed = (prevTime <= event.time && layer.time > event.time) ||
                               (layer.speed < 0 && prevTime >= event.time && layer.time < event.time);
                if (crossed) {
                    AnimationEvent evt;
                    evt.animator = handle;
                    evt.clip = layer.clip;
                    evt.layer = static_cast<std::uint32_t>(&layer - animator.layers.data());
                    evt.name = event.name;
                    evt.clipTime = event.time;
                    evt.normalizedTime = clip.duration > 0 ? event.time / clip.duration : 0;
                    evt.stringParam = event.stringParam;
                    evt.floatParam = event.floatParam;
                    evt.intParam = event.intParam;
                    impl_->pendingEvents_.push_back(evt);
                }
            }

            // Handle wrap mode
            float wrappedTime = wrapTime(layer.time, clip.duration, layer.wrapMode);

            // Check for completion
            if (layer.wrapMode == AnimationWrapMode::Once && layer.time >= clip.duration) {
                layer.playing = false;
                layer.time = clip.duration;

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

            const int numSoaJoints = skeleton.ozzSkeleton.num_soa_joints();

            //==================================================================
            // Sample OUTGOING clip (if crossfading)
            //==================================================================
            Vec3 outgoingRootPos{0.0f};
            bool hasOutgoingRoot = false;

            if (layer.outgoingClip != AnimationHandles::InvalidClip) {
                auto outClipIt = impl_->clips_.find(layer.outgoingClip);
                if (outClipIt != impl_->clips_.end()) {
                    auto& outClip = outClipIt->second;

                    // Update outgoing time
                    layer.outgoingTime += dt * layer.outgoingSpeed * animator.globalSpeed;
                    float outWrappedTime = wrapTime(layer.outgoingTime, outClip.duration, layer.outgoingWrapMode);
                    layer.outgoingTime = outWrappedTime;

                    // Initialize outgoing sampling context if needed (check for size mismatch too)
                    if (!layer.outgoingSamplingContext ||
                        layer.outgoingSamplingContext->max_tracks() < outClip.ozzAnimation.num_tracks()) {
                        layer.outgoingSamplingContext = std::make_unique<ozz::animation::SamplingJob::Context>();
                        layer.outgoingSamplingContext->Resize(outClip.ozzAnimation.num_tracks());
                    }

                    // Resize outgoing transforms buffer
                    if (layer.outgoingLocalTransforms.size() != static_cast<size_t>(numSoaJoints)) {
                        layer.outgoingLocalTransforms.resize(numSoaJoints);
                    }

                    // Sample outgoing animation
                    impl_->stats_.samplingJobs++;
                    ozz::animation::SamplingJob outSamplingJob;
                    outSamplingJob.animation = &outClip.ozzAnimation;
                    outSamplingJob.context = layer.outgoingSamplingContext.get();
                    float outRatio = outClip.duration > 0 ? layer.outgoingTime / outClip.duration : 0.0f;
                    outSamplingJob.ratio = outRatio;
                    outSamplingJob.output = ozz::make_span(layer.outgoingLocalTransforms);

                    if (outSamplingJob.Run()) {
                        // Extract root position from outgoing animation (joint 0)
                        if (!layer.outgoingLocalTransforms.empty()) {
                            const auto& rootSoa = layer.outgoingLocalTransforms[0];
                            outgoingRootPos.x = ozz::math::GetX(rootSoa.translation.x);
                            outgoingRootPos.y = ozz::math::GetX(rootSoa.translation.y);
                            outgoingRootPos.z = ozz::math::GetX(rootSoa.translation.z);
                            hasOutgoingRoot = true;
                        }

                        // Apply smoothstep to fadeWeight for smoother blend curve
                        // smoothstep(t) = t * t * (3 - 2 * t)
                        float t = layer.fadeWeight;
                        float smoothT = t * t * (3.0f - 2.0f * t);

                        // Outgoing weight decreases as fadeWeight increases (1 - smoothT)
                        float outgoingWeight = layer.weight * (1.0f - smoothT);
                        if (shouldLog) {
                            spdlog::info("[Crossfade] Outgoing clip {} weight={:.3f} (fadeWeight={:.3f}, smoothT={:.3f})",
                                layer.outgoingClip, outgoingWeight, layer.fadeWeight, smoothT);
                        }
                        // Always add outgoing layer during crossfade (let ozz handle tiny weights)
                        // Using a minimum weight floor to avoid numerical issues
                        ozz::animation::BlendingJob::Layer blendLayer;
                        blendLayer.transform = ozz::make_span(layer.outgoingLocalTransforms);
                        blendLayer.weight = std::max(outgoingWeight, 0.0001f);
                        blendLayers.push_back(blendLayer);
                    }
                }
            }

            //==================================================================
            // Sample INCOMING clip (current)
            //==================================================================

            // Initialize sampling context if needed (check for size mismatch too)
            if (!layer.samplingContext ||
                layer.samplingContext->max_tracks() < clip.ozzAnimation.num_tracks()) {
                layer.samplingContext = std::make_unique<ozz::animation::SamplingJob::Context>();
                layer.samplingContext->Resize(clip.ozzAnimation.num_tracks());
            }

            // Resize local transforms buffer
            if (layer.localTransforms.size() != static_cast<size_t>(numSoaJoints)) {
                layer.localTransforms.resize(numSoaJoints);
            }

            // Sample animation using ozz
            impl_->stats_.samplingJobs++;
            ozz::animation::SamplingJob samplingJob;
            samplingJob.animation = &clip.ozzAnimation;
            samplingJob.context = layer.samplingContext.get();
            float ratio = clip.duration > 0 ? layer.time / clip.duration : 0.0f;
            samplingJob.ratio = ratio;
            samplingJob.output = ozz::make_span(layer.localTransforms);

            if (shouldLog) {
                spdlog::info("[AnimUpdate] Sampling: time={:.3f}, duration={:.3f}, ratio={:.3f}, tracks={}, soa_joints={}",
                    layer.time, clip.duration, ratio,
                    clip.ozzAnimation.num_tracks(),
                    layer.localTransforms.size());
            }

            if (samplingJob.Run()) {
                // Apply smoothstep to fadeWeight for smoother blend curve
                float t = layer.fadeWeight;
                float smoothT = t * t * (3.0f - 2.0f * t);

                float effectiveWeight = layer.weight * smoothT;
                if (shouldLog) {
                    spdlog::info("[AnimUpdate] Incoming clip {} weight={:.3f} (weight={:.3f}, fadeWeight={:.3f})",
                        layer.clip, effectiveWeight, layer.weight, layer.fadeWeight);
                }
                // Always add incoming layer (let ozz handle tiny weights via threshold)
                // Using a minimum weight floor to avoid numerical issues
                ozz::animation::BlendingJob::Layer blendLayer;
                blendLayer.transform = ozz::make_span(layer.localTransforms);
                blendLayer.weight = std::max(effectiveWeight, 0.0001f);
                blendLayers.push_back(blendLayer);
            } else {
                if (shouldLog) {
                    spdlog::warn("[AnimUpdate] Sampling FAILED!");
                }
            }
        }

        // Blend all layers
        if (!blendLayers.empty()) {
            impl_->stats_.blendingJobs++;

            if (shouldLog) {
                spdlog::info("[AnimUpdate] Blending {} layers", blendLayers.size());
            }

            ozz::animation::BlendingJob blendJob;
            blendJob.threshold = 0.001f;  // Very low - only use rest pose when weights are nearly zero
            blendJob.layers = ozz::make_span(blendLayers);
            blendJob.rest_pose = skeleton.ozzSkeleton.joint_rest_poses();
            blendJob.output = ozz::make_span(animator.blendedLocals);

            if (blendJob.Run()) {
                // Convert to model space
                ozz::animation::LocalToModelJob ltmJob;
                ltmJob.skeleton = &skeleton.ozzSkeleton;
                ltmJob.input = ozz::make_span(animator.blendedLocals);
                ltmJob.output = ozz::make_span(animator.modelMatrices);

                if (ltmJob.Run()) {
                    // Resize model space poses if needed
                    if (animator.modelSpacePoses.size() != boneCount) {
                        animator.modelSpacePoses.resize(boneCount, Mat4{1.0f});
                    }

                    // Compute final skinning matrices: ModelSpace * InverseBindPose
                    // - InverseBindPose: transforms vertex from bind pose to bone-local space
                    // - ModelSpace: transforms from bone-local space to animated world space
                    // The order matters! ModelSpace * InverseBindPose (column-major convention)
                    //
                    // IMPORTANT: ozz joint indices != our bone indices!
                    // ozz reorders joints during skeleton building (depth-first traversal)
                    // We must use the mapping to get the correct bone index.
                    for (std::size_t ozzIdx = 0; ozzIdx < boneCount; ++ozzIdx) {
                        std::int32_t boneIdx = skeleton.ozzToBoneIndex[ozzIdx];
                        Mat4 modelPose = fromOzz(animator.modelMatrices[ozzIdx]);
                        animator.modelSpacePoses[boneIdx] = modelPose;  // Store for visualization
                        animator.boneTransforms[boneIdx] = modelPose * skeleton.bones[boneIdx].inverseBindPose;
                    }

                    // Root offset system DISABLED - ozz BlendingJob already handles smooth
                    // interpolation between animation poses. The manual root offset was causing
                    // artifacts with large offsets (60+ cm) during rapid state transitions.
                    // The ozz blending naturally interpolates root positions correctly.

                    if (shouldLog && boneCount > 0) {
                        // Log first bone transform position for debugging
                        const auto& m = animator.boneTransforms[0];
                        spdlog::info("[AnimUpdate] Bone 0 skinning matrix pos: {}, {}, {}",
                            m[3][0], m[3][1], m[3][2]);
                        // Also log the model pose position (use proper ozz conversion)
                        Mat4 modelPose0 = fromOzz(animator.modelMatrices[0]);
                        spdlog::info("[AnimUpdate] Bone 0 model pose pos: {}, {}, {}",
                            modelPose0[3][0], modelPose0[3][1], modelPose0[3][2]);
                        // Also check a mid-body bone like bone 5 (should be spine/chest)
                        if (boneCount > 5) {
                            Mat4 modelPose5 = fromOzz(animator.modelMatrices[5]);
                            spdlog::info("[AnimUpdate] Bone 5 model pose pos: {}, {}, {}",
                                modelPose5[3][0], modelPose5[3][1], modelPose5[3][2]);
                        }
                    }
                }
            }
        } else {
            // No blend layers - use rest pose as fallback to avoid stale data
            if (shouldLog) {
                spdlog::info("[AnimUpdate] No blend layers - using rest pose");
            }

            // Copy rest pose to blendedLocals
            auto restPose = skeleton.ozzSkeleton.joint_rest_poses();
            if (animator.blendedLocals.size() == restPose.size()) {
                std::copy(restPose.begin(), restPose.end(), animator.blendedLocals.begin());

                // Convert to model space
                ozz::animation::LocalToModelJob ltmJob;
                ltmJob.skeleton = &skeleton.ozzSkeleton;
                ltmJob.input = ozz::make_span(animator.blendedLocals);
                ltmJob.output = ozz::make_span(animator.modelMatrices);

                if (ltmJob.Run()) {
                    for (std::size_t ozzIdx = 0; ozzIdx < boneCount; ++ozzIdx) {
                        std::int32_t boneIdx = skeleton.ozzToBoneIndex[ozzIdx];
                        Mat4 modelPose = fromOzz(animator.modelMatrices[ozzIdx]);
                        animator.modelSpacePoses[boneIdx] = modelPose;
                        animator.boneTransforms[boneIdx] = modelPose * skeleton.bones[boneIdx].inverseBindPose;
                    }
                }
            }
        }

        // Apply IK
        applyIK(animator, skeleton);

        // Extract root motion if enabled
        if (animator.rootMotionConfig.enabled) {
            extractRootMotion(animator, skeleton);
        }
    }

    // Dispatch pending events
    for (const auto& event : impl_->pendingEvents_) {
        for (auto& sub : impl_->subscriptions_) {
            if (sub.animator == event.animator && sub.type == EventSubscription::Type::Event) {
                auto* cb = std::get_if<AnimationEventCallback>(&sub.callback);
                if (cb && *cb) {
                    (*cb)(event);
                    impl_->stats_.eventsDispatched++;
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    impl_->stats_.updateTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

void AnimationSystem::applyIK(AnimatorData& animator, SkeletonData& skeleton) {
    // Apply two-bone IK
    auto chainIt = impl_->ikChains_.find(animator.skeleton);
    if (chainIt != impl_->ikChains_.end()) {
        for (const auto& chain : chainIt->second) {
            auto targetIt = animator.twoBoneTargets.find(chain.name);
            if (targetIt == animator.twoBoneTargets.end() || !targetIt->second.enabled) continue;

            const auto& target = targetIt->second;
            if (target.weight < 0.001f) continue;

            impl_->stats_.ikSolves++;

            ozz::animation::IKTwoBoneJob ikJob;
            ikJob.target = ozz::math::simd_float4::Load3PtrU(&target.targetPosition.x);
            ikJob.pole_vector = ozz::math::simd_float4::Load3PtrU(&target.poleVector.x);
            ikJob.mid_axis = ozz::math::simd_float4::z_axis();
            ikJob.weight = target.weight;
            ikJob.soften = 0.95f;
            ikJob.twist_angle = 0.0f;

            ikJob.start_joint = &animator.modelMatrices[chain.rootBoneIndex];
            ikJob.mid_joint = &animator.modelMatrices[chain.midBoneIndex];
            ikJob.end_joint = &animator.modelMatrices[chain.tipBoneIndex];

            ozz::math::SimdQuaternion startCorrection, midCorrection;
            ikJob.start_joint_correction = &startCorrection;
            ikJob.mid_joint_correction = &midCorrection;
            ikJob.reached = nullptr;

            ikJob.Run();
        }
    }

    // Apply aim IK
    auto aimIt = impl_->ikAims_.find(animator.skeleton);
    if (aimIt != impl_->ikAims_.end()) {
        for (const auto& aim : aimIt->second) {
            auto targetIt = animator.aimTargets.find(aim.name);
            if (targetIt == animator.aimTargets.end() || !targetIt->second.enabled) continue;

            const auto& target = targetIt->second;
            if (target.weight < 0.001f) continue;

            impl_->stats_.ikSolves++;

            ozz::animation::IKAimJob ikJob;
            ikJob.target = ozz::math::simd_float4::Load3PtrU(&target.targetPosition.x);
            ikJob.forward = ozz::math::simd_float4::Load3PtrU(&aim.aimAxis.x);
            ikJob.up = ozz::math::simd_float4::Load3PtrU(&aim.upAxis.x);
            ikJob.pole_vector = ozz::math::simd_float4::Load3PtrU(&target.worldUp.x);
            ikJob.weight = target.weight;
            ikJob.joint = &animator.modelMatrices[aim.boneIndex];

            ozz::math::SimdQuaternion correction;
            ikJob.joint_correction = &correction;
            ikJob.reached = nullptr;

            ikJob.Run();
        }
    }

    // Compute final skinning matrices after IK: ModelSpace * InverseBindPose
    // Use ozz-to-bone mapping since ozz reorders joints
    std::size_t boneCount = animator.boneTransforms.size();
    for (std::size_t ozzIdx = 0; ozzIdx < boneCount; ++ozzIdx) {
        std::int32_t boneIdx = skeleton.ozzToBoneIndex[ozzIdx];
        Mat4 modelPose = fromOzz(animator.modelMatrices[ozzIdx]);
        animator.boneTransforms[boneIdx] = modelPose * skeleton.bones[boneIdx].inverseBindPose;
    }
}

void AnimationSystem::extractRootMotion(AnimatorData& animator, SkeletonData& skeleton) {
    if (skeleton.bones.empty()) return;

    std::int32_t rootIdx = animator.rootMotionConfig.rootBoneIndex;
    if (rootIdx < 0) rootIdx = skeleton.rootBoneIndex;
    if (rootIdx < 0 || static_cast<std::size_t>(rootIdx) >= animator.boneTransforms.size()) return;

    // Get root bone position from MODEL SPACE pose (before skinning matrix)
    // This gives us the animated position of the root bone
    Vec3 currentPos, currentScale;
    Quat currentRot;
    decomposeTransform(animator.modelSpacePoses[rootIdx], currentPos, currentRot, currentScale);

    // Debug: Log root motion values periodically
    static int frameCounter = 0;
    bool shouldLog = (frameCounter++ % 60 == 0);

    // Calculate deltas (or initialize on first frame)
    animator.currentRootMotion.deltaPosition = Vec3{0.0f};
    animator.currentRootMotion.deltaRotation = Quat{1.0f, 0.0f, 0.0f, 0.0f};

    // First frame initialization - set lastRootPosition to current to avoid huge delta
    if (!animator.rootMotionInitialized) {
        animator.lastRootPosition = currentPos;
        animator.lastRootRotation = currentRot;
        animator.rootMotionInitialized = true;
        spdlog::info("[RootMotion] Initialized lastRootPosition to ({:.3f}, {:.3f}, {:.3f})",
            currentPos.x, currentPos.y, currentPos.z);
        // Delta stays 0 on first frame, but we continue to apply cancel offset below
    } else {
        // Calculate deltas from last frame
        if (animator.rootMotionConfig.extractTranslationX) {
            animator.currentRootMotion.deltaPosition.x = currentPos.x - animator.lastRootPosition.x;
        }
        if (animator.rootMotionConfig.extractTranslationY) {
            animator.currentRootMotion.deltaPosition.y = currentPos.y - animator.lastRootPosition.y;
        }
        if (animator.rootMotionConfig.extractTranslationZ) {
            animator.currentRootMotion.deltaPosition.z = currentPos.z - animator.lastRootPosition.z;
        }

        if (animator.rootMotionConfig.extractRotationY) {
            animator.currentRootMotion.deltaRotation = glm::inverse(animator.lastRootRotation) * currentRot;
        }

        animator.currentRootMotion.totalPosition += animator.currentRootMotion.deltaPosition;
        animator.currentRootMotion.totalRotation = animator.currentRootMotion.totalRotation * animator.currentRootMotion.deltaRotation;

        animator.lastRootPosition = currentPos;
        animator.lastRootRotation = currentRot;

        if (shouldLog) {
            spdlog::info("[RootMotion] Root bone {} pos: ({:.3f}, {:.3f}, {:.3f}), Delta: ({:.4f}, {:.4f}, {:.4f})",
                rootIdx, currentPos.x, currentPos.y, currentPos.z,
                animator.currentRootMotion.deltaPosition.x,
                animator.currentRootMotion.deltaPosition.y,
                animator.currentRootMotion.deltaPosition.z);
        }
    }

    animator.currentRootMotion.hasTranslation =
        animator.rootMotionConfig.extractTranslationX ||
        animator.rootMotionConfig.extractTranslationY ||
        animator.rootMotionConfig.extractTranslationZ;
    animator.currentRootMotion.hasRotation = animator.rootMotionConfig.extractRotationY;

    // Cancel out root bone motion from bone transforms so skeleton stays at origin
    // The motion will be applied to the entity position instead via getRootMotion()
    if (animator.currentRootMotion.hasTranslation) {
        // Get the skeleton's rest/bind pose root position
        // For most characters, the root bone's bind pose is at origin (0,0,0)
        // The inverseBindPose transforms from model to bone space
        // bindPose = inverse(inverseBindPose), position is column 3
        Vec3 bindPoseRoot{0.0f};
        if (!skeleton.bones.empty()) {
            Mat4 bindPose = glm::inverse(skeleton.bones[0].inverseBindPose);
            bindPoseRoot = Vec3{bindPose[3][0], bindPose[3][1], bindPose[3][2]};
        }

        // Calculate how far the root has moved from bind pose
        Vec3 cancelOffset{0.0f};
        if (animator.rootMotionConfig.extractTranslationX) {
            cancelOffset.x = bindPoseRoot.x - currentPos.x;
        }
        if (animator.rootMotionConfig.extractTranslationY) {
            cancelOffset.y = bindPoseRoot.y - currentPos.y;
        }
        if (animator.rootMotionConfig.extractTranslationZ) {
            cancelOffset.z = bindPoseRoot.z - currentPos.z;
        }

        if (shouldLog) {
            spdlog::info("[RootMotion] BindPose: ({:.3f}, {:.3f}, {:.3f}), CancelOffset: ({:.3f}, {:.3f}, {:.3f})",
                bindPoseRoot.x, bindPoseRoot.y, bindPoseRoot.z,
                cancelOffset.x, cancelOffset.y, cancelOffset.z);
        }

        // Apply cancel offset to ALL bones to keep skeleton centered at bind pose
        for (std::size_t i = 0; i < animator.boneTransforms.size(); ++i) {
            animator.boneTransforms[i][3][0] += cancelOffset.x;
            animator.boneTransforms[i][3][1] += cancelOffset.y;
            animator.boneTransforms[i][3][2] += cancelOffset.z;

            animator.modelSpacePoses[i][3][0] += cancelOffset.x;
            animator.modelSpacePoses[i][3][1] += cancelOffset.y;
            animator.modelSpacePoses[i][3][2] += cancelOffset.z;
        }
    }
}

//==========================================================================
// Skeleton Management
//==========================================================================

Result<SkeletonHandle, AnimationError> AnimationSystem::createSkeleton(const ModelData& modelData) {
    if (modelData.bones.empty()) {
        return std::unexpected(AnimationError::EmptyBoneData);
    }

    // Build index maps for node lookup
    std::unordered_map<std::string, std::size_t> nodeNameToIndex;
    for (std::size_t i = 0; i < modelData.nodes.size(); ++i) {
        nodeNameToIndex[modelData.nodes[i].name] = i;
    }

    // Helper to convert flat float[16] to Mat4
    auto floatToMat4 = [](const float* m) -> Mat4 {
        Mat4 result;
        for (int j = 0; j < 16; ++j) {
            result[j / 4][j % 4] = m[j];
        }
        return result;
    };

    // For each bone, compute the accumulated local transform by traversing UP
    // through any Assimp intermediate nodes ($AssimpFbx$_Translation, etc.)
    // until we reach the bone's actual parent bone (or root).
    //
    // Assimp splits FBX transforms into multiple nodes:
    //   ParentBone -> Translation -> PreRotation -> Rotation -> BoneName
    // We need to multiply all these together to get the true local transform.
    auto computeAccumulatedLocalTransform = [&](const std::string& boneName, int boneParentIdx) -> Mat4 {
        auto it = nodeNameToIndex.find(boneName);
        if (it == nodeNameToIndex.end()) {
            return Mat4{1.0f};  // Identity fallback
        }

        std::size_t nodeIdx = it->second;
        Mat4 accumulated = floatToMat4(modelData.nodes[nodeIdx].localTransform);

        // Walk up through intermediate nodes
        int parentNodeIdx = modelData.nodes[nodeIdx].parentIndex;
        while (parentNodeIdx >= 0) {
            const auto& parentNode = modelData.nodes[parentNodeIdx];

            // Check if this parent is an Assimp intermediate node (contains $AssimpFbx$)
            if (parentNode.name.find("$AssimpFbx$") != std::string::npos) {
                // Multiply parent's transform on the LEFT (parent * child)
                Mat4 parentMat = floatToMat4(parentNode.localTransform);
                accumulated = parentMat * accumulated;
                parentNodeIdx = parentNode.parentIndex;
            } else {
                // This is a real bone or the root - stop here
                break;
            }
        }

        return accumulated;
    };

    // Convert ModelData::Bone to BoneInfo
    std::vector<BoneInfo> bones;
    bones.reserve(modelData.bones.size());

    for (std::size_t i = 0; i < modelData.bones.size(); ++i) {
        const auto& srcBone = modelData.bones[i];
        BoneInfo bone;
        bone.name = srcBone.name;
        bone.index = static_cast<std::int32_t>(i);
        bone.parentIndex = srcBone.parentIndex;

        // Copy inverse bind pose (offset matrix)
        for (int j = 0; j < 16; ++j) {
            bone.inverseBindPose[j / 4][j % 4] = srcBone.offsetMatrix[j];
        }

        // Compute accumulated local transform (handles Assimp's $AssimpFbx$ intermediate nodes)
        bone.localBindPose = computeAccumulatedLocalTransform(srcBone.name, srcBone.parentIndex);

        // Debug: Print first 10 bones' local bind pose vs inverse bind pose
        if (i < 10) {
            Mat4 globalBindPose = glm::inverse(bone.inverseBindPose);
            spdlog::info("[Skeleton] Bone {} '{}' parent={}:", i, srcBone.name, srcBone.parentIndex);
            spdlog::info("  localBindPose pos: ({:.2f}, {:.2f}, {:.2f})",
                bone.localBindPose[3][0], bone.localBindPose[3][1], bone.localBindPose[3][2]);
            spdlog::info("  globalBindPose pos: ({:.2f}, {:.2f}, {:.2f})",
                globalBindPose[3][0], globalBindPose[3][1], globalBindPose[3][2]);
        }

        // Extract position, rotation, scale from local bind pose
        bone.localPosition = Vec3{bone.localBindPose[3][0], bone.localBindPose[3][1], bone.localBindPose[3][2]};

        // Extract rotation from the upper-left 3x3 (assuming no shear)
        glm::mat3 rotMat = glm::mat3(bone.localBindPose);
        bone.localScale = Vec3{
            glm::length(glm::vec3(rotMat[0])),
            glm::length(glm::vec3(rotMat[1])),
            glm::length(glm::vec3(rotMat[2]))
        };

        // Normalize rotation matrix
        if (bone.localScale.x > 0.0001f) rotMat[0] /= bone.localScale.x;
        if (bone.localScale.y > 0.0001f) rotMat[1] /= bone.localScale.y;
        if (bone.localScale.z > 0.0001f) rotMat[2] /= bone.localScale.z;

        bone.localRotation = glm::quat_cast(rotMat);

        bones.push_back(bone);
    }

    return createSkeleton(std::span<const BoneInfo>(bones));
}

Result<SkeletonHandle, AnimationError> AnimationSystem::createSkeleton(std::span<const BoneInfo> bones) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSkeleton);
    if (bones.empty()) return std::unexpected(AnimationError::EmptyBoneData);

    SkeletonData data;
    data.handle = impl_->nextSkeletonHandle_++;
    data.bones.assign(bones.begin(), bones.end());

    // Build name lookup
    for (std::size_t i = 0; i < bones.size(); ++i) {
        data.boneNameToIndex[bones[i].name] = static_cast<std::int32_t>(i);
        if (bones[i].parentIndex < 0) {
            data.rootBoneIndex = static_cast<std::int32_t>(i);
        }
    }

    // Build ozz skeleton from our bone data
    ozz::animation::offline::RawSkeleton rawSkeleton;

    // Build joint hierarchy recursively
    std::function<void(ozz::animation::offline::RawSkeleton::Joint&, std::int32_t)> buildJoint;
    buildJoint = [&](ozz::animation::offline::RawSkeleton::Joint& joint, std::int32_t boneIdx) {
        const auto& bone = bones[boneIdx];
        joint.name = bone.name;
        joint.transform.translation = toOzz(bone.localPosition);
        joint.transform.rotation = toOzz(bone.localRotation);
        joint.transform.scale = toOzz(bone.localScale);

        // Find children
        for (std::size_t i = 0; i < bones.size(); ++i) {
            if (bones[i].parentIndex == boneIdx) {
                joint.children.emplace_back();
                buildJoint(joint.children.back(), static_cast<std::int32_t>(i));
            }
        }
    };

    // Find root bones and build hierarchy
    for (std::size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].parentIndex < 0) {
            rawSkeleton.roots.emplace_back();
            buildJoint(rawSkeleton.roots.back(), static_cast<std::int32_t>(i));
        }
    }

    // Validate and build runtime skeleton
    if (!rawSkeleton.Validate()) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    ozz::animation::offline::SkeletonBuilder builder;
    auto skeleton = builder(rawSkeleton);
    if (!skeleton) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    data.ozzSkeleton = std::move(*skeleton);

    // Build mapping from ozz joint index to our bone index
    // ozz reorders joints during skeleton building (depth-first traversal)
    int numJoints = data.ozzSkeleton.num_joints();
    data.ozzToBoneIndex.resize(numJoints);

    auto jointNames = data.ozzSkeleton.joint_names();
    for (int ozzIdx = 0; ozzIdx < numJoints; ++ozzIdx) {
        const char* jointName = jointNames[ozzIdx];
        auto it = data.boneNameToIndex.find(jointName);
        if (it != data.boneNameToIndex.end()) {
            data.ozzToBoneIndex[ozzIdx] = it->second;
        } else {
            // Fallback to same index if name not found (shouldn't happen)
            data.ozzToBoneIndex[ozzIdx] = ozzIdx;
            spdlog::warn("[AnimationSystem] Joint '{}' not found in bone name map", jointName);
        }
    }

    // Build reverse mapping: our bone index -> ozz joint index
    // This is needed when building animations (tracks are in ozz order)
    int numBones = static_cast<int>(data.bones.size());
    data.boneToOzzIndex.resize(numBones, -1);  // -1 means bone not in ozz skeleton
    for (int ozzIdx = 0; ozzIdx < numJoints; ++ozzIdx) {
        std::int32_t boneIdx = data.ozzToBoneIndex[ozzIdx];
        if (boneIdx >= 0 && boneIdx < numBones) {
            data.boneToOzzIndex[boneIdx] = ozzIdx;
        }
    }

    impl_->skeletons_[data.handle] = std::move(data);
    impl_->stats_.skeletonCount++;

    return data.handle;
}

void AnimationSystem::destroySkeleton(SkeletonHandle skeleton) {
    if (!impl_) return;

    auto it = impl_->skeletons_.find(skeleton);
    if (it == impl_->skeletons_.end()) return;

    // Destroy associated clips
    auto clipIt = impl_->skeletonClips_.find(skeleton);
    if (clipIt != impl_->skeletonClips_.end()) {
        for (auto clipHandle : clipIt->second) {
            impl_->clips_.erase(clipHandle);
            impl_->stats_.clipCount--;
        }
        impl_->skeletonClips_.erase(clipIt);
    }

    // Destroy associated sockets
    auto socketIt = impl_->skeletonSockets_.find(skeleton);
    if (socketIt != impl_->skeletonSockets_.end()) {
        for (auto socketHandle : socketIt->second) {
            impl_->sockets_.erase(socketHandle);
            impl_->stats_.socketDefCount--;
        }
        impl_->skeletonSockets_.erase(socketIt);
    }

    // Destroy associated animators
    std::vector<AnimatorHandle> toDestroy;
    for (const auto& [handle, animator] : impl_->animators_) {
        if (animator.skeleton == skeleton) {
            toDestroy.push_back(handle);
        }
    }
    for (auto h : toDestroy) {
        impl_->animators_.erase(h);
        impl_->stats_.animatorCount--;
    }

    // Remove IK data
    impl_->ikChains_.erase(skeleton);
    impl_->ikAims_.erase(skeleton);

    impl_->skeletons_.erase(it);
    impl_->stats_.skeletonCount--;
}

bool AnimationSystem::isValidSkeleton(SkeletonHandle skeleton) const {
    if (!impl_) return false;
    return impl_->skeletons_.contains(skeleton);
}

SkeletonInfo AnimationSystem::getSkeletonInfo(SkeletonHandle skeleton) const {
    SkeletonInfo info;
    if (!impl_) return info;

    auto it = impl_->skeletons_.find(skeleton);
    if (it == impl_->skeletons_.end()) return info;

    info.boneCount = static_cast<std::uint32_t>(it->second.bones.size());
    info.bones = it->second.bones;
    info.rootBoneIndex = it->second.rootBoneIndex;
    info.bounds = it->second.bounds;
    return info;
}

std::int32_t AnimationSystem::findBoneIndex(SkeletonHandle skeleton, std::string_view boneName) const {
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
    SkeletonHandle skeleton,
    const ModelData& modelData,
    std::string_view clipName) {
    // TODO: Extract from ModelData when asset pipeline supports it
    return std::unexpected(AnimationError::NoAnimationData);
}

std::vector<AnimationClipHandle> AnimationSystem::createAnimationClips(
    SkeletonHandle skeleton,
    const ModelData& modelData) {
    if (!impl_) return {};

    // Find the skeleton data
    auto skelIt = impl_->skeletons_.find(skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        spdlog::error("[AnimationSystem] Cannot create clips - invalid skeleton handle");
        return {};
    }
    const auto& skeletonData = skelIt->second;

    std::vector<AnimationClipHandle> clipHandles;

    for (const auto& srcAnim : modelData.animations) {
        AnimationClipData clip;
        clip.handle = impl_->nextClipHandle_++;
        clip.skeleton = skeleton;
        clip.name = srcAnim.name;
        clip.duration = srcAnim.duration / srcAnim.ticksPerSecond; // Convert to seconds
        clip.ticksPerSecond = srcAnim.ticksPerSecond;
        clip.looping = true;

        // Convert channels
        int matchedChannels = 0;
        int unmatchedChannels = 0;
        for (const auto& srcChannel : srcAnim.channels) {
            // Look up bone index by name in the skeleton (handles animation-only files)
            int boneIndex = srcChannel.boneIndex;
            if (boneIndex < 0 && !srcChannel.boneName.empty()) {
                auto nameIt = skeletonData.boneNameToIndex.find(srcChannel.boneName);
                if (nameIt != skeletonData.boneNameToIndex.end()) {
                    boneIndex = nameIt->second;
                }
            }

            if (boneIndex < 0) {
                // Log first few unmatched channels
                if (unmatchedChannels < 5) {
                    spdlog::warn("[AnimationSystem] Animation channel '{}' not found in skeleton",
                                 srcChannel.boneName);
                }
                unmatchedChannels++;
                continue;
            }
            matchedChannels++;

            AnimationChannel channel;
            channel.boneIndex = boneIndex;

            // Convert keyframes
            for (const auto& srcKey : srcChannel.keyframes) {
                AnimationKeyframe key;
                key.time = srcKey.time / srcAnim.ticksPerSecond; // Convert to seconds
                key.position = Vec3{srcKey.translation[0], srcKey.translation[1], srcKey.translation[2]};
                // ModelData uses xyzw, Quat is wxyz
                key.rotation = Quat{srcKey.rotation[3], srcKey.rotation[0], srcKey.rotation[1], srcKey.rotation[2]};
                key.scale = Vec3{srcKey.scale[0], srcKey.scale[1], srcKey.scale[2]};
                channel.keyframes.push_back(key);
            }

            clip.channels.push_back(channel);
        }

        // Build ozz animation from the raw animation data
        ozz::animation::offline::RawAnimation rawAnimation;
        rawAnimation.duration = clip.duration;
        rawAnimation.tracks.resize(skeletonData.ozzSkeleton.num_joints());

        // Fill tracks from channels
        // IMPORTANT: Animation tracks are in ozz joint order, not our bone order!
        // We must convert from our bone index to ozz joint index.
        for (const auto& channel : clip.channels) {
            if (channel.boneIndex < 0 || channel.boneIndex >= static_cast<int>(skeletonData.boneToOzzIndex.size())) {
                continue;
            }

            // Convert from our bone index to ozz joint index
            int ozzIdx = skeletonData.boneToOzzIndex[channel.boneIndex];
            if (ozzIdx < 0 || ozzIdx >= static_cast<int>(rawAnimation.tracks.size())) {
                continue;  // Bone not in ozz skeleton
            }

            auto& track = rawAnimation.tracks[ozzIdx];

            for (const auto& key : channel.keyframes) {
                track.translations.push_back({key.time, toOzz(key.position)});
                track.rotations.push_back({key.time, toOzz(key.rotation)});
                track.scales.push_back({key.time, toOzz(key.scale)});
            }
        }

        // Ensure all tracks have at least one keyframe (ozz requirement)
        for (auto& track : rawAnimation.tracks) {
            if (track.translations.empty()) {
                track.translations.push_back({0.0f, ozz::math::Float3{0, 0, 0}});
            }
            if (track.rotations.empty()) {
                track.rotations.push_back({0.0f, ozz::math::Quaternion{0, 0, 0, 1}});
            }
            if (track.scales.empty()) {
                track.scales.push_back({0.0f, ozz::math::Float3{1, 1, 1}});
            }
        }

        // Build the runtime animation
        ozz::animation::offline::AnimationBuilder builder;
        auto builtAnim = builder(rawAnimation);
        if (!builtAnim) {
            spdlog::error("[AnimationSystem] Failed to build ozz animation for: {}", srcAnim.name);
            continue;
        }

        clip.ozzAnimation = std::move(*builtAnim);

        // Store the clip
        impl_->clips_[clip.handle] = std::move(clip);
        impl_->skeletonClips_[skeleton].push_back(clip.handle);
        impl_->stats_.clipCount++;

        clipHandles.push_back(clip.handle);
        spdlog::info("[AnimationSystem] Created animation clip: {} (duration: {:.2f}s, {} matched channels, {} unmatched)",
                     srcAnim.name, clip.duration, matchedChannels, unmatchedChannels);
    }

    return clipHandles;
}

void AnimationSystem::destroyAnimationClip(AnimationClipHandle clip) {
    if (!impl_) return;

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return;

    // Remove from skeleton mapping
    auto skelIt = impl_->skeletonClips_.find(it->second.skeleton);
    if (skelIt != impl_->skeletonClips_.end()) {
        auto& clips = skelIt->second;
        clips.erase(std::remove(clips.begin(), clips.end(), clip), clips.end());
    }

    impl_->clips_.erase(it);
    impl_->stats_.clipCount--;
}

bool AnimationSystem::isValidClip(AnimationClipHandle clip) const {
    if (!impl_) return false;
    return impl_->clips_.contains(clip);
}

AnimationClipInfo AnimationSystem::getAnimationClipInfo(AnimationClipHandle clip) const {
    AnimationClipInfo info;
    if (!impl_) return info;

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return info;

    info.name = it->second.name;
    info.skeleton = it->second.skeleton;
    info.duration = it->second.duration;
    info.ticksPerSecond = it->second.ticksPerSecond;
    info.looping = it->second.looping;
    info.hasRootMotion = it->second.hasRootMotion;
    info.channelCount = static_cast<std::uint32_t>(it->second.channels.size());

    std::uint32_t totalKeyframes = 0;
    for (const auto& channel : it->second.channels) {
        totalKeyframes += static_cast<std::uint32_t>(channel.keyframes.size());
    }
    info.keyframeCount = totalKeyframes;

    return info;
}

AnimationClipHandle AnimationSystem::findClip(SkeletonHandle skeleton, std::string_view clipName) const {
    if (!impl_) return AnimationHandles::InvalidClip;

    auto skelIt = impl_->skeletonClips_.find(skeleton);
    if (skelIt == impl_->skeletonClips_.end()) return AnimationHandles::InvalidClip;

    for (auto clipHandle : skelIt->second) {
        auto clipIt = impl_->clips_.find(clipHandle);
        if (clipIt != impl_->clips_.end() && clipIt->second.name == clipName) {
            return clipHandle;
        }
    }
    return AnimationHandles::InvalidClip;
}

std::vector<AnimationClipHandle> AnimationSystem::getClipsForSkeleton(SkeletonHandle skeleton) const {
    if (!impl_) return {};

    auto it = impl_->skeletonClips_.find(skeleton);
    if (it == impl_->skeletonClips_.end()) return {};

    return it->second;
}

std::vector<std::string> AnimationSystem::getClipNames(SkeletonHandle skeleton) const {
    std::vector<std::string> names;
    if (!impl_) return names;

    auto clips = getClipsForSkeleton(skeleton);
    for (auto clipHandle : clips) {
        auto it = impl_->clips_.find(clipHandle);
        if (it != impl_->clips_.end()) {
            names.push_back(it->second.name);
        }
    }
    return names;
}

//==========================================================================
// Animation Events (Clip-level)
//==========================================================================

Result<void, AnimationError> AnimationSystem::addClipEvent(AnimationClipHandle clip, const AnimationEventDef& event) {
    if (!impl_) return std::unexpected(AnimationError::InvalidClip);

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return std::unexpected(AnimationError::ClipNotFound);

    it->second.events.push_back(event);
    return {};
}

Result<void, AnimationError> AnimationSystem::removeClipEvent(AnimationClipHandle clip, std::string_view eventName) {
    if (!impl_) return std::unexpected(AnimationError::InvalidClip);

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return std::unexpected(AnimationError::ClipNotFound);

    auto& events = it->second.events;
    events.erase(
        std::remove_if(events.begin(), events.end(),
            [&](const AnimationEventDef& e) { return e.name == eventName; }),
        events.end());
    return {};
}

void AnimationSystem::clearClipEvents(AnimationClipHandle clip) {
    if (!impl_) return;

    auto it = impl_->clips_.find(clip);
    if (it != impl_->clips_.end()) {
        it->second.events.clear();
    }
}

std::vector<AnimationEventDef> AnimationSystem::getClipEvents(AnimationClipHandle clip) const {
    if (!impl_) return {};

    auto it = impl_->clips_.find(clip);
    if (it == impl_->clips_.end()) return {};

    return it->second.events;
}

//==========================================================================
// Animator Management
//==========================================================================

Result<AnimatorHandle, AnimationError> AnimationSystem::createAnimator(SkeletonHandle skeleton) {
    if (!impl_) return std::unexpected(AnimationError::InvalidAnimator);

    auto skelIt = impl_->skeletons_.find(skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        return std::unexpected(AnimationError::InvalidSkeleton);
    }

    AnimatorData data;
    data.handle = impl_->nextAnimatorHandle_++;
    data.skeleton = skeleton;
    data.layers.resize(1);  // Start with one layer

    impl_->animators_[data.handle] = std::move(data);
    impl_->stats_.animatorCount++;

    return data.handle;
}

void AnimationSystem::destroyAnimator(AnimatorHandle animator) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    // Remove subscriptions for this animator
    impl_->subscriptions_.erase(
        std::remove_if(impl_->subscriptions_.begin(), impl_->subscriptions_.end(),
            [animator](const EventSubscription& sub) { return sub.animator == animator; }),
        impl_->subscriptions_.end());

    impl_->animators_.erase(it);
    impl_->stats_.animatorCount--;
}

bool AnimationSystem::isValidAnimator(AnimatorHandle animator) const {
    if (!impl_) return false;
    return impl_->animators_.contains(animator);
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

void AnimationSystem::play(AnimatorHandle animator, AnimationClipHandle clip, float transitionTime) {
    AnimationPlayConfig config;
    config.clip = clip;
    config.blendInTime = transitionTime;
    config.layer = 0;
    play(animator, config);
}

void AnimationSystem::play(AnimatorHandle animator, std::string_view clipName, float transitionTime) {
    if (!impl_) return;

    auto animIt = impl_->animators_.find(animator);
    if (animIt == impl_->animators_.end()) return;

    auto clip = findClip(animIt->second.skeleton, clipName);
    if (clip == AnimationHandles::InvalidClip) return;

    play(animator, clip, transitionTime);
}

void AnimationSystem::play(AnimatorHandle animator, const AnimationPlayConfig& config) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    auto& anim = it->second;

    // Ensure we have enough layers
    while (anim.layers.size() <= config.layer) {
        anim.layers.emplace_back();
    }

    auto& layer = anim.layers[config.layer];

    // Notify layer change
    AnimationClipHandle prevClip = layer.clip;
    AnimationClipHandle newClip = config.clip;

    // If blending and there's a current animation, preserve it as outgoing for crossfade
    // Note: We allow crossfade even if layer.playing is false (animation completed)
    // because we still want smooth transitions from completed Once animations
    bool shouldCrossfade = config.blendInTime > 0.0f &&
                           layer.clip != AnimationHandles::InvalidClip;

    if (shouldCrossfade) {
        // Preserve current clip as outgoing for crossfade
        // Note: If already crossfading, the previous outgoing animation is lost.
        // This is a deliberate simplification - most animation systems only support
        // one crossfade at a time. The current animation becomes the new outgoing.
        layer.outgoingClip = layer.clip;
        layer.outgoingTime = layer.time;
        layer.outgoingSpeed = layer.speed;
        layer.outgoingWrapMode = layer.wrapMode;
        // Transfer sampling context and transforms to outgoing
        layer.outgoingSamplingContext = std::move(layer.samplingContext);
        layer.outgoingLocalTransforms = std::move(layer.localTransforms);

        spdlog::info("[Crossfade] Starting crossfade from clip {} to clip {} over {:.2f}s",
            layer.outgoingClip, config.clip, config.blendInTime);
    } else {
        // No crossfade - clear any outgoing animation
        layer.outgoingClip = AnimationHandles::InvalidClip;
        layer.outgoingTime = 0.0f;
        layer.outgoingSamplingContext.reset();
        layer.outgoingLocalTransforms.clear();
    }

    // Set up the new incoming animation
    layer.clip = config.clip;
    layer.clipName = config.clipName;
    layer.speed = config.speed;
    layer.weight = config.weight;
    layer.wrapMode = config.wrapMode;
    layer.blendMode = config.blendMode;
    layer.playing = true;
    layer.paused = false;
    layer.samplingContext.reset();  // Will be re-created on first sample
    layer.localTransforms.clear();

    // Always start new animation from the configured start time
    // Animation sync (for foot phase matching) would need to be opt-in per animation
    // and only between similar locomotion cycles - too complex for now
    layer.time = config.startTime;

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
                (*cb)(animator, config.layer, prevClip, newClip);
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

void AnimationSystem::stopLayer(AnimatorHandle animator, std::uint32_t layer, float fadeOutTime) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    if (layer >= it->second.layers.size()) return;

    auto& layerData = it->second.layers[layer];

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

AnimationLayerState AnimationSystem::getLayerState(AnimatorHandle animator, std::uint32_t layer) const {
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

    // Compute normalized time
    auto clipIt = impl_->clips_.find(layerData.clip);
    if (clipIt != impl_->clips_.end() && clipIt->second.duration > 0) {
        state.normalizedTime = layerData.time / clipIt->second.duration;
    }

    return state;
}

void AnimationSystem::setLayerWeight(AnimatorHandle animator, std::uint32_t layer, float weight) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    while (it->second.layers.size() <= layer) {
        it->second.layers.emplace_back();
    }

    it->second.layers[layer].weight = std::clamp(weight, 0.0f, 1.0f);
}

float AnimationSystem::getLayerWeight(AnimatorHandle animator, std::uint32_t layer) const {
    if (!impl_) return 1.0f;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return 1.0f;
    if (layer >= it->second.layers.size()) return 1.0f;

    return it->second.layers[layer].weight;
}

void AnimationSystem::setLayerBlendMode(AnimatorHandle animator, std::uint32_t layer, AnimationBlendMode mode) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    while (it->second.layers.size() <= layer) {
        it->second.layers.emplace_back();
    }

    it->second.layers[layer].blendMode = mode;
}

std::uint32_t AnimationSystem::getLayerCount(AnimatorHandle animator) const {
    if (!impl_) return 0;

    auto it = impl_->animators_.find(animator);
    return it != impl_->animators_.end() ? static_cast<std::uint32_t>(it->second.layers.size()) : 0;
}

void AnimationSystem::setLayerBoneMask(AnimatorHandle animator, std::uint32_t layer, const std::set<std::uint32_t>& boneMask) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    while (it->second.layers.size() <= layer) {
        it->second.layers.emplace_back();
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

void AnimationSystem::setCurrentTime(AnimatorHandle animator, float time, std::uint32_t layer) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;
    if (layer >= it->second.layers.size()) return;

    it->second.layers[layer].time = time;
}

void AnimationSystem::setNormalizedTime(AnimatorHandle animator, float normalizedTime, std::uint32_t layer) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;
    if (layer >= it->second.layers.size()) return;

    const auto& layerData = it->second.layers[layer];
    auto clipIt = impl_->clips_.find(layerData.clip);
    if (clipIt == impl_->clips_.end()) return;

    it->second.layers[layer].time = normalizedTime * clipIt->second.duration;
}

float AnimationSystem::getClipDuration(AnimatorHandle animator, std::uint32_t layer) const {
    if (!impl_) return 0.0f;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return 0.0f;
    if (layer >= it->second.layers.size()) return 0.0f;

    auto clipIt = impl_->clips_.find(it->second.layers[layer].clip);
    return clipIt != impl_->clips_.end() ? clipIt->second.duration : 0.0f;
}

//==========================================================================
// Animator Bone Transforms
//==========================================================================

std::span<const Mat4> AnimationSystem::getBoneTransforms(AnimatorHandle animator) const {
    if (!impl_) return {};

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return {};

    return std::span<const Mat4>(it->second.boneTransforms);
}

std::span<const Mat4> AnimationSystem::getModelSpaceBonePoses(AnimatorHandle animator) const {
    if (!impl_) return {};

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return {};

    return std::span<const Mat4>(it->second.modelSpacePoses);
}

Mat4 AnimationSystem::getBoneTransform(AnimatorHandle animator, std::uint32_t boneIndex) const {
    if (!impl_) return Mat4{1.0f};

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return Mat4{1.0f};
    if (boneIndex >= it->second.boneTransforms.size()) return Mat4{1.0f};

    return it->second.boneTransforms[boneIndex];
}

Mat4 AnimationSystem::getBoneTransform(AnimatorHandle animator, std::string_view boneName) const {
    if (!impl_) return Mat4{1.0f};

    auto animIt = impl_->animators_.find(animator);
    if (animIt == impl_->animators_.end()) return Mat4{1.0f};

    auto skelIt = impl_->skeletons_.find(animIt->second.skeleton);
    if (skelIt == impl_->skeletons_.end()) return Mat4{1.0f};

    auto nameIt = skelIt->second.boneNameToIndex.find(std::string(boneName));
    if (nameIt == skelIt->second.boneNameToIndex.end()) return Mat4{1.0f};

    return getBoneTransform(animator, static_cast<std::uint32_t>(nameIt->second));
}

Mat4 AnimationSystem::getBoneWorldTransform(AnimatorHandle animator, std::uint32_t boneIndex, const Mat4& entityWorldMatrix) const {
    return entityWorldMatrix * getBoneTransform(animator, boneIndex);
}

//==========================================================================
// Socket System
//==========================================================================

Result<SocketHandle, AnimationError> AnimationSystem::defineSocket(SkeletonHandle skeleton, const SocketDef& def) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSocket);

    auto skelIt = impl_->skeletons_.find(skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        return std::unexpected(AnimationError::SkeletonNotFound);
    }

    // Check if bone exists
    auto boneIt = skelIt->second.boneNameToIndex.find(def.boneName);
    if (boneIt == skelIt->second.boneNameToIndex.end()) {
        return std::unexpected(AnimationError::SocketBoneNotFound);
    }

    // Check for duplicate name
    if (hasSocket(skeleton, def.name)) {
        return std::unexpected(AnimationError::SocketAlreadyExists);
    }

    SocketData data;
    data.handle = impl_->nextSocketHandle_++;
    data.skeleton = skeleton;
    data.name = def.name;
    data.boneIndex = static_cast<std::uint32_t>(boneIt->second);
    data.localPosition = def.localPosition;
    data.localRotation = def.localRotation;
    data.localScale = def.localScale;
    data.attachMode = def.attachMode;
    data.enabled = true;

    impl_->sockets_[data.handle] = std::move(data);
    impl_->skeletonSockets_[skeleton].push_back(data.handle);
    impl_->stats_.socketDefCount++;

    return data.handle;
}

std::vector<Result<SocketHandle, AnimationError>> AnimationSystem::defineSockets(
    SkeletonHandle skeleton,
    std::span<const SocketDef> defs) {

    std::vector<Result<SocketHandle, AnimationError>> results;
    results.reserve(defs.size());

    for (const auto& def : defs) {
        results.push_back(defineSocket(skeleton, def));
    }

    return results;
}

void AnimationSystem::removeSocket(SocketHandle socket) {
    if (!impl_) return;

    auto it = impl_->sockets_.find(socket);
    if (it == impl_->sockets_.end()) return;

    auto skelSocket = impl_->skeletonSockets_.find(it->second.skeleton);
    if (skelSocket != impl_->skeletonSockets_.end()) {
        auto& sockets = skelSocket->second;
        sockets.erase(std::remove(sockets.begin(), sockets.end(), socket), sockets.end());
    }

    impl_->sockets_.erase(it);
    impl_->stats_.socketDefCount--;
}

void AnimationSystem::removeSocket(SkeletonHandle skeleton, std::string_view name) {
    auto socket = findSocket(skeleton, name);
    if (socket != AnimationHandles::InvalidSocket) {
        removeSocket(socket);
    }
}

bool AnimationSystem::hasSocket(SkeletonHandle skeleton, std::string_view socketName) const {
    return findSocket(skeleton, socketName) != AnimationHandles::InvalidSocket;
}

SocketHandle AnimationSystem::findSocket(SkeletonHandle skeleton, std::string_view socketName) const {
    if (!impl_) return AnimationHandles::InvalidSocket;

    auto skelIt = impl_->skeletonSockets_.find(skeleton);
    if (skelIt == impl_->skeletonSockets_.end()) return AnimationHandles::InvalidSocket;

    for (auto handle : skelIt->second) {
        auto sockIt = impl_->sockets_.find(handle);
        if (sockIt != impl_->sockets_.end() && sockIt->second.name == socketName) {
            return handle;
        }
    }
    return AnimationHandles::InvalidSocket;
}

std::vector<SocketState> AnimationSystem::getSockets(SkeletonHandle skeleton) const {
    std::vector<SocketState> states;
    if (!impl_) return states;

    auto skelIt = impl_->skeletonSockets_.find(skeleton);
    if (skelIt == impl_->skeletonSockets_.end()) return states;

    for (auto handle : skelIt->second) {
        auto sockIt = impl_->sockets_.find(handle);
        if (sockIt != impl_->sockets_.end()) {
            SocketState state;
            state.handle = sockIt->second.handle;
            state.name = sockIt->second.name;
            state.boneIndex = sockIt->second.boneIndex;
            state.localTransform = composeTransform(
                sockIt->second.localPosition,
                sockIt->second.localRotation,
                sockIt->second.localScale);
            state.attachMode = sockIt->second.attachMode;
            state.enabled = sockIt->second.enabled;
            states.push_back(state);
        }
    }
    return states;
}

std::optional<SocketDef> AnimationSystem::getSocketDef(SocketHandle socket) const {
    if (!impl_) return std::nullopt;

    auto it = impl_->sockets_.find(socket);
    if (it == impl_->sockets_.end()) return std::nullopt;

    SocketDef def;
    def.name = it->second.name;

    auto skelIt = impl_->skeletons_.find(it->second.skeleton);
    if (skelIt != impl_->skeletons_.end() && it->second.boneIndex < skelIt->second.bones.size()) {
        def.boneName = skelIt->second.bones[it->second.boneIndex].name;
    }

    def.localPosition = it->second.localPosition;
    def.localRotation = it->second.localRotation;
    def.localScale = it->second.localScale;
    def.attachMode = it->second.attachMode;

    return def;
}

Result<SocketTransform, AnimationError> AnimationSystem::getSocketTransform(
    AnimatorHandle animator,
    std::string_view socketName,
    const Mat4& entityWorldMatrix) const {

    if (!impl_) return std::unexpected(AnimationError::InvalidSocket);

    auto animIt = impl_->animators_.find(animator);
    if (animIt == impl_->animators_.end()) {
        return std::unexpected(AnimationError::AnimatorNotFound);
    }

    auto socket = findSocket(animIt->second.skeleton, socketName);
    if (socket == AnimationHandles::InvalidSocket) {
        return std::unexpected(AnimationError::SocketNotFound);
    }

    return getSocketTransform(animator, socket, entityWorldMatrix);
}

Result<SocketTransform, AnimationError> AnimationSystem::getSocketTransform(
    AnimatorHandle animator,
    SocketHandle socket,
    const Mat4& entityWorldMatrix) const {

    if (!impl_) return std::unexpected(AnimationError::InvalidSocket);

    auto animIt = impl_->animators_.find(animator);
    if (animIt == impl_->animators_.end()) {
        return std::unexpected(AnimationError::AnimatorNotFound);
    }

    auto sockIt = impl_->sockets_.find(socket);
    if (sockIt == impl_->sockets_.end()) {
        return std::unexpected(AnimationError::SocketNotFound);
    }

    const auto& socketData = sockIt->second;
    if (socketData.boneIndex >= animIt->second.boneTransforms.size()) {
        return std::unexpected(AnimationError::InvalidBoneIndex);
    }

    impl_->stats_.socketQueries++;

    Mat4 boneWorld = entityWorldMatrix * animIt->second.boneTransforms[socketData.boneIndex];
    Mat4 socketLocal = composeTransform(socketData.localPosition, socketData.localRotation, socketData.localScale);
    Mat4 socketWorld = boneWorld * socketLocal;

    SocketTransform transform;
    transform.worldMatrix = socketWorld;
    decomposeTransform(socketWorld, transform.position, transform.rotation, transform.scale);

    // Extract direction vectors from rotation
    transform.forward = glm::normalize(transform.rotation * Vec3{0.0f, 0.0f, -1.0f});
    transform.up = glm::normalize(transform.rotation * Vec3{0.0f, 1.0f, 0.0f});
    transform.right = glm::normalize(transform.rotation * Vec3{1.0f, 0.0f, 0.0f});

    return transform;
}

std::vector<SocketTransform> AnimationSystem::getSocketTransforms(
    AnimatorHandle animator,
    std::span<const std::string_view> socketNames,
    const Mat4& entityWorldMatrix) const {

    std::vector<SocketTransform> transforms;
    transforms.reserve(socketNames.size());

    for (const auto& name : socketNames) {
        auto result = getSocketTransform(animator, name, entityWorldMatrix);
        if (result.has_value()) {
            transforms.push_back(result.value());
        } else {
            transforms.emplace_back();
        }
    }

    return transforms;
}

Result<void, AnimationError> AnimationSystem::setSocketLocalTransform(
    SocketHandle socket,
    const Vec3& position,
    const Quat& rotation,
    const Vec3& scale) {

    if (!impl_) return std::unexpected(AnimationError::InvalidSocket);

    auto it = impl_->sockets_.find(socket);
    if (it == impl_->sockets_.end()) {
        return std::unexpected(AnimationError::SocketNotFound);
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
    AnimatorHandle animator,
    const SocketRaycastDef& def,
    const Mat4& entityWorldMatrix,
    const IPhysics3DSystem& physics) const {

    SocketRaycastResult result;
    result.socketName = def.socketName;

    auto transform = getSocketTransform(animator, def.socketName, entityWorldMatrix);
    if (!transform.has_value()) {
        return result;
    }

    result.origin = transform->position;

    // Transform local direction to world space
    Vec3 worldDir = glm::normalize(transform->rotation * def.direction);
    result.direction = worldDir;

    // Perform physics raycast
    QueryFilter3D filter{.layerMask = def.collisionMask};
    auto hitResult = physics.raycast(result.origin, worldDir, def.maxDistance, filter);

    if (hitResult.has_value()) {
        result.hit = true;
        result.hitPoint = hitResult->point;
        result.hitNormal = hitResult->normal;
        result.distance = hitResult->distance;
        result.hitEntity = hitResult->entity;
        result.hitShapeIndex = hitResult->shapeIndex;
    }

    return result;
}

std::vector<SocketRaycastResult> AnimationSystem::raycastFromSockets(
    AnimatorHandle animator,
    std::span<const SocketRaycastDef> defs,
    const Mat4& entityWorldMatrix,
    const IPhysics3DSystem& physics) const {

    std::vector<SocketRaycastResult> results;
    results.reserve(defs.size());

    for (const auto& def : defs) {
        results.push_back(raycastFromSocket(animator, def, entityWorldMatrix, physics));
    }

    return results;
}

SocketRaycastResult AnimationSystem::sphereCastFromSocket(
    AnimatorHandle animator,
    const SocketRaycastDef& def,
    float sphereRadius,
    const Mat4& entityWorldMatrix,
    const IPhysics3DSystem& physics) const {

    SocketRaycastResult result;
    result.socketName = def.socketName;

    auto transform = getSocketTransform(animator, def.socketName, entityWorldMatrix);
    if (!transform.has_value()) {
        return result;
    }

    result.origin = transform->position;
    Vec3 worldDir = glm::normalize(transform->rotation * def.direction);
    result.direction = worldDir;

    // Perform physics sphere cast
    QueryFilter3D filter{.layerMask = def.collisionMask};
    auto hitResult = physics.sphereCast(result.origin, sphereRadius, worldDir, def.maxDistance, filter);

    if (hitResult.has_value()) {
        result.hit = true;
        result.hitPoint = hitResult->point;
        result.hitNormal = hitResult->normal;
        result.distance = hitResult->distance;
        result.hitEntity = hitResult->entity;
        // result.hitShapeIndex not available from sphereCast
    }

    return result;
}

//==========================================================================
// Inverse Kinematics: Chain Definition
//==========================================================================

Result<void, AnimationError> AnimationSystem::defineIKChain(SkeletonHandle skeleton, const IKTwoBoneChain& chain) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSkeleton);

    auto skelIt = impl_->skeletons_.find(skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        return std::unexpected(AnimationError::SkeletonNotFound);
    }

    IKChainData data;
    data.name = chain.name;

    auto rootIt = skelIt->second.boneNameToIndex.find(chain.rootBoneName);
    auto midIt = skelIt->second.boneNameToIndex.find(chain.midBoneName);
    auto tipIt = skelIt->second.boneNameToIndex.find(chain.tipBoneName);

    if (rootIt == skelIt->second.boneNameToIndex.end() ||
        midIt == skelIt->second.boneNameToIndex.end() ||
        tipIt == skelIt->second.boneNameToIndex.end()) {
        return std::unexpected(AnimationError::IKChainInvalid);
    }

    data.rootBoneIndex = rootIt->second;
    data.midBoneIndex = midIt->second;
    data.tipBoneIndex = tipIt->second;

    impl_->ikChains_[skeleton].push_back(std::move(data));
    impl_->stats_.ikChainCount++;

    return {};
}

Result<void, AnimationError> AnimationSystem::defineIKAim(SkeletonHandle skeleton, const IKAimConfig& config) {
    if (!impl_) return std::unexpected(AnimationError::InvalidSkeleton);

    auto skelIt = impl_->skeletons_.find(skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        return std::unexpected(AnimationError::SkeletonNotFound);
    }

    auto boneIt = skelIt->second.boneNameToIndex.find(config.boneName);
    if (boneIt == skelIt->second.boneNameToIndex.end()) {
        return std::unexpected(AnimationError::IKChainInvalid);
    }

    IKAimData data;
    data.name = config.name;
    data.boneIndex = boneIt->second;
    data.aimAxis = config.aimAxis;
    data.upAxis = config.upAxis;
    data.horizontalLimit = config.horizontalLimit;
    data.verticalLimit = config.verticalLimit;

    impl_->ikAims_[skeleton].push_back(std::move(data));
    impl_->stats_.ikChainCount++;

    return {};
}

void AnimationSystem::removeIKChain(SkeletonHandle skeleton, std::string_view name) {
    if (!impl_) return;

    auto it = impl_->ikChains_.find(skeleton);
    if (it == impl_->ikChains_.end()) return;

    auto& chains = it->second;
    auto chainIt = std::find_if(chains.begin(), chains.end(),
        [&](const IKChainData& c) { return c.name == name; });

    if (chainIt != chains.end()) {
        chains.erase(chainIt);
        impl_->stats_.ikChainCount--;
    }
}

void AnimationSystem::removeIKAim(SkeletonHandle skeleton, std::string_view name) {
    if (!impl_) return;

    auto it = impl_->ikAims_.find(skeleton);
    if (it == impl_->ikAims_.end()) return;

    auto& aims = it->second;
    auto aimIt = std::find_if(aims.begin(), aims.end(),
        [&](const IKAimData& a) { return a.name == name; });

    if (aimIt != aims.end()) {
        aims.erase(aimIt);
        impl_->stats_.ikChainCount--;
    }
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

std::optional<IKTwoBoneTarget> AnimationSystem::getIKTwoBoneTarget(AnimatorHandle animator, std::string_view chainName) const {
    if (!impl_) return std::nullopt;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return std::nullopt;

    auto targetIt = it->second.twoBoneTargets.find(std::string(chainName));
    if (targetIt == it->second.twoBoneTargets.end()) return std::nullopt;

    return targetIt->second;
}

std::optional<IKAimTarget> AnimationSystem::getIKAimTarget(AnimatorHandle animator, std::string_view configName) const {
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

void AnimationSystem::setIKWeight(AnimatorHandle animator, std::string_view targetName, float weight) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return;

    std::string name(targetName);
    auto twoBoneIt = it->second.twoBoneTargets.find(name);
    if (twoBoneIt != it->second.twoBoneTargets.end()) {
        twoBoneIt->second.weight = weight;
        return;
    }

    auto aimIt = it->second.aimTargets.find(name);
    if (aimIt != it->second.aimTargets.end()) {
        aimIt->second.weight = weight;
    }
}

float AnimationSystem::getIKWeight(AnimatorHandle animator, std::string_view targetName) const {
    if (!impl_) return 0.0f;

    auto it = impl_->animators_.find(animator);
    if (it == impl_->animators_.end()) return 0.0f;

    std::string name(targetName);
    auto twoBoneIt = it->second.twoBoneTargets.find(name);
    if (twoBoneIt != it->second.twoBoneTargets.end()) {
        return twoBoneIt->second.weight;
    }

    auto aimIt = it->second.aimTargets.find(name);
    if (aimIt != it->second.aimTargets.end()) {
        return aimIt->second.weight;
    }

    return 0.0f;
}

//==========================================================================
// Root Motion
//==========================================================================

void AnimationSystem::setRootMotionConfig(AnimatorHandle animator, const RootMotionConfig& config) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.rootMotionConfig = config;
    }
}

RootMotionConfig AnimationSystem::getRootMotionConfig(AnimatorHandle animator) const {
    if (!impl_) return {};

    auto it = impl_->animators_.find(animator);
    return it != impl_->animators_.end() ? it->second.rootMotionConfig : RootMotionConfig{};
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
    return it != impl_->animators_.end() ? it->second.currentRootMotion : RootMotion{};
}

RootMotion AnimationSystem::extractRootMotion(AnimationClipHandle clip, float fromTime, float toTime) const {
    RootMotion motion;
    // TODO: Sample the animation at both times and compute delta
    return motion;
}

void AnimationSystem::consumeRootMotion(AnimatorHandle animator) {
    if (!impl_) return;

    auto it = impl_->animators_.find(animator);
    if (it != impl_->animators_.end()) {
        it->second.currentRootMotion.deltaPosition = Vec3{0.0f};
        it->second.currentRootMotion.deltaRotation = Quat{1.0f, 0.0f, 0.0f, 0.0f};
    }
}

//==========================================================================
// Physics Integration: Ragdoll
//==========================================================================

Result<void, AnimationError> AnimationSystem::createRagdoll(Entity entity, const RagdollDef& def, IPhysics3DSystem& physics) {
    if (!impl_) return std::unexpected(AnimationError::PhysicsSystemRequired);

    if (impl_->ragdolls_.contains(entity)) {
        return std::unexpected(AnimationError::RagdollAlreadyExists);
    }

    auto skelIt = impl_->skeletons_.find(def.skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        return std::unexpected(AnimationError::SkeletonNotFound);
    }

    RagdollData data;
    data.entity = entity;
    data.skeleton = def.skeleton;
    data.definition = def;
    data.state.created = true;
    data.state.active = false;
    data.state.blendWeight = 0.0f;

    // Create physics bodies for each bone
    for (const auto& boneDef : def.bones) {
        // TODO: Create actual physics bodies via physics system
        data.state.boneIndices.push_back(boneDef.boneIndex);
    }

    impl_->ragdolls_[entity] = std::move(data);
    impl_->stats_.ragdollCount++;

    return {};
}

void AnimationSystem::destroyRagdoll(Entity entity, IPhysics3DSystem& physics) {
    if (!impl_) return;

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return;

    // Destroy physics bodies
    for (auto bodyEntity : it->second.state.boneEntities) {
        physics.destroyBody(bodyEntity);
    }

    impl_->ragdolls_.erase(it);
    impl_->stats_.ragdollCount--;
}

bool AnimationSystem::hasRagdoll(Entity entity) const {
    if (!impl_) return false;
    return impl_->ragdolls_.contains(entity);
}

RagdollState AnimationSystem::getRagdollState(Entity entity) const {
    if (!impl_) return {};

    auto it = impl_->ragdolls_.find(entity);
    return it != impl_->ragdolls_.end() ? it->second.state : RagdollState{};
}

void AnimationSystem::activateRagdoll(Entity entity, IPhysics3DSystem& physics, bool instant, float blendDuration) {
    if (!impl_) return;

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return;

    it->second.state.active = true;
    if (instant) {
        it->second.state.blendWeight = 1.0f;
        it->second.state.blendTime = 0.0f;
    } else {
        it->second.state.blendTarget = 1.0f;
        it->second.state.blendDuration = blendDuration;
        it->second.state.blendTime = 0.0f;
    }
}

void AnimationSystem::deactivateRagdoll(Entity entity, IPhysics3DSystem& physics, bool instant, float blendDuration) {
    if (!impl_) return;

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return;

    if (instant) {
        it->second.state.active = false;
        it->second.state.blendWeight = 0.0f;
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

std::vector<Mat4> AnimationSystem::getRagdollBoneTransforms(Entity entity, const IPhysics3DSystem& physics) const {
    std::vector<Mat4> transforms;
    if (!impl_) return transforms;

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return transforms;

    // Get transforms from physics bodies
    for (auto bodyEntity : it->second.state.boneEntities) {
        auto transform = physics.getTransform(bodyEntity);
        if (transform.has_value()) {
            transforms.push_back(composeTransform(
                transform->position,
                transform->rotation,
                Vec3{1.0f}));
        } else {
            transforms.push_back(Mat4{1.0f});
        }
    }

    return transforms;
}

//==========================================================================
// Physics Integration: Bone Sync & Impulses
//==========================================================================

void AnimationSystem::syncToPhysics(AnimatorHandle animator, Entity entity, IPhysics3DSystem& physics) {
    if (!impl_) return;

    auto animIt = impl_->animators_.find(animator);
    if (animIt == impl_->animators_.end()) return;

    auto ragIt = impl_->ragdolls_.find(entity);
    if (ragIt == impl_->ragdolls_.end()) return;

    impl_->stats_.ragdollSyncs++;

    // TODO: Sync animation transforms to kinematic bodies
}

void AnimationSystem::applyBoneImpulse(Entity entity, std::uint32_t boneIndex, const Vec3& impulse, IPhysics3DSystem& physics) {
    if (!impl_) return;

    auto it = impl_->ragdolls_.find(entity);
    if (it == impl_->ragdolls_.end()) return;

    // Find body for this bone and apply impulse
    for (std::size_t i = 0; i < it->second.state.boneIndices.size(); ++i) {
        if (it->second.state.boneIndices[i] == boneIndex && i < it->second.state.boneEntities.size()) {
            physics.applyImpulse(it->second.state.boneEntities[i], impulse);
            return;
        }
    }
}

void AnimationSystem::applyBoneImpulse(Entity entity, std::string_view boneName, const Vec3& impulse, IPhysics3DSystem& physics) {
    if (!impl_) return;

    auto ragIt = impl_->ragdolls_.find(entity);
    if (ragIt == impl_->ragdolls_.end()) return;

    auto skelIt = impl_->skeletons_.find(ragIt->second.skeleton);
    if (skelIt == impl_->skeletons_.end()) return;

    auto nameIt = skelIt->second.boneNameToIndex.find(std::string(boneName));
    if (nameIt != skelIt->second.boneNameToIndex.end()) {
        applyBoneImpulse(entity, static_cast<std::uint32_t>(nameIt->second), impulse, physics);
    }
}

void AnimationSystem::applyImpulseAtPosition(Entity entity, const Vec3& worldPosition, const Vec3& impulse, float radius, IPhysics3DSystem& physics) {
    if (!impl_) return;

    auto ragIt = impl_->ragdolls_.find(entity);
    if (ragIt == impl_->ragdolls_.end()) return;

    // Apply impulse to all bodies within radius
    for (auto bodyEntity : ragIt->second.state.boneEntities) {
        auto transform = physics.getTransform(bodyEntity);
        if (transform.has_value()) {
            float distance = glm::length(transform->position - worldPosition);
            if (distance < radius) {
                float falloff = 1.0f - (distance / radius);
                physics.applyImpulse(bodyEntity, impulse * falloff);
            }
        }
    }
}

//==========================================================================
// Event Subscriptions
//==========================================================================

SubscriptionId AnimationSystem::subscribeToEvents(AnimatorHandle animator, AnimationEventCallback callback) {
    if (!impl_) return 0;

    EventSubscription sub;
    sub.id = impl_->nextSubscriptionId_++;
    sub.animator = animator;
    sub.type = EventSubscription::Type::Event;
    sub.callback = std::move(callback);

    impl_->subscriptions_.push_back(std::move(sub));
    return sub.id;
}

SubscriptionId AnimationSystem::subscribeToComplete(AnimatorHandle animator, AnimationCompleteCallback callback) {
    if (!impl_) return 0;

    EventSubscription sub;
    sub.id = impl_->nextSubscriptionId_++;
    sub.animator = animator;
    sub.type = EventSubscription::Type::Complete;
    sub.callback = std::move(callback);

    impl_->subscriptions_.push_back(std::move(sub));
    return sub.id;
}

SubscriptionId AnimationSystem::subscribeToLayerChanges(AnimatorHandle animator, AnimationLayerCallback callback) {
    if (!impl_) return 0;

    EventSubscription sub;
    sub.id = impl_->nextSubscriptionId_++;
    sub.animator = animator;
    sub.type = EventSubscription::Type::LayerChange;
    sub.callback = std::move(callback);

    impl_->subscriptions_.push_back(std::move(sub));
    return sub.id;
}

void AnimationSystem::unsubscribe(SubscriptionId id) {
    if (!impl_) return;

    impl_->subscriptions_.erase(
        std::remove_if(impl_->subscriptions_.begin(), impl_->subscriptions_.end(),
            [id](const EventSubscription& sub) { return sub.id == id; }),
        impl_->subscriptions_.end());
}

//==========================================================================
// Stateless Animation Sampling
//==========================================================================

Result<std::vector<Mat4>, AnimationError> AnimationSystem::sampleAnimation(
    AnimationClipHandle clip,
    float time,
    AnimationWrapMode wrapMode) {

    if (!impl_) return std::unexpected(AnimationError::InvalidClip);

    auto clipIt = impl_->clips_.find(clip);
    if (clipIt == impl_->clips_.end()) {
        return std::unexpected(AnimationError::ClipNotFound);
    }

    auto skelIt = impl_->skeletons_.find(clipIt->second.skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        return std::unexpected(AnimationError::SkeletonNotFound);
    }

    const auto& clipData = clipIt->second;
    const auto& skeleton = skelIt->second;
    const std::size_t boneCount = skeleton.bones.size();

    float wrappedTime = wrapTime(time, clipData.duration, wrapMode);

    std::vector<Mat4> transforms(boneCount, Mat4{1.0f});

    // Use ozz for sampling
    ozz::animation::SamplingJob::Context context;
    context.Resize(clipData.ozzAnimation.num_tracks());

    ozz::vector<ozz::math::SoaTransform> locals(skeleton.ozzSkeleton.num_soa_joints());

    ozz::animation::SamplingJob samplingJob;
    samplingJob.animation = &clipData.ozzAnimation;
    samplingJob.context = &context;
    samplingJob.ratio = clipData.duration > 0 ? wrappedTime / clipData.duration : 0.0f;
    samplingJob.output = ozz::make_span(locals);

    if (samplingJob.Run()) {
        ozz::vector<ozz::math::Float4x4> models(skeleton.ozzSkeleton.num_joints());

        ozz::animation::LocalToModelJob ltmJob;
        ltmJob.skeleton = &skeleton.ozzSkeleton;
        ltmJob.input = ozz::make_span(locals);
        ltmJob.output = ozz::make_span(models);

        if (ltmJob.Run()) {
            for (std::size_t i = 0; i < boneCount; ++i) {
                transforms[i] = fromOzz(models[i]);
            }
        }
    }

    return transforms;
}

Result<std::vector<Mat4>, AnimationError> AnimationSystem::blendAnimations(
    SkeletonHandle skeleton,
    const AnimationBlendConfig& config) {

    if (!impl_) return std::unexpected(AnimationError::InvalidSkeleton);

    auto skelIt = impl_->skeletons_.find(skeleton);
    if (skelIt == impl_->skeletons_.end()) {
        return std::unexpected(AnimationError::SkeletonNotFound);
    }

    if (config.layers.empty()) {
        return std::unexpected(AnimationError::NoAnimationData);
    }

    const std::size_t boneCount = skelIt->second.bones.size();
    std::vector<Mat4> result(boneCount, Mat4{1.0f});

    // Sample and blend each layer
    float totalWeight = 0.0f;
    for (const auto& layer : config.layers) {
        auto sampledResult = sampleAnimation(layer.clip, layer.time, layer.wrapMode);
        if (!sampledResult.has_value()) continue;

        float effectiveWeight = layer.weight * config.masterWeight;
        totalWeight += effectiveWeight;

        const auto& sampled = sampledResult.value();
        for (std::size_t i = 0; i < boneCount; ++i) {
            Vec3 pos1, pos2, scale1, scale2;
            Quat rot1, rot2;
            decomposeTransform(result[i], pos1, rot1, scale1);
            decomposeTransform(sampled[i], pos2, rot2, scale2);

            float normalizedWeight = totalWeight > 0 ? effectiveWeight / totalWeight : 0;
            Vec3 blendPos = lerp(pos1, pos2, normalizedWeight);
            Quat blendRot = slerp(rot1, rot2, normalizedWeight);
            Vec3 blendScale = lerp(scale1, scale2, normalizedWeight);

            result[i] = composeTransform(blendPos, blendRot, blendScale);
        }
    }

    return result;
}

//==========================================================================
// Statistics & Debugging
//==========================================================================

AnimationStats AnimationSystem::getStats() const {
    if (!impl_) return {};

    auto stats = impl_->stats_;
    stats.skeletonCount = static_cast<std::uint32_t>(impl_->skeletons_.size());
    stats.clipCount = static_cast<std::uint32_t>(impl_->clips_.size());
    stats.animatorCount = static_cast<std::uint32_t>(impl_->animators_.size());
    stats.socketDefCount = static_cast<std::uint32_t>(impl_->sockets_.size());
    stats.ragdollCount = static_cast<std::uint32_t>(impl_->ragdolls_.size());

    // Estimate memory usage
    stats.skeletonMemoryBytes = 0;
    for (const auto& [handle, skel] : impl_->skeletons_) {
        stats.skeletonMemoryBytes += skel.bones.size() * sizeof(BoneInfo);
    }

    stats.clipMemoryBytes = 0;
    for (const auto& [handle, clip] : impl_->clips_) {
        for (const auto& channel : clip.channels) {
            stats.clipMemoryBytes += channel.keyframes.size() * sizeof(AnimationKeyframe);
        }
    }

    stats.animatorMemoryBytes = 0;
    for (const auto& [handle, anim] : impl_->animators_) {
        stats.animatorMemoryBytes += anim.boneTransforms.size() * sizeof(Mat4);
        stats.animatorMemoryBytes += anim.localTransforms.size() * sizeof(Mat4);
    }

    stats.totalMemoryBytes = stats.skeletonMemoryBytes + stats.clipMemoryBytes + stats.animatorMemoryBytes;

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
    return impl_ ? impl_->debugVisualization_ : false;
}

}  // namespace bestow
