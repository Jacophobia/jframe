// bestow-luabind/src/docs/animation_binding_doc.cpp
// API documentation for bestow.animation

module bestow.luabind;

import std;

namespace bestow {

void registerAnimationDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "animation";
    sys.qualifiedName = "bestow.animation";
    sys.description = "Skeletal animation system. Manages skeletons, clips, animators, layered blending, IK, sockets, root motion, and ragdolls.";

    // --- Enums ---

    sys.enums.push_back(EnumDoc{
        .name = "AnimationError",
        .qualifiedName = "AnimationError",
        .description = "Error codes returned by animation operations.",
        .values = {
            {"Success", "Operation completed successfully"},
            {"InvalidSkeleton", "The skeleton handle is invalid"},
            {"SkeletonNotFound", "Skeleton not found in registry"},
            {"InvalidBoneIndex", "Bone index out of range"},
            {"BoneNotFound", "Bone name not found in skeleton"},
            {"EmptyBoneData", "Bone data is empty"},
            {"InvalidClip", "The animation clip handle is invalid"},
            {"ClipNotFound", "Animation clip not found"},
            {"IncompatibleSkeleton", "Clip skeleton does not match animator skeleton"},
            {"NoAnimationData", "No animation data in the source"},
            {"DuplicateClipName", "A clip with this name already exists"},
            {"InvalidAnimator", "The animator handle is invalid"},
            {"AnimatorNotFound", "Animator not found in registry"},
            {"AnimatorSkeletonMismatch", "Animator and skeleton do not match"},
            {"InvalidSocket", "The socket handle is invalid"},
            {"SocketNotFound", "Socket not found"},
            {"SocketAlreadyExists", "A socket with this name already exists"},
            {"SocketBoneNotFound", "The bone referenced by the socket was not found"},
            {"IKChainNotFound", "IK chain not found"},
            {"IKChainInvalid", "IK chain definition is invalid"},
            {"IKSolveFailed", "IK solve did not converge"},
            {"SamplingFailed", "Animation sampling failed"},
            {"BlendingFailed", "Animation blending failed"},
            {"InvalidTimeRange", "Time value is out of valid range"},
            {"InvalidWeight", "Weight value is out of valid range"},
            {"PhysicsSystemRequired", "Physics system required for ragdoll"},
            {"RagdollCreationFailed", "Failed to create ragdoll"},
            {"RagdollNotFound", "Ragdoll not found for entity"},
            {"RagdollAlreadyExists", "Entity already has a ragdoll"},
            {"RagdollInactive", "Ragdoll is not active"},
            {"AssetLoadFailed", "Failed to load asset"},
            {"InvalidModelData", "Model data is invalid or corrupt"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "AnimationWrapMode",
        .qualifiedName = "AnimationWrapMode",
        .description = "How an animation clip behaves when it reaches its end.",
        .values = {
            {"Once", "Play once and stop"},
            {"Loop", "Loop continuously"},
            {"PingPong", "Play forward then backward repeatedly"},
            {"ClampForever", "Play once and hold the last frame"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "AnimationBlendMode",
        .qualifiedName = "AnimationBlendMode",
        .description = "How animation layers are blended together.",
        .values = {
            {"Override", "Replace lower layers"},
            {"Additive", "Add to lower layers"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "SocketAttachMode",
        .qualifiedName = "SocketAttachMode",
        .description = "How a socket attachment follows its parent bone.",
        .values = {
            {"FollowBone", "Follow bone position and rotation"},
            {"FollowPosition", "Follow bone position only"},
            {"FollowRotation", "Follow bone rotation only"},
            {"WorldSpace", "Stay in world space (no following)"},
        }
    });

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "BoneInfo",
        .qualifiedName = "BoneInfo",
        .description = "Information about a single bone in a skeleton.",
        .fields = {
            {"name", "string", "Bone name"},
            {"index", "number", "Bone index in the skeleton"},
            {"parentIndex", "number", "Parent bone index (-1 for root)"},
            {"inverseBindPose", "Mat4", "Inverse bind pose matrix"},
            {"localBindPose", "Mat4", "Local bind pose matrix"},
            {"localPosition", "Vec3", "Local position in bind pose"},
            {"localRotation", "Quat", "Local rotation in bind pose"},
            {"localScale", "Vec3", "Local scale in bind pose"},
            {"isRoot", "boolean", "Whether this is the root bone"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "SkeletonInfo",
        .qualifiedName = "SkeletonInfo",
        .description = "Information about a skeleton hierarchy.",
        .fields = {
            {"boneCount", "number", "Total number of bones"},
            {"bones", "BoneInfo[]", "Array of bone information"},
            {"rootBoneIndex", "number", "Index of the root bone"},
            {"bounds", "AABB3D", "Bounding box of the skeleton in bind pose"},
        },
        .methods = {
            {.name = "findBone", .qualifiedName = "SkeletonInfo:findBone",
             .description = "Find a bone by name.",
             .params = {{.name = "name", .type = "string", .description = "Bone name to find"}},
             .returns = {{.type = "number|nil", .description = "Bone index, or nil if not found"}}},
            {.name = "getChildren", .qualifiedName = "SkeletonInfo:getChildren",
             .description = "Get child bone indices for a given bone.",
             .params = {{.name = "boneIndex", .type = "number", .description = "Parent bone index"}},
             .returns = {{.type = "number[]", .description = "Array of child bone indices"}}},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "AnimationClipInfo",
        .qualifiedName = "AnimationClipInfo",
        .description = "Information about an animation clip.",
        .fields = {
            {"name", "string", "Clip name"},
            {"skeleton", "SkeletonHandle", "Associated skeleton"},
            {"duration", "number", "Duration in seconds"},
            {"ticksPerSecond", "number", "Animation ticks per second"},
            {"defaultWrapMode", "AnimationWrapMode", "Default wrap mode"},
            {"channelCount", "number", "Number of animated channels"},
            {"keyframeCount", "number", "Total number of keyframes"},
            {"hasRootMotion", "boolean", "Whether the clip has root motion data"},
            {"looping", "boolean", "Whether the clip loops by default"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "AnimationEventDef",
        .qualifiedName = "AnimationEventDef",
        .description = "Definition of an animation event at a specific time in a clip.",
        .fields = {
            {"name", "string", "Event name"},
            {"time", "number", "Time in seconds when the event fires"},
            {"stringParam", "string", "Optional string parameter"},
            {"floatParam", "number", "Optional float parameter"},
            {"intParam", "number", "Optional integer parameter"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "AnimationEvent",
        .qualifiedName = "AnimationEvent",
        .description = "A dispatched animation event with runtime context.",
        .fields = {
            {"animator", "AnimatorHandle", "Animator that triggered the event"},
            {"clip", "AnimationClipHandle", "Clip that contains the event"},
            {"layer", "number", "Animation layer the event was on"},
            {"name", "string", "Event name"},
            {"clipTime", "number", "Time in the clip when the event fired"},
            {"normalizedTime", "number", "Normalized time (0-1) when the event fired"},
            {"stringParam", "string", "String parameter"},
            {"floatParam", "number", "Float parameter"},
            {"intParam", "number", "Integer parameter"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "SocketDef",
        .qualifiedName = "SocketDef",
        .description = "Definition of an attachment socket on a skeleton bone.",
        .fields = {
            {"name", "string", "Socket name"},
            {"boneName", "string", "Name of the bone to attach to"},
            {"localPosition", "Vec3", "Local position offset from bone"},
            {"localRotation", "Quat", "Local rotation offset from bone"},
            {"localScale", "Vec3", "Local scale"},
            {"attachMode", "SocketAttachMode", "How the socket follows the bone"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "SocketTransform",
        .qualifiedName = "SocketTransform",
        .description = "World-space transform of a socket at a given frame.",
        .fields = {
            {"worldMatrix", "Mat4", "Full world transform matrix"},
            {"position", "Vec3", "World position"},
            {"rotation", "Quat", "World rotation"},
            {"scale", "Vec3", "World scale"},
            {"forward", "Vec3", "Forward direction vector"},
            {"up", "Vec3", "Up direction vector"},
            {"right", "Vec3", "Right direction vector"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "AnimationLayerState",
        .qualifiedName = "AnimationLayerState",
        .description = "Current state of a single animation layer on an animator.",
        .fields = {
            {"clip", "AnimationClipHandle", "Currently playing clip"},
            {"clipName", "string", "Name of the currently playing clip"},
            {"time", "number", "Current time in seconds"},
            {"normalizedTime", "number", "Current time normalized (0-1)"},
            {"speed", "number", "Playback speed multiplier"},
            {"weight", "number", "Layer weight (0-1)"},
            {"fadeWeight", "number", "Current crossfade weight"},
            {"wrapMode", "AnimationWrapMode", "Current wrap mode"},
            {"blendMode", "AnimationBlendMode", "Layer blend mode"},
            {"playing", "boolean", "Whether the layer is playing"},
            {"paused", "boolean", "Whether the layer is paused"},
            {"finished", "boolean", "Whether playback has finished (non-looping)"},
            {"effectiveWeight", "number", "Final combined weight"},
            {"isActive", "boolean", "Whether the layer has active content"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "AnimationPlayConfig",
        .qualifiedName = "AnimationPlayConfig",
        .description = "Configuration for playing an animation with full control.",
        .fields = {
            {"clip", "AnimationClipHandle", "Clip handle to play"},
            {"clipName", "string", "Clip name (alternative to handle)"},
            {"startTime", "number", "Start time in seconds"},
            {"speed", "number", "Playback speed multiplier"},
            {"weight", "number", "Layer weight (0-1)"},
            {"blendInTime", "number", "Crossfade blend-in duration"},
            {"blendOutTime", "number", "Crossfade blend-out duration"},
            {"wrapMode", "AnimationWrapMode", "Wrap mode"},
            {"layer", "number", "Animation layer index"},
            {"blendMode", "AnimationBlendMode", "Blend mode for this layer"},
            {"restartIfSame", "boolean", "Restart if the same clip is already playing"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "IKTwoBoneChain",
        .qualifiedName = "IKTwoBoneChain",
        .description = "Definition of a two-bone IK chain (e.g., arm or leg).",
        .fields = {
            {"name", "string", "Chain name (e.g., 'leftArm', 'rightLeg')"},
            {"rootBoneName", "string", "Name of the root bone (e.g., upper arm/thigh)"},
            {"midBoneName", "string", "Name of the middle bone (e.g., forearm/shin)"},
            {"tipBoneName", "string", "Name of the tip bone (e.g., hand/foot)"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "IKTwoBoneTarget",
        .qualifiedName = "IKTwoBoneTarget",
        .description = "Runtime target for a two-bone IK chain.",
        .fields = {
            {"chainName", "string", "Name of the IK chain to solve"},
            {"targetPosition", "Vec3", "World-space target position"},
            {"poleVector", "Vec3", "Pole vector for elbow/knee direction"},
            {"weight", "number", "IK weight (0=animation, 1=full IK)"},
            {"enabled", "boolean", "Whether this IK target is active"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "IKAimConfig",
        .qualifiedName = "IKAimConfig",
        .description = "Configuration for an aim IK constraint (e.g., head look-at).",
        .fields = {
            {"name", "string", "Aim constraint name"},
            {"boneName", "string", "Name of the bone to rotate (e.g., 'Head')"},
            {"aimAxis", "Vec3", "Local axis to aim toward target"},
            {"upAxis", "Vec3", "Local up axis"},
            {"horizontalLimit", "number", "Maximum horizontal rotation in radians"},
            {"verticalLimit", "number", "Maximum vertical rotation in radians"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "IKAimTarget",
        .qualifiedName = "IKAimTarget",
        .description = "Runtime target for an aim IK constraint.",
        .fields = {
            {"configName", "string", "Name of the aim constraint"},
            {"targetPosition", "Vec3", "World-space position to look at"},
            {"worldUp", "Vec3", "World up vector"},
            {"weight", "number", "IK weight (0=animation, 1=full IK)"},
            {"enabled", "boolean", "Whether this aim target is active"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "RootMotion",
        .qualifiedName = "RootMotion",
        .description = "Root motion data extracted from an animation.",
        .fields = {
            {"deltaPosition", "Vec3", "Position change this frame"},
            {"deltaRotation", "Quat", "Rotation change this frame"},
            {"totalPosition", "Vec3", "Total accumulated position"},
            {"totalRotation", "Quat", "Total accumulated rotation"},
            {"hasTranslation", "boolean", "Whether the clip has root translation"},
            {"hasRotation", "boolean", "Whether the clip has root rotation"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "RootMotionConfig",
        .qualifiedName = "RootMotionConfig",
        .description = "Configuration for root motion extraction.",
        .fields = {
            {"enabled", "boolean", "Whether root motion is enabled"},
            {"extractTranslationX", "boolean", "Extract X translation"},
            {"extractTranslationY", "boolean", "Extract Y translation"},
            {"extractTranslationZ", "boolean", "Extract Z translation"},
            {"extractRotationY", "boolean", "Extract Y-axis rotation"},
            {"extractRotationXZ", "boolean", "Extract X/Z rotation"},
            {"rootBoneName", "string", "Name of the root bone"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "RagdollState",
        .qualifiedName = "RagdollState",
        .description = "Current state of a ragdoll.",
        .fields = {
            {"created", "boolean", "Whether the ragdoll has been created"},
            {"active", "boolean", "Whether the ragdoll is currently simulating"},
            {"blendWeight", "number", "Current blend weight (0=animation, 1=ragdoll)"},
            {"blendTarget", "number", "Target blend weight"},
            {"blendDuration", "number", "Blend transition duration"},
            {"blendTime", "number", "Current blend transition time"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "AnimationStats",
        .qualifiedName = "AnimationStats",
        .description = "Animation system performance statistics.",
        .fields = {
            {"skeletonCount", "number", "Number of active skeletons"},
            {"clipCount", "number", "Number of loaded clips"},
            {"animatorCount", "number", "Number of active animators"},
            {"socketDefCount", "number", "Number of defined sockets"},
            {"ikChainCount", "number", "Number of IK chains"},
            {"ragdollCount", "number", "Number of ragdolls"},
            {"animatorsUpdated", "number", "Animators updated this frame"},
            {"layersProcessed", "number", "Animation layers processed this frame"},
            {"samplingJobs", "number", "Sampling jobs this frame"},
            {"blendingJobs", "number", "Blending jobs this frame"},
            {"ikSolves", "number", "IK solves this frame"},
            {"socketQueries", "number", "Socket queries this frame"},
            {"eventsDispatched", "number", "Events dispatched this frame"},
            {"ragdollSyncs", "number", "Ragdoll syncs this frame"},
            {"updateTimeMs", "number", "Total update time in ms"},
            {"samplingTimeMs", "number", "Sampling time in ms"},
            {"blendingTimeMs", "number", "Blending time in ms"},
            {"ikTimeMs", "number", "IK time in ms"},
            {"ragdollSyncTimeMs", "number", "Ragdoll sync time in ms"},
            {"skeletonMemoryBytes", "number", "Memory used by skeletons"},
            {"clipMemoryBytes", "number", "Memory used by clips"},
            {"animatorMemoryBytes", "number", "Memory used by animators"},
            {"totalMemoryBytes", "number", "Total animation memory usage"},
        },
    });

    // --- Methods: Skeleton Creation ---

    sys.methods.push_back(MethodDoc{
        .name = "createSkeletonFromModel",
        .qualifiedName = "bestow.animation.createSkeletonFromModel",
        .description = "Create a skeleton from a loaded model asset.",
        .params = {
            {.name = "modelHandle", .type = "AssetHandle", .description = "Handle to a loaded model with skeleton data"},
        },
        .returns = {{.type = "SkeletonHandle|nil", .description = "Skeleton handle, or nil on failure"}},
        .seeAlso = {"bestow.assets.loadModel"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "loadClipsFromModel",
        .qualifiedName = "bestow.animation.loadClipsFromModel",
        .description = "Load all animation clips from a model into an existing skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Target skeleton"},
            {.name = "modelHandle", .type = "AssetHandle", .description = "Handle to a loaded model with animation data"},
        },
        .returns = {{.type = "table|nil", .description = "Table mapping clip names to handles (e.g., {idle=h1, walk=h2}), or nil on failure"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "loadClip",
        .qualifiedName = "bestow.animation.loadClip",
        .description = "Load a single animation clip from a file. If clipName is nil, uses the first animation found.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Target skeleton"},
            {.name = "path", .type = "string", .description = "Path to animation file (e.g., ':library:/animations/idle.fbx')"},
            {.name = "clipName", .type = "string", .description = "Name of the specific clip to load", .optional = true},
        },
        .returns = {{.type = "AnimationClipHandle|nil", .description = "Clip handle, or nil on failure"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "loadCharacter",
        .qualifiedName = "bestow.animation.loadCharacter",
        .description = "High-level: Load a complete character with skeleton, animator, mesh, and clips from a model file.",
        .params = {
            {.name = "path", .type = "string", .description = "Path to character model file"},
        },
        .returns = {{.type = "table|nil", .description = "Table with fields: skeleton, animator, clips, modelHandle, mesh, material. Returns nil on failure."}},
        .example = "local char = bestow.animation.loadCharacter(\":library:/characters/hero.fbx\")\nif char then\n    bestow.animation.play(char.animator, char.clips.idle)\nend",
    });

    // --- Methods: Character Rendering ---

    sys.methods.push_back(MethodDoc{
        .name = "drawCharacter",
        .qualifiedName = "bestow.animation.drawCharacter",
        .description = "Draw an animated character with skinned mesh rendering.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator handle"},
            {.name = "mesh", .type = "MeshHandle", .description = "Mesh handle"},
            {.name = "material", .type = "MaterialHandle", .description = "Material handle"},
            {.name = "worldMatrix", .type = "Mat4", .description = "World transform matrix"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "drawCharacterTransform",
        .qualifiedName = "bestow.animation.drawCharacterTransform",
        .description = "Draw an animated character using a Transform3D instead of a Mat4.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator handle"},
            {.name = "mesh", .type = "MeshHandle", .description = "Mesh handle"},
            {.name = "material", .type = "MaterialHandle", .description = "Material handle"},
            {.name = "transform", .type = "Transform3D", .description = "World transform"},
        },
    });

    // --- Methods: Skeleton Management ---

    sys.methods.push_back(MethodDoc{
        .name = "destroySkeleton",
        .qualifiedName = "bestow.animation.destroySkeleton",
        .description = "Destroy a skeleton and free its resources.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to destroy"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isValidSkeleton",
        .qualifiedName = "bestow.animation.isValidSkeleton",
        .description = "Check if a skeleton handle is valid.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to check"},
        },
        .returns = {{.type = "boolean", .description = "true if valid"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getSkeletonInfo",
        .qualifiedName = "bestow.animation.getSkeletonInfo",
        .description = "Get detailed information about a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to query"},
        },
        .returns = {{.type = "SkeletonInfo", .description = "Skeleton information"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "findBoneIndex",
        .qualifiedName = "bestow.animation.findBoneIndex",
        .description = "Find a bone index by name in a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to search"},
            {.name = "boneName", .type = "string", .description = "Bone name to find"},
        },
        .returns = {{.type = "number", .description = "Bone index, or -1 if not found"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBoneNames",
        .qualifiedName = "bestow.animation.getBoneNames",
        .description = "Get all bone names in a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to query"},
        },
        .returns = {{.type = "string[]", .description = "Array of bone names"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBoneCount",
        .qualifiedName = "bestow.animation.getBoneCount",
        .description = "Get the number of bones in a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to query"},
        },
        .returns = {{.type = "number", .description = "Number of bones"}},
    });

    // --- Methods: Clip Management ---

    sys.methods.push_back(MethodDoc{
        .name = "destroyAnimationClip",
        .qualifiedName = "bestow.animation.destroyAnimationClip",
        .description = "Destroy an animation clip.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Clip to destroy"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isValidClip",
        .qualifiedName = "bestow.animation.isValidClip",
        .description = "Check if an animation clip handle is valid.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Clip to check"},
        },
        .returns = {{.type = "boolean", .description = "true if valid"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAnimationClipInfo",
        .qualifiedName = "bestow.animation.getAnimationClipInfo",
        .description = "Get information about an animation clip.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Clip to query"},
        },
        .returns = {{.type = "AnimationClipInfo", .description = "Clip information"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "findClip",
        .qualifiedName = "bestow.animation.findClip",
        .description = "Find a clip handle by name within a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to search"},
            {.name = "clipName", .type = "string", .description = "Clip name to find"},
        },
        .returns = {{.type = "AnimationClipHandle", .description = "Clip handle (0 if not found)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getClipsForSkeleton",
        .qualifiedName = "bestow.animation.getClipsForSkeleton",
        .description = "Get all clip handles associated with a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to query"},
        },
        .returns = {{.type = "AnimationClipHandle[]", .description = "Array of clip handles"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getClipNames",
        .qualifiedName = "bestow.animation.getClipNames",
        .description = "Get all clip names associated with a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to query"},
        },
        .returns = {{.type = "string[]", .description = "Array of clip names"}},
    });

    // --- Methods: Animation Events ---

    sys.methods.push_back(MethodDoc{
        .name = "addClipEvent",
        .qualifiedName = "bestow.animation.addClipEvent",
        .description = "Add a timed event to an animation clip.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Clip to add the event to"},
            {.name = "event", .type = "AnimationEventDef", .description = "Event definition"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeClipEvent",
        .qualifiedName = "bestow.animation.removeClipEvent",
        .description = "Remove a named event from an animation clip.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Clip to remove the event from"},
            {.name = "eventName", .type = "string", .description = "Name of the event to remove"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearClipEvents",
        .qualifiedName = "bestow.animation.clearClipEvents",
        .description = "Remove all events from an animation clip.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Clip to clear events from"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getClipEvents",
        .qualifiedName = "bestow.animation.getClipEvents",
        .description = "Get all events defined on an animation clip.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Clip to query"},
        },
        .returns = {{.type = "AnimationEventDef[]", .description = "Array of event definitions"}},
    });

    // --- Methods: Animator Management ---

    sys.methods.push_back(MethodDoc{
        .name = "createAnimator",
        .qualifiedName = "bestow.animation.createAnimator",
        .description = "Create a new animator for a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to animate"},
        },
        .returns = {{.type = "AnimatorHandle|nil", .description = "Animator handle, or nil on failure"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "destroyAnimator",
        .qualifiedName = "bestow.animation.destroyAnimator",
        .description = "Destroy an animator.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to destroy"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isValidAnimator",
        .qualifiedName = "bestow.animation.isValidAnimator",
        .description = "Check if an animator handle is valid.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to check"},
        },
        .returns = {{.type = "boolean", .description = "true if valid"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAnimatorSkeleton",
        .qualifiedName = "bestow.animation.getAnimatorSkeleton",
        .description = "Get the skeleton handle associated with an animator.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
        },
        .returns = {{.type = "SkeletonHandle", .description = "Associated skeleton handle"}},
    });

    // --- Methods: Playback Control ---

    sys.methods.push_back(MethodDoc{
        .name = "play",
        .qualifiedName = "bestow.animation.play",
        .description = "Play an animation on an animator. Accepts a clip handle, clip name, or AnimationPlayConfig for full control.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to play on"},
            {.name = "clipOrConfig", .type = "AnimationClipHandle|string|AnimationPlayConfig", .description = "Clip handle, clip name, or play config"},
            {.name = "transitionTime", .type = "number", .description = "Crossfade transition time in seconds", .optional = true, .defaultVal = "0.25"},
        },
        .example = "bestow.animation.play(animator, clips.idle)\nbestow.animation.play(animator, \"walk\", 0.3)",
    });

    sys.methods.push_back(MethodDoc{
        .name = "stop",
        .qualifiedName = "bestow.animation.stop",
        .description = "Stop all animation on an animator.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to stop"},
            {.name = "fadeOutTime", .type = "number", .description = "Fade-out duration", .optional = true, .defaultVal = "0"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "stopLayer",
        .qualifiedName = "bestow.animation.stopLayer",
        .description = "Stop animation on a specific layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator"},
            {.name = "layer", .type = "number", .description = "Layer index to stop"},
            {.name = "fadeOutTime", .type = "number", .description = "Fade-out duration", .optional = true, .defaultVal = "0"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setPaused",
        .qualifiedName = "bestow.animation.setPaused",
        .description = "Pause or unpause an animator.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to pause/unpause"},
            {.name = "paused", .type = "boolean", .description = "true to pause, false to resume"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isPaused",
        .qualifiedName = "bestow.animation.isPaused",
        .description = "Check if an animator is paused.",
        .params = {{.name = "animator", .type = "AnimatorHandle", .description = "Animator to check"}},
        .returns = {{.type = "boolean", .description = "true if paused"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setSpeed",
        .qualifiedName = "bestow.animation.setSpeed",
        .description = "Set the global playback speed of an animator.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to modify"},
            {.name = "speed", .type = "number", .description = "Speed multiplier (1.0 = normal)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getSpeed",
        .qualifiedName = "bestow.animation.getSpeed",
        .description = "Get the global playback speed of an animator.",
        .params = {{.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"}},
        .returns = {{.type = "number", .description = "Speed multiplier"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isPlaying",
        .qualifiedName = "bestow.animation.isPlaying",
        .description = "Check if an animator has any active animation.",
        .params = {{.name = "animator", .type = "AnimatorHandle", .description = "Animator to check"}},
        .returns = {{.type = "boolean", .description = "true if playing"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isLayerPlaying",
        .qualifiedName = "bestow.animation.isLayerPlaying",
        .description = "Check if a specific layer is playing.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to check"},
            {.name = "layer", .type = "number", .description = "Layer index"},
        },
        .returns = {{.type = "boolean", .description = "true if the layer is playing"}},
    });

    // --- Methods: Layer Control ---

    sys.methods.push_back(MethodDoc{
        .name = "getLayerState",
        .qualifiedName = "bestow.animation.getLayerState",
        .description = "Get the current state of an animation layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "layer", .type = "number", .description = "Layer index"},
        },
        .returns = {{.type = "AnimationLayerState", .description = "Layer state"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setLayerWeight",
        .qualifiedName = "bestow.animation.setLayerWeight",
        .description = "Set the weight of an animation layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to modify"},
            {.name = "layer", .type = "number", .description = "Layer index"},
            {.name = "weight", .type = "number", .description = "Weight (0-1)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getLayerWeight",
        .qualifiedName = "bestow.animation.getLayerWeight",
        .description = "Get the weight of an animation layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "layer", .type = "number", .description = "Layer index"},
        },
        .returns = {{.type = "number", .description = "Layer weight"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setLayerBlendMode",
        .qualifiedName = "bestow.animation.setLayerBlendMode",
        .description = "Set the blend mode of an animation layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to modify"},
            {.name = "layer", .type = "number", .description = "Layer index"},
            {.name = "mode", .type = "AnimationBlendMode", .description = "Blend mode"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getLayerCount",
        .qualifiedName = "bestow.animation.getLayerCount",
        .description = "Get the number of animation layers on an animator.",
        .params = {{.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"}},
        .returns = {{.type = "number", .description = "Number of layers"}},
    });

    // --- Methods: Time Control ---

    sys.methods.push_back(MethodDoc{
        .name = "getCurrentTime",
        .qualifiedName = "bestow.animation.getCurrentTime",
        .description = "Get the current playback time of a layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "layer", .type = "number", .description = "Layer index", .optional = true, .defaultVal = "0"},
        },
        .returns = {{.type = "number", .description = "Current time in seconds"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getNormalizedTime",
        .qualifiedName = "bestow.animation.getNormalizedTime",
        .description = "Get the normalized playback time (0-1) of a layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "layer", .type = "number", .description = "Layer index", .optional = true, .defaultVal = "0"},
        },
        .returns = {{.type = "number", .description = "Normalized time (0-1)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setCurrentTime",
        .qualifiedName = "bestow.animation.setCurrentTime",
        .description = "Set the playback time of a layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to modify"},
            {.name = "time", .type = "number", .description = "Time in seconds"},
            {.name = "layer", .type = "number", .description = "Layer index", .optional = true, .defaultVal = "0"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setNormalizedTime",
        .qualifiedName = "bestow.animation.setNormalizedTime",
        .description = "Set the normalized playback time (0-1) of a layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to modify"},
            {.name = "normalizedTime", .type = "number", .description = "Normalized time (0-1)"},
            {.name = "layer", .type = "number", .description = "Layer index", .optional = true, .defaultVal = "0"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getClipDuration",
        .qualifiedName = "bestow.animation.getClipDuration",
        .description = "Get the duration of the clip currently playing on a layer.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "layer", .type = "number", .description = "Layer index", .optional = true, .defaultVal = "0"},
        },
        .returns = {{.type = "number", .description = "Duration in seconds"}},
    });

    // --- Methods: Bone Transforms ---

    sys.methods.push_back(MethodDoc{
        .name = "getBoneTransform",
        .qualifiedName = "bestow.animation.getBoneTransform",
        .description = "Get the local transform of a bone. Accepts bone index or bone name.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "bone", .type = "number|string", .description = "Bone index or bone name"},
        },
        .returns = {{.type = "Mat4", .description = "Bone local transform matrix"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBoneWorldTransform",
        .qualifiedName = "bestow.animation.getBoneWorldTransform",
        .description = "Get the world transform of a bone.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "boneIndex", .type = "number", .description = "Bone index"},
            {.name = "entityWorldMatrix", .type = "Mat4", .description = "Entity world transform matrix"},
        },
        .returns = {{.type = "Mat4", .description = "Bone world transform matrix"}},
    });

    // --- Methods: Socket System ---

    sys.methods.push_back(MethodDoc{
        .name = "defineSocket",
        .qualifiedName = "bestow.animation.defineSocket",
        .description = "Define an attachment socket on a skeleton bone.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to add the socket to"},
            {.name = "def", .type = "SocketDef", .description = "Socket definition"},
        },
        .returns = {{.type = "SocketHandle|nil", .description = "Socket handle, or nil on failure"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeSocket",
        .qualifiedName = "bestow.animation.removeSocket",
        .description = "Remove a socket. Accepts a socket handle, or skeleton + socket name.",
        .params = {
            {.name = "socketOrSkeleton", .type = "SocketHandle|SkeletonHandle", .description = "Socket handle or skeleton handle"},
            {.name = "name", .type = "string", .description = "Socket name (when passing skeleton handle)", .optional = true},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasSocket",
        .qualifiedName = "bestow.animation.hasSocket",
        .description = "Check if a skeleton has a socket with the given name.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to check"},
            {.name = "socketName", .type = "string", .description = "Socket name"},
        },
        .returns = {{.type = "boolean", .description = "true if the socket exists"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "findSocket",
        .qualifiedName = "bestow.animation.findSocket",
        .description = "Find a socket handle by name.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to search"},
            {.name = "socketName", .type = "string", .description = "Socket name"},
        },
        .returns = {{.type = "SocketHandle", .description = "Socket handle (0 if not found)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getSockets",
        .qualifiedName = "bestow.animation.getSockets",
        .description = "Get all socket handles for a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to query"},
        },
        .returns = {{.type = "SocketHandle[]", .description = "Array of socket handles"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getSocketDef",
        .qualifiedName = "bestow.animation.getSocketDef",
        .description = "Get the definition of a socket.",
        .params = {
            {.name = "socket", .type = "SocketHandle", .description = "Socket to query"},
        },
        .returns = {{.type = "SocketDef|nil", .description = "Socket definition, or nil if invalid"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getSocketTransform",
        .qualifiedName = "bestow.animation.getSocketTransform",
        .description = "Get the world transform of a socket. Accepts socket name or handle.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator for the current pose"},
            {.name = "socketOrName", .type = "SocketHandle|string", .description = "Socket handle or socket name"},
            {.name = "entityWorldMatrix", .type = "Mat4", .description = "Entity world transform"},
        },
        .returns = {{.type = "SocketTransform|nil", .description = "Socket world transform, or nil on failure"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setSocketLocalTransform",
        .qualifiedName = "bestow.animation.setSocketLocalTransform",
        .description = "Update the local transform offset of a socket.",
        .params = {
            {.name = "socket", .type = "SocketHandle", .description = "Socket to modify"},
            {.name = "position", .type = "Vec3", .description = "New local position"},
            {.name = "rotation", .type = "Quat", .description = "New local rotation"},
            {.name = "scale", .type = "Vec3", .description = "New local scale", .optional = true, .defaultVal = "Vec3(1,1,1)"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setSocketEnabled",
        .qualifiedName = "bestow.animation.setSocketEnabled",
        .description = "Enable or disable a socket.",
        .params = {
            {.name = "socket", .type = "SocketHandle", .description = "Socket to modify"},
            {.name = "enabled", .type = "boolean", .description = "true to enable"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isSocketEnabled",
        .qualifiedName = "bestow.animation.isSocketEnabled",
        .description = "Check if a socket is enabled.",
        .params = {{.name = "socket", .type = "SocketHandle", .description = "Socket to check"}},
        .returns = {{.type = "boolean", .description = "true if enabled"}},
    });

    // --- Methods: IK Chain Definition ---

    sys.methods.push_back(MethodDoc{
        .name = "defineIKChain",
        .qualifiedName = "bestow.animation.defineIKChain",
        .description = "Define a two-bone IK chain on a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to add the chain to"},
            {.name = "chain", .type = "IKTwoBoneChain", .description = "Chain definition"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "defineIKAim",
        .qualifiedName = "bestow.animation.defineIKAim",
        .description = "Define an aim IK constraint on a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to add the constraint to"},
            {.name = "config", .type = "IKAimConfig", .description = "Aim constraint configuration"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeIKChain",
        .qualifiedName = "bestow.animation.removeIKChain",
        .description = "Remove an IK chain from a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton"},
            {.name = "name", .type = "string", .description = "Chain name"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeIKAim",
        .qualifiedName = "bestow.animation.removeIKAim",
        .description = "Remove an aim IK constraint from a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton"},
            {.name = "name", .type = "string", .description = "Aim constraint name"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getIKChainNames",
        .qualifiedName = "bestow.animation.getIKChainNames",
        .description = "Get all IK chain names for a skeleton.",
        .params = {{.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to query"}},
        .returns = {{.type = "string[]", .description = "Array of chain names"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getIKAimNames",
        .qualifiedName = "bestow.animation.getIKAimNames",
        .description = "Get all aim IK constraint names for a skeleton.",
        .params = {{.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton to query"}},
        .returns = {{.type = "string[]", .description = "Array of aim constraint names"}},
    });

    // --- Methods: IK Runtime Control ---

    sys.methods.push_back(MethodDoc{
        .name = "setIKTarget",
        .qualifiedName = "bestow.animation.setIKTarget",
        .description = "Set an IK target on an animator. Accepts either an IKTwoBoneTarget or IKAimTarget.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to modify"},
            {.name = "target", .type = "IKTwoBoneTarget|IKAimTarget", .description = "IK target configuration"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getIKTwoBoneTarget",
        .qualifiedName = "bestow.animation.getIKTwoBoneTarget",
        .description = "Get the current two-bone IK target for a chain.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "chainName", .type = "string", .description = "IK chain name"},
        },
        .returns = {{.type = "IKTwoBoneTarget|nil", .description = "Current target, or nil if not set"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getIKAimTarget",
        .qualifiedName = "bestow.animation.getIKAimTarget",
        .description = "Get the current aim IK target.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "configName", .type = "string", .description = "Aim constraint name"},
        },
        .returns = {{.type = "IKAimTarget|nil", .description = "Current target, or nil if not set"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearIKTarget",
        .qualifiedName = "bestow.animation.clearIKTarget",
        .description = "Clear an IK target by name.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to modify"},
            {.name = "targetName", .type = "string", .description = "Name of the IK target to clear"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearAllIKTargets",
        .qualifiedName = "bestow.animation.clearAllIKTargets",
        .description = "Clear all IK targets on an animator.",
        .params = {{.name = "animator", .type = "AnimatorHandle", .description = "Animator to clear"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setIKWeight",
        .qualifiedName = "bestow.animation.setIKWeight",
        .description = "Set the weight of an IK target.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to modify"},
            {.name = "targetName", .type = "string", .description = "IK target name"},
            {.name = "weight", .type = "number", .description = "Weight (0=animation, 1=full IK)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getIKWeight",
        .qualifiedName = "bestow.animation.getIKWeight",
        .description = "Get the weight of an IK target.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"},
            {.name = "targetName", .type = "string", .description = "IK target name"},
        },
        .returns = {{.type = "number", .description = "IK weight"}},
    });

    // --- Methods: Root Motion ---

    sys.methods.push_back(MethodDoc{
        .name = "setRootMotionConfig",
        .qualifiedName = "bestow.animation.setRootMotionConfig",
        .description = "Set the root motion extraction configuration.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to configure"},
            {.name = "config", .type = "RootMotionConfig", .description = "Root motion configuration"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getRootMotionConfig",
        .qualifiedName = "bestow.animation.getRootMotionConfig",
        .description = "Get the current root motion configuration.",
        .params = {{.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"}},
        .returns = {{.type = "RootMotionConfig", .description = "Current configuration"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setRootMotionEnabled",
        .qualifiedName = "bestow.animation.setRootMotionEnabled",
        .description = "Enable or disable root motion extraction.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to modify"},
            {.name = "enabled", .type = "boolean", .description = "true to enable root motion"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isRootMotionEnabled",
        .qualifiedName = "bestow.animation.isRootMotionEnabled",
        .description = "Check if root motion extraction is enabled.",
        .params = {{.name = "animator", .type = "AnimatorHandle", .description = "Animator to check"}},
        .returns = {{.type = "boolean", .description = "true if enabled"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getRootMotion",
        .qualifiedName = "bestow.animation.getRootMotion",
        .description = "Get the current frame's root motion data.",
        .params = {{.name = "animator", .type = "AnimatorHandle", .description = "Animator to query"}},
        .returns = {{.type = "RootMotion", .description = "Root motion data for this frame"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "extractRootMotion",
        .qualifiedName = "bestow.animation.extractRootMotion",
        .description = "Extract root motion between two times in a clip.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Clip to extract from"},
            {.name = "fromTime", .type = "number", .description = "Start time in seconds"},
            {.name = "toTime", .type = "number", .description = "End time in seconds"},
        },
        .returns = {{.type = "RootMotion", .description = "Extracted root motion"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "consumeRootMotion",
        .qualifiedName = "bestow.animation.consumeRootMotion",
        .description = "Consume (reset) the accumulated root motion delta. Call after applying root motion to the entity.",
        .params = {{.name = "animator", .type = "AnimatorHandle", .description = "Animator to reset"}},
    });

    // --- Methods: Ragdoll ---

    sys.methods.push_back(MethodDoc{
        .name = "hasRagdoll",
        .qualifiedName = "bestow.animation.hasRagdoll",
        .description = "Check if an entity has a ragdoll.",
        .params = {{.name = "entity", .type = "Entity", .description = "Entity to check"}},
        .returns = {{.type = "boolean", .description = "true if the entity has a ragdoll"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getRagdollState",
        .qualifiedName = "bestow.animation.getRagdollState",
        .description = "Get the current ragdoll state of an entity.",
        .params = {{.name = "entity", .type = "Entity", .description = "Entity to query"}},
        .returns = {{.type = "RagdollState", .description = "Current ragdoll state"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setRagdollBlendWeight",
        .qualifiedName = "bestow.animation.setRagdollBlendWeight",
        .description = "Set the blend weight between animation and ragdoll.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "Entity to modify"},
            {.name = "weight", .type = "number", .description = "Blend weight (0=animation, 1=ragdoll)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getRagdollBlendWeight",
        .qualifiedName = "bestow.animation.getRagdollBlendWeight",
        .description = "Get the ragdoll blend weight.",
        .params = {{.name = "entity", .type = "Entity", .description = "Entity to query"}},
        .returns = {{.type = "number", .description = "Blend weight"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isRagdollActive",
        .qualifiedName = "bestow.animation.isRagdollActive",
        .description = "Check if an entity's ragdoll is actively simulating.",
        .params = {{.name = "entity", .type = "Entity", .description = "Entity to check"}},
        .returns = {{.type = "boolean", .description = "true if ragdoll is active"}},
    });

    // --- Methods: Event Subscriptions ---

    sys.methods.push_back(MethodDoc{
        .name = "subscribeToEvents",
        .qualifiedName = "bestow.animation.subscribeToEvents",
        .description = "Subscribe to animation events on an animator.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to subscribe to"},
            {.name = "callback", .type = "function", .description = "Callback function receiving an AnimationEvent"},
        },
        .returns = {{.type = "SubscriptionId", .description = "ID for unsubscribing"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "subscribeToComplete",
        .qualifiedName = "bestow.animation.subscribeToComplete",
        .description = "Subscribe to animation completion on an animator.",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to subscribe to"},
            {.name = "callback", .type = "function", .description = "Callback(animator, clip, layer) called when an animation completes"},
        },
        .returns = {{.type = "SubscriptionId", .description = "ID for unsubscribing"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "subscribeToLayerChanges",
        .qualifiedName = "bestow.animation.subscribeToLayerChanges",
        .description = "Subscribe to layer animation changes (clip transitions).",
        .params = {
            {.name = "animator", .type = "AnimatorHandle", .description = "Animator to subscribe to"},
            {.name = "callback", .type = "function", .description = "Callback(animator, layer, prevClip, newClip) called on transitions"},
        },
        .returns = {{.type = "SubscriptionId", .description = "ID for unsubscribing"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "unsubscribe",
        .qualifiedName = "bestow.animation.unsubscribe",
        .description = "Unsubscribe from animation events.",
        .params = {
            {.name = "id", .type = "SubscriptionId", .description = "Subscription ID to cancel"},
        },
    });

    // --- Methods: Statistics & Debugging ---

    sys.methods.push_back(MethodDoc{
        .name = "getStats",
        .qualifiedName = "bestow.animation.getStats",
        .description = "Get animation system performance statistics.",
        .returns = {{.type = "AnimationStats", .description = "Current statistics"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "resetFrameStats",
        .qualifiedName = "bestow.animation.resetFrameStats",
        .description = "Reset per-frame statistics counters.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "setDebugVisualization",
        .qualifiedName = "bestow.animation.setDebugVisualization",
        .description = "Enable or disable skeleton debug visualization.",
        .params = {{.name = "enabled", .type = "boolean", .description = "true to enable"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isDebugVisualizationEnabled",
        .qualifiedName = "bestow.animation.isDebugVisualizationEnabled",
        .description = "Check if skeleton debug visualization is enabled.",
        .returns = {{.type = "boolean", .description = "true if enabled"}},
    });

    // --- Properties ---

    sys.properties.push_back(PropertyDoc{
        .name = "Handle",
        .type = "table",
        .description = "Table of invalid handle constants: InvalidSkeleton, InvalidClip, InvalidAnimator, InvalidSocket.",
        .readOnly = true,
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
