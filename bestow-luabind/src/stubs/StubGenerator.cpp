// bestow-luabind/src/stubs/StubGenerator.cpp
// Generates EmmyLua-annotated stub files for IDE support

module;

#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

//=============================================================================
// StubGenerator Implementation
//=============================================================================

void StubGenerator::generate(const std::filesystem::path& outputDir) {
    outputDir_ = outputDir;

    // Create output directory if it doesn't exist
    std::filesystem::create_directories(outputDir_);

    spdlog::info("[StubGenerator] Generating Lua stubs to: {}", outputDir_.string());

    // Generate stub files
    generateTypesStub();
    generateEntityStub();
    generateInputStub();
    generateAudioStub();
    generatePhysicsStub();
    generatePhysics3DStub();
    generateGraphics3DStub();
    generateAnimationStub();
    generateCoreStub();
    generateIndexStub();

    spdlog::info("[StubGenerator] Stub generation complete!");
}

void StubGenerator::generateTypesStub() {
    std::ofstream file(outputDir_ / "types.lua");
    file << R"lua(---@meta

-- Bestow Core Types
-- Auto-generated stubs for IDE support

---@class Vec2
---@field x number
---@field y number
---@operator add(Vec2): Vec2
---@operator sub(Vec2): Vec2
---@operator mul(number): Vec2
---@operator div(number): Vec2
---@operator unm: Vec2
Vec2 = {}

---Create a new Vec2
---@param x number
---@param y number
---@return Vec2
function Vec2.new(x, y) end

---Get the length of the vector
---@return number
function Vec2:length() end

---Get the squared length
---@return number
function Vec2:lengthSquared() end

---Normalize the vector
---@return Vec2
function Vec2:normalize() end

---Dot product
---@param other Vec2
---@return number
function Vec2:dot(other) end

---Linear interpolation
---@param other Vec2
---@param t number
---@return Vec2
function Vec2:lerp(other, t) end

---@class Vec3
---@field x number
---@field y number
---@field z number
---@operator add(Vec3): Vec3
---@operator sub(Vec3): Vec3
---@operator mul(number): Vec3
---@operator div(number): Vec3
---@operator unm: Vec3
Vec3 = {}

---Create a new Vec3
---@param x number
---@param y number
---@param z number
---@return Vec3
function Vec3.new(x, y, z) end

---Get the length of the vector
---@return number
function Vec3:length() end

---Get the squared length
---@return number
function Vec3:lengthSquared() end

---Normalize the vector
---@return Vec3
function Vec3:normalize() end

---Dot product
---@param other Vec3
---@return number
function Vec3:dot(other) end

---Cross product
---@param other Vec3
---@return Vec3
function Vec3:cross(other) end

---Linear interpolation
---@param other Vec3
---@param t number
---@return Vec3
function Vec3:lerp(other, t) end

---@class Vec4
---@field x number
---@field y number
---@field z number
---@field w number
Vec4 = {}

---Create a new Vec4
---@param x number
---@param y number
---@param z number
---@param w number
---@return Vec4
function Vec4.new(x, y, z, w) end

---@class Quat
---@field x number
---@field y number
---@field z number
---@field w number
Quat = {}

---Create a new quaternion
---@param x number
---@param y number
---@param z number
---@param w number
---@return Quat
function Quat.new(x, y, z, w) end

---Get the identity quaternion
---@return Quat
function Quat.identity() end

---Create from axis angle
---@param axis Vec3
---@param angle number
---@return Quat
function Quat.fromAxisAngle(axis, angle) end

---Create from Euler angles (radians)
---@param euler Vec3
---@return Quat
function Quat.fromEuler(euler) end

---Spherical interpolation
---@param other Quat
---@param t number
---@return Quat
function Quat:slerp(other, t) end

---Rotate a vector
---@param v Vec3
---@return Vec3
function Quat:rotateVector(v) end

---@class Color
---@field r number Red (0-1)
---@field g number Green (0-1)
---@field b number Blue (0-1)
---@field a number Alpha (0-1)
Color = {}

---Create a new color
---@param r number Red (0-1)
---@param g number Green (0-1)
---@param b number Blue (0-1)
---@param a number Alpha (0-1)
---@return Color
function Color.new(r, g, b, a) end

---@class Mat4
Mat4 = {}

---Get the identity matrix
---@return Mat4
function Mat4.identity() end

---Create a look-at view matrix
---@param eye Vec3
---@param target Vec3
---@param up Vec3
---@return Mat4
function Mat4.lookAt(eye, target, up) end

---Create a perspective projection matrix
---@param fov number Field of view in radians
---@param aspect number Aspect ratio
---@param near number Near plane
---@param far number Far plane
---@return Mat4
function Mat4.perspective(fov, aspect, near, far) end

---Create an orthographic projection matrix
---@param left number
---@param right number
---@param bottom number
---@param top number
---@param near number
---@param far number
---@return Mat4
function Mat4.ortho(left, right, bottom, top, near, far) end

---Create a translation matrix
---@param v Vec3
---@return Mat4
function Mat4.translate(v) end

---Create a rotation matrix
---@param q Quat
---@return Mat4
function Mat4.rotate(q) end

---Create a scale matrix
---@param v Vec3
---@return Mat4
function Mat4.scale(v) end

---@class Entity : number
---An entity handle (opaque integer)

---@alias WorldHandle integer
---@alias BodyHandle integer
---@alias CharacterHandle integer
---@alias MeshHandle integer
---@alias MaterialHandle integer
---@alias SoundHandle integer
---@alias ChannelHandle integer
---@alias AnimationHandle integer
)lua";
    file.close();
}

void StubGenerator::generateEntityStub() {
    std::ofstream file(outputDir_ / "entity.lua");
    file << R"lua(---@meta

-- Bestow Entity System
-- Auto-generated stubs for IDE support

---@class bestow.entity
bestow.entity = {}

---Create a new entity
---@return Entity
function bestow.entity.create() end

---Destroy an entity
---@param entity Entity
function bestow.entity.destroy(entity) end

---Check if an entity is valid
---@param entity Entity
---@return boolean
function bestow.entity.isValid(entity) end

---Get the total entity count
---@return integer
function bestow.entity.count() end

---Iterate over all entities
---@param callback fun(entity: Entity)
function bestow.entity.each(callback) end

---Add a component to an entity
---@param entity Entity
---@param typeName string Component type name
---@param data? table Optional component data
---@return boolean success
function bestow.entity.addComponent(entity, typeName, data) end

---Remove a component from an entity
---@param entity Entity
---@param typeName string Component type name
---@return boolean success
function bestow.entity.removeComponent(entity, typeName) end

---Check if an entity has a component
---@param entity Entity
---@param typeName string Component type name
---@return boolean
function bestow.entity.hasComponent(entity, typeName) end

---Get a component from an entity
---@param entity Entity
---@param typeName string Component type name
---@return table|nil Component data or nil if not found
function bestow.entity.getComponent(entity, typeName) end

---Set component data on an entity
---@param entity Entity
---@param typeName string Component type name
---@param data table Component data
---@return boolean success
function bestow.entity.setComponent(entity, typeName, data) end

---Get a single field from a component
---@param entity Entity
---@param typeName string Component type name
---@param fieldName string Field name
---@return any|nil Field value or nil if not found
function bestow.entity.getField(entity, typeName, fieldName) end

---Set a single field on a component
---@param entity Entity
---@param typeName string Component type name
---@param fieldName string Field name
---@param value any Field value
---@return boolean success
function bestow.entity.setField(entity, typeName, fieldName, value) end

---Check if a component type is registered
---@param typeName string Component type name
---@return boolean
function bestow.entity.isTypeRegistered(typeName) end

---Get all registered component types
---@return string[]
function bestow.entity.getRegisteredTypes() end

---Get type info for a component type
---@param typeName string Component type name
---@return ComponentTypeInfo|nil
function bestow.entity.getTypeInfo(typeName) end

---Get field info for a component type
---@param typeName string Component type name
---@return ComponentFieldInfo[]
function bestow.entity.getFields(typeName) end

---@class ComponentTypeInfo
---@field name string
---@field size integer
---@field fields ComponentFieldInfo[]
---@field canConstruct boolean

---@class ComponentFieldInfo
---@field name string
---@field type string
---@field offset integer
---@field size integer
---@field readOnly boolean
)lua";
    file.close();
}

void StubGenerator::generateInputStub() {
    std::ofstream file(outputDir_ / "input.lua");
    file << R"lua(---@meta

-- Bestow Input System
-- Auto-generated stubs for IDE support

---@class bestow.input
bestow.input = {}

---Check if an action is currently active
---@param action string
---@return boolean
function bestow.input.isActionActive(action) end

---Check if an action was just pressed this frame
---@param action string
---@return boolean
function bestow.input.wasActionJustPressed(action) end

---Check if an action was just released this frame
---@param action string
---@return boolean
function bestow.input.wasActionJustReleased(action) end

---Get the value of an action (for analog inputs)
---@param action string
---@return number
function bestow.input.getActionValue(action) end

---Check if a specific key is currently held down
---@param key integer
---@return boolean
function bestow.input.isKeyDown(key) end

---Check if a key was just pressed this frame
---@param key integer
---@return boolean
function bestow.input.wasKeyJustPressed(key) end

---Check if a key was just released this frame
---@param key integer
---@return boolean
function bestow.input.wasKeyJustReleased(key) end

---Get the current mouse position
---@return Vec2
function bestow.input.getMousePosition() end

---Get the mouse movement since last frame
---@return Vec2
function bestow.input.getMouseDelta() end

---Check if a mouse button is held down
---@param button integer 0=left, 1=right, 2=middle
---@return boolean
function bestow.input.isMouseButtonDown(button) end

---Check if a mouse button was just pressed
---@param button integer 0=left, 1=right, 2=middle
---@return boolean
function bestow.input.wasMouseButtonJustPressed(button) end

---Check if a mouse button was just released
---@param button integer 0=left, 1=right, 2=middle
---@return boolean
function bestow.input.wasMouseButtonJustReleased(button) end

---Get the mouse scroll wheel delta
---@return Vec2
function bestow.input.getScrollDelta() end

-- Key constants
---@class Keys
Keys = {
    Space = 32,
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Insert = 260,
    Delete = 261,
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    -- Letters
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70,
    G = 71, H = 72, I = 73, J = 74, K = 75, L = 76,
    M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82,
    S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
    Y = 89, Z = 90,
    -- Dvorak equivalents
    Comma = 44,  -- Forward in Dvorak (W position)
    -- Numbers
    Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51,
    Num4 = 52, Num5 = 53, Num6 = 54, Num7 = 55,
    Num8 = 56, Num9 = 57,
    -- Modifiers
    LeftShift = 340, RightShift = 344,
    LeftControl = 341, RightControl = 345,
    LeftAlt = 342, RightAlt = 346,
}
)lua";
    file.close();
}

void StubGenerator::generateAudioStub() {
    std::ofstream file(outputDir_ / "audio.lua");
    file << R"lua(---@meta

-- Bestow Audio System
-- Auto-generated stubs for IDE support

---@class bestow.audio
bestow.audio = {}

---Load a sound file
---@param path string Path to the sound file
---@return SoundHandle
function bestow.audio.loadSound(path) end

---Unload a sound
---@param handle SoundHandle
function bestow.audio.unloadSound(handle) end

---Play a sound
---@param handle SoundHandle
---@param volume? number Volume (0-1, default 1)
---@param pitch? number Pitch multiplier (default 1)
---@return ChannelHandle
function bestow.audio.playSound(handle, volume, pitch) end

---Stop a playing channel
---@param channel ChannelHandle
function bestow.audio.stopChannel(channel) end

---Pause or unpause a channel
---@param channel ChannelHandle
---@param paused boolean
function bestow.audio.setPaused(channel, paused) end

---Set channel volume
---@param channel ChannelHandle
---@param volume number
function bestow.audio.setChannelVolume(channel, volume) end

---Set channel pitch
---@param channel ChannelHandle
---@param pitch number
function bestow.audio.setChannelPitch(channel, pitch) end

---Set master volume
---@param volume number
function bestow.audio.setMasterVolume(volume) end

---Get master volume
---@return number
function bestow.audio.getMasterVolume() end

---Stop all sounds
function bestow.audio.stopAll() end
)lua";
    file.close();
}

void StubGenerator::generatePhysicsStub() {
    std::ofstream file(outputDir_ / "physics.lua");
    file << R"lua(---@meta

-- Bestow 2D Physics System
-- Auto-generated stubs for IDE support

---@class bestow.physics
bestow.physics = {}

---Create a physics body for an entity
---@param entity Entity
---@param def BodyDef2D
function bestow.physics.createBody(entity, def) end

---Destroy a physics body
---@param entity Entity
function bestow.physics.destroyBody(entity) end

---Set body velocity
---@param entity Entity
---@param velocity Vec2
function bestow.physics.setVelocity(entity, velocity) end

---Get body velocity
---@param entity Entity
---@return Vec2
function bestow.physics.getVelocity(entity) end

---Apply force to a body
---@param entity Entity
---@param force Vec2
function bestow.physics.applyForce(entity, force) end

---Apply impulse to a body
---@param entity Entity
---@param impulse Vec2
function bestow.physics.applyImpulse(entity, impulse) end

---@class BodyDef2D
---@field type string "static"|"dynamic"|"kinematic"
---@field position Vec2
---@field width number
---@field height number
---@field fixedRotation? boolean
---@field isSensor? boolean
)lua";
    file.close();
}

void StubGenerator::generatePhysics3DStub() {
    std::ofstream file(outputDir_ / "physics3d.lua");
    file << R"lua(---@meta

-- Bestow 3D Physics System
-- Auto-generated stubs for IDE support

---@class bestow.physics3d
bestow.physics3d = {}

---Create a physics world
---@param gravity? Vec3 Default: (0, -9.81, 0)
---@return WorldHandle
function bestow.physics3d.createWorld(gravity) end

---Destroy a physics world
---@param world WorldHandle
function bestow.physics3d.destroyWorld(world) end

---Step the physics simulation
---@param world WorldHandle
---@param dt number Delta time
function bestow.physics3d.stepWorld(world, dt) end

---Create a rigid body
---@param world WorldHandle
---@param def RigidBodyDef
---@return BodyHandle
function bestow.physics3d.createRigidBody(world, def) end

---Destroy a rigid body
---@param world WorldHandle
---@param body BodyHandle
function bestow.physics3d.destroyRigidBody(world, body) end

---Create a character controller
---@param world WorldHandle
---@param def CharacterDef
---@return CharacterHandle
function bestow.physics3d.createCharacter(world, def) end

---Destroy a character controller
---@param world WorldHandle
---@param character CharacterHandle
function bestow.physics3d.destroyCharacter(world, character) end

---Move a character
---@param world WorldHandle
---@param character CharacterHandle
---@param velocity Vec3
---@param dt number
function bestow.physics3d.moveCharacter(world, character, velocity, dt) end

---Get character ground state
---@param world WorldHandle
---@param character CharacterHandle
---@return CharacterGroundInfo
function bestow.physics3d.getCharacterGroundInfo(world, character) end

---Cast a ray
---@param world WorldHandle
---@param from Vec3
---@param to Vec3
---@return RaycastResult|nil
function bestow.physics3d.raycast(world, from, to) end

---@class RigidBodyDef
---@field motionType string "static"|"dynamic"|"kinematic"
---@field position Vec3
---@field rotation Quat
---@field shape ShapeDef
---@field mass? number
---@field friction? number
---@field restitution? number
---@field linearDamping? number
---@field angularDamping? number

---@class ShapeDef
---@field type string "box"|"sphere"|"capsule"|"cylinder"|"mesh"
---@field halfExtents? Vec3 For box
---@field radius? number For sphere, capsule, cylinder
---@field height? number For capsule, cylinder

---@class CharacterDef
---@field position Vec3
---@field rotation? Quat
---@field height number
---@field radius number
---@field maxSlopeAngle? number
---@field mass? number

---@class CharacterGroundInfo
---@field state string "OnGround"|"InAir"|"OnSteepGround"|"Sliding"
---@field groundNormal? Vec3
---@field groundEntity? BodyHandle
---@field groundPoint? Vec3

---@class RaycastResult
---@field hit boolean
---@field position Vec3
---@field normal Vec3
---@field distance number
---@field body BodyHandle
)lua";
    file.close();
}

void StubGenerator::generateGraphics3DStub() {
    std::ofstream file(outputDir_ / "graphics3d.lua");
    file << R"lua(---@meta

-- Bestow 3D Graphics System
-- Auto-generated stubs for IDE support

---@class bestow.graphics3d
bestow.graphics3d = {}

---Begin a new frame
function bestow.graphics3d.beginFrame() end

---End the current frame and present
function bestow.graphics3d.endFrame() end

---Check if the window should close
---@return boolean
function bestow.graphics3d.shouldClose() end

---Set the active camera
---@param camera Camera3D
function bestow.graphics3d.setCamera(camera) end

---Get the current camera
---@return Camera3D
function bestow.graphics3d.getCamera() end

---Set fog settings using a Fog struct
---@param fog Fog
function bestow.graphics3d.setFog(fog) end

---Set ambient light
---@param color Vec3 RGB color
---@param intensity? number Optional intensity (default 1.0)
function bestow.graphics3d.setAmbientLight(color, intensity) end

---Set directional light using a DirectionalLight struct
---@param light DirectionalLight
function bestow.graphics3d.setDirectionalLight(light) end

---Clear directional light
function bestow.graphics3d.clearDirectionalLight() end

---Add a point light
---@param light PointLight
---@param position Vec3
---@return integer lightId
function bestow.graphics3d.addPointLight(light, position) end

---Add a spot light
---@param light SpotLight
---@param position Vec3
---@return integer lightId
function bestow.graphics3d.addSpotLight(light, position) end

---Remove a light
---@param lightId integer
function bestow.graphics3d.removeLight(lightId) end

---Clear all lights
function bestow.graphics3d.clearLights() end

---Draw a mesh at a transform (multiple overloads)
---@overload fun(mesh: MeshHandle, material: MaterialHandle, worldMatrix: Mat4, castShadow?: boolean, receiveShadow?: boolean)
---@overload fun(mesh: MeshHandle, material: MaterialHandle, transform: Transform3D, castShadow?: boolean, receiveShadow?: boolean)
---@param mesh MeshHandle
---@param material MaterialHandle
---@param transformOrMatrix Transform3D|Mat4
---@param castShadow? boolean Default true
---@param receiveShadow? boolean Default true
function bestow.graphics3d.drawMesh(mesh, material, transformOrMatrix, castShadow, receiveShadow) end

---Create a cube mesh
---@param size number
---@return MeshHandle
function bestow.graphics3d.createCubeMesh(size) end

---Create a sphere mesh
---@param radius number
---@param segments? integer
---@return MeshHandle
function bestow.graphics3d.createSphereMesh(radius, segments) end

---Create a plane mesh
---@param width number
---@param height number
---@return MeshHandle
function bestow.graphics3d.createPlaneMesh(width, height) end

---Load a mesh from file
---@param path string
---@return MeshHandle
function bestow.graphics3d.loadMesh(path) end

---Load a material from file
---@param path string
---@return MaterialHandle
function bestow.graphics3d.loadMaterial(path) end

---Get the default PBR material
---@return MaterialHandle
function bestow.graphics3d.getDefaultPBRMaterial() end

---@class Camera3D
---@field transform Transform3D
---@field projection ProjectionType
---@field fovY number Field of view in radians (perspective)
---@field aspectRatio number
---@field orthoWidth number (orthographic)
---@field orthoHeight number (orthographic)
---@field nearPlane number
---@field farPlane number
Camera3D = {}

---Create a new Camera3D
---@return Camera3D
function Camera3D.new() end

---@class Transform3D
---@field position Vec3
---@field rotation Quat
---@field scale Vec3
Transform3D = {}

---Create a new Transform3D
---@return Transform3D
function Transform3D.new() end

---@class Fog
---@field enabled boolean
---@field color Color
---@field density number
---@field startDistance number
---@field endDistance number
Fog = {}

---Create a new Fog struct
---@return Fog
function Fog.new() end

---@class DirectionalLight
---@field direction Vec3
---@field color Color
---@field intensity number
---@field castShadows boolean
---@field shadowMapResolution integer
DirectionalLight = {}

---Create a new DirectionalLight
---@return DirectionalLight
function DirectionalLight.new() end

---@class PointLight
---@field color Color
---@field intensity number
---@field range number
---@field castShadows boolean
PointLight = {}

---Create a new PointLight
---@return PointLight
function PointLight.new() end

---@class SpotLight
---@field direction Vec3
---@field color Color
---@field intensity number
---@field range number
---@field innerConeAngle number
---@field outerConeAngle number
---@field castShadows boolean
SpotLight = {}

---Create a new SpotLight
---@return SpotLight
function SpotLight.new() end

---@alias ProjectionType
---| "Perspective"
---| "Orthographic"
)lua";
    file.close();
}

void StubGenerator::generateAnimationStub() {
    std::ofstream file(outputDir_ / "animation.lua");
    file << R"lua(---@meta

-- Bestow Animation System
-- Auto-generated stubs for IDE support

---@alias SkeletonHandle integer
---@alias AnimationClipHandle integer
---@alias AnimatorHandle integer
---@alias SocketHandle integer

---@class bestow.animation
bestow.animation = {}

--=============================================================================
-- Skeleton Management
--=============================================================================

---Check if a skeleton handle is valid
---@param skeleton SkeletonHandle
---@return boolean
function bestow.animation.isValidSkeleton(skeleton) end

---Destroy a skeleton
---@param skeleton SkeletonHandle
function bestow.animation.destroySkeleton(skeleton) end

---Get skeleton info
---@param skeleton SkeletonHandle
---@return SkeletonInfo
function bestow.animation.getSkeletonInfo(skeleton) end

---Find a bone index by name
---@param skeleton SkeletonHandle
---@param boneName string
---@return integer
function bestow.animation.findBoneIndex(skeleton, boneName) end

---Get all bone names
---@param skeleton SkeletonHandle
---@return string[]
function bestow.animation.getBoneNames(skeleton) end

---Get bone count
---@param skeleton SkeletonHandle
---@return integer
function bestow.animation.getBoneCount(skeleton) end

--=============================================================================
-- Animation Clip Management
--=============================================================================

---Check if a clip handle is valid
---@param clip AnimationClipHandle
---@return boolean
function bestow.animation.isValidClip(clip) end

---Destroy an animation clip
---@param clip AnimationClipHandle
function bestow.animation.destroyAnimationClip(clip) end

---Get animation clip info
---@param clip AnimationClipHandle
---@return AnimationClipInfo
function bestow.animation.getAnimationClipInfo(clip) end

---Find a clip by name
---@param skeleton SkeletonHandle
---@param clipName string
---@return AnimationClipHandle
function bestow.animation.findClip(skeleton, clipName) end

---Get all clip handles for a skeleton
---@param skeleton SkeletonHandle
---@return AnimationClipHandle[]
function bestow.animation.getClipsForSkeleton(skeleton) end

---Get all clip names for a skeleton
---@param skeleton SkeletonHandle
---@return string[]
function bestow.animation.getClipNames(skeleton) end

--=============================================================================
-- Animator Management
--=============================================================================

---Create an animator for a skeleton
---@param skeleton SkeletonHandle
---@return AnimatorHandle|nil Returns nil on error
function bestow.animation.createAnimator(skeleton) end

---Destroy an animator
---@param animator AnimatorHandle
function bestow.animation.destroyAnimator(animator) end

---Check if an animator handle is valid
---@param animator AnimatorHandle
---@return boolean
function bestow.animation.isValidAnimator(animator) end

---Get the skeleton for an animator
---@param animator AnimatorHandle
---@return SkeletonHandle
function bestow.animation.getAnimatorSkeleton(animator) end

--=============================================================================
-- Animator Playback Control
--=============================================================================

---Play an animation (multiple overloads)
---@overload fun(animator: AnimatorHandle, clip: AnimationClipHandle, transitionTime?: number)
---@overload fun(animator: AnimatorHandle, clipName: string, transitionTime?: number)
---@overload fun(animator: AnimatorHandle, config: AnimationPlayConfig)
---@param animator AnimatorHandle
---@param clipOrName AnimationClipHandle|string|AnimationPlayConfig
---@param transitionTime? number Crossfade duration (default 0.25)
function bestow.animation.play(animator, clipOrName, transitionTime) end

---Stop playback
---@param animator AnimatorHandle
---@param fadeOutTime? number Fade out duration (default 0)
function bestow.animation.stop(animator, fadeOutTime) end

---Stop a specific layer
---@param animator AnimatorHandle
---@param layer integer
---@param fadeOutTime? number
function bestow.animation.stopLayer(animator, layer, fadeOutTime) end

---Set paused state
---@param animator AnimatorHandle
---@param paused boolean
function bestow.animation.setPaused(animator, paused) end

---Check if paused
---@param animator AnimatorHandle
---@return boolean
function bestow.animation.isPaused(animator) end

---Set playback speed
---@param animator AnimatorHandle
---@param speed number
function bestow.animation.setSpeed(animator, speed) end

---Get playback speed
---@param animator AnimatorHandle
---@return number
function bestow.animation.getSpeed(animator) end

---Check if any animation is playing
---@param animator AnimatorHandle
---@return boolean
function bestow.animation.isPlaying(animator) end

---Check if a specific layer is playing
---@param animator AnimatorHandle
---@param layer integer
---@return boolean
function bestow.animation.isLayerPlaying(animator, layer) end

--=============================================================================
-- Animator Layer Control
--=============================================================================

---Get layer state
---@param animator AnimatorHandle
---@param layer integer
---@return AnimationLayerState
function bestow.animation.getLayerState(animator, layer) end

---Set layer weight
---@param animator AnimatorHandle
---@param layer integer
---@param weight number
function bestow.animation.setLayerWeight(animator, layer, weight) end

---Get layer weight
---@param animator AnimatorHandle
---@param layer integer
---@return number
function bestow.animation.getLayerWeight(animator, layer) end

---Set layer blend mode
---@param animator AnimatorHandle
---@param layer integer
---@param mode AnimationBlendMode
function bestow.animation.setLayerBlendMode(animator, layer, mode) end

---Get layer count
---@param animator AnimatorHandle
---@return integer
function bestow.animation.getLayerCount(animator) end

--=============================================================================
-- Animator Time Control
--=============================================================================

---Get current playback time
---@param animator AnimatorHandle
---@param layer? integer
---@return number
function bestow.animation.getCurrentTime(animator, layer) end

---Get normalized playback time (0-1)
---@param animator AnimatorHandle
---@param layer? integer
---@return number
function bestow.animation.getNormalizedTime(animator, layer) end

---Set current playback time
---@param animator AnimatorHandle
---@param time number
---@param layer? integer
function bestow.animation.setCurrentTime(animator, time, layer) end

---Set normalized playback time
---@param animator AnimatorHandle
---@param normalizedTime number
---@param layer? integer
function bestow.animation.setNormalizedTime(animator, normalizedTime, layer) end

---Get clip duration
---@param animator AnimatorHandle
---@param layer? integer
---@return number
function bestow.animation.getClipDuration(animator, layer) end

--=============================================================================
-- Bone Transforms
--=============================================================================

---Get bone transform (local space)
---@overload fun(animator: AnimatorHandle, boneIndex: integer): Mat4
---@overload fun(animator: AnimatorHandle, boneName: string): Mat4
---@param animator AnimatorHandle
---@param boneIndexOrName integer|string
---@return Mat4
function bestow.animation.getBoneTransform(animator, boneIndexOrName) end

---Get bone world transform
---@param animator AnimatorHandle
---@param boneIndex integer
---@param entityWorldMatrix Mat4
---@return Mat4
function bestow.animation.getBoneWorldTransform(animator, boneIndex, entityWorldMatrix) end

--=============================================================================
-- Socket System
--=============================================================================

---Define a socket attachment point
---@param skeleton SkeletonHandle
---@param def SocketDef
---@return SocketHandle|nil
function bestow.animation.defineSocket(skeleton, def) end

---Remove a socket
---@overload fun(socket: SocketHandle)
---@overload fun(skeleton: SkeletonHandle, name: string)
function bestow.animation.removeSocket(socketOrSkeleton, name) end

---Check if a socket exists
---@param skeleton SkeletonHandle
---@param socketName string
---@return boolean
function bestow.animation.hasSocket(skeleton, socketName) end

---Find a socket by name
---@param skeleton SkeletonHandle
---@param socketName string
---@return SocketHandle
function bestow.animation.findSocket(skeleton, socketName) end

---Get socket world transform
---@param animator AnimatorHandle
---@param socketNameOrHandle string|SocketHandle
---@param entityWorldMatrix Mat4
---@return SocketTransform|nil
function bestow.animation.getSocketTransform(animator, socketNameOrHandle, entityWorldMatrix) end

--=============================================================================
-- Inverse Kinematics
--=============================================================================

---Define a two-bone IK chain
---@param skeleton SkeletonHandle
---@param chain IKTwoBoneChain
---@return boolean
function bestow.animation.defineIKChain(skeleton, chain) end

---Define an aim IK config
---@param skeleton SkeletonHandle
---@param config IKAimConfig
---@return boolean
function bestow.animation.defineIKAim(skeleton, config) end

---Set an IK target
---@overload fun(animator: AnimatorHandle, target: IKTwoBoneTarget)
---@overload fun(animator: AnimatorHandle, target: IKAimTarget)
function bestow.animation.setIKTarget(animator, target) end

---Set IK weight
---@param animator AnimatorHandle
---@param targetName string
---@param weight number
function bestow.animation.setIKWeight(animator, targetName, weight) end

---Clear an IK target
---@param animator AnimatorHandle
---@param targetName string
function bestow.animation.clearIKTarget(animator, targetName) end

---Clear all IK targets
---@param animator AnimatorHandle
function bestow.animation.clearAllIKTargets(animator) end

--=============================================================================
-- Root Motion
--=============================================================================

---Set root motion config
---@param animator AnimatorHandle
---@param config RootMotionConfig
function bestow.animation.setRootMotionConfig(animator, config) end

---Get root motion config
---@param animator AnimatorHandle
---@return RootMotionConfig
function bestow.animation.getRootMotionConfig(animator) end

---Enable/disable root motion
---@param animator AnimatorHandle
---@param enabled boolean
function bestow.animation.setRootMotionEnabled(animator, enabled) end

---Check if root motion is enabled
---@param animator AnimatorHandle
---@return boolean
function bestow.animation.isRootMotionEnabled(animator) end

---Get root motion delta
---@param animator AnimatorHandle
---@return RootMotion
function bestow.animation.getRootMotion(animator) end

---Consume (clear) root motion delta
---@param animator AnimatorHandle
function bestow.animation.consumeRootMotion(animator) end

--=============================================================================
-- Event Subscriptions
--=============================================================================

---Subscribe to animation events
---@param animator AnimatorHandle
---@param callback fun(event: AnimationEvent)
---@return integer subscriptionId
function bestow.animation.subscribeToEvents(animator, callback) end

---Subscribe to animation completion
---@param animator AnimatorHandle
---@param callback fun(animator: AnimatorHandle, clip: AnimationClipHandle, layer: integer)
---@return integer subscriptionId
function bestow.animation.subscribeToComplete(animator, callback) end

---Unsubscribe from events
---@param subscriptionId integer
function bestow.animation.unsubscribe(subscriptionId) end

--=============================================================================
-- Statistics
--=============================================================================

---Get animation statistics
---@return AnimationStats
function bestow.animation.getStats() end

---Reset frame statistics
function bestow.animation.resetFrameStats() end

--=============================================================================
-- Type Definitions
--=============================================================================

---@class AnimationPlayConfig
---@field clip? AnimationClipHandle
---@field clipName? string
---@field startTime? number
---@field speed? number
---@field weight? number
---@field blendInTime? number
---@field blendOutTime? number
---@field wrapMode? AnimationWrapMode
---@field layer? integer
---@field blendMode? AnimationBlendMode
---@field restartIfSame? boolean

---@class AnimationLayerState
---@field clip AnimationClipHandle
---@field clipName string
---@field time number
---@field normalizedTime number
---@field speed number
---@field weight number
---@field fadeWeight number
---@field playing boolean
---@field paused boolean
---@field finished boolean

---@class AnimationClipInfo
---@field name string
---@field skeleton SkeletonHandle
---@field duration number
---@field ticksPerSecond number
---@field channelCount integer
---@field keyframeCount integer
---@field hasRootMotion boolean
---@field looping boolean

---@class SkeletonInfo
---@field boneCount integer
---@field rootBoneIndex integer

---@class SocketDef
---@field name string
---@field boneName string
---@field localPosition Vec3
---@field localRotation Quat
---@field localScale? Vec3
---@field attachMode? SocketAttachMode

---@class SocketTransform
---@field worldMatrix Mat4
---@field position Vec3
---@field rotation Quat
---@field scale Vec3
---@field forward Vec3
---@field up Vec3
---@field right Vec3

---@class IKTwoBoneChain
---@field name string
---@field rootBoneName string
---@field midBoneName string
---@field tipBoneName string

---@class IKTwoBoneTarget
---@field chainName string
---@field targetPosition Vec3
---@field poleVector? Vec3
---@field weight? number
---@field enabled? boolean

---@class IKAimConfig
---@field name string
---@field boneName string
---@field aimAxis? Vec3
---@field upAxis? Vec3
---@field horizontalLimit? number
---@field verticalLimit? number

---@class IKAimTarget
---@field configName string
---@field targetPosition Vec3
---@field worldUp? Vec3
---@field weight? number
---@field enabled? boolean

---@class RootMotionConfig
---@field enabled boolean
---@field extractTranslationX? boolean
---@field extractTranslationY? boolean
---@field extractTranslationZ? boolean
---@field extractRotationY? boolean
---@field extractRotationXZ? boolean
---@field rootBoneName? string

---@class RootMotion
---@field deltaPosition Vec3
---@field deltaRotation Quat
---@field totalPosition Vec3
---@field totalRotation Quat
---@field hasTranslation boolean
---@field hasRotation boolean

---@class AnimationEvent
---@field animator AnimatorHandle
---@field clip AnimationClipHandle
---@field layer integer
---@field name string
---@field clipTime number
---@field normalizedTime number
---@field stringParam? string
---@field floatParam? number
---@field intParam? integer

---@class AnimationStats
---@field skeletonCount integer
---@field clipCount integer
---@field animatorCount integer
---@field animatorsUpdated integer
---@field layersProcessed integer
---@field eventsDispatched integer
---@field updateTimeMs number

---@alias AnimationWrapMode
---| "Once"
---| "Loop"
---| "PingPong"
---| "ClampForever"

---@alias AnimationBlendMode
---| "Override"
---| "Additive"

---@alias SocketAttachMode
---| "FollowBone"
---| "FollowPosition"
---| "FollowRotation"
---| "WorldSpace"
)lua";
    file.close();
}

void StubGenerator::generateCoreStub() {
    std::ofstream file(outputDir_ / "core.lua");
    file << R"lua(---@meta

-- Bestow Core Utilities
-- Auto-generated stubs for IDE support

---@class bestow.core
bestow.core = {}

---Get the delta time since last frame
---@return number
function bestow.core.deltaTime() end

---Get the engine version
---@return string
function bestow.core.version() end

---Get the engine name
---@return string
function bestow.core.engine() end
)lua";
    file.close();
}

void StubGenerator::generateIndexStub() {
    std::ofstream file(outputDir_ / "_index.lua");
    file << R"lua(---@meta

-- Bestow Game Engine
-- Auto-generated stubs for IDE support
--
-- Add this folder to your Lua Language Server settings:
-- {
--   "Lua.workspace.library": ["path/to/bestow-sdk/stubs"]
-- }

---@class bestow
---@field core bestow.core
---@field entity bestow.entity
---@field input bestow.input
---@field audio bestow.audio
---@field physics bestow.physics
---@field physics3d bestow.physics3d
---@field graphics3d bestow.graphics3d
---@field animation bestow.animation
bestow = {}

---@class app
---@field main table The main.lua table (app.main)
---@field entities table Entity definitions (app.entities.*)
---@field systems table Game systems (app.systems.*)
---@field levels table Level definitions (app.levels.*)
app = {}

-- Global key constants for convenience
-- Can also use bestow.input.Key.SPACE, etc.
---@class Keys
---@field Space integer (32)
---@field Escape integer (256)
---@field Enter integer (257)
---@field Tab integer (258)
---@field Backspace integer (259)
---@field Comma integer (44)
---@field Period integer (46)
---@field A integer (65)
---@field B integer (66)
---@field C integer (67)
---@field D integer (68)
---@field E integer (69)
---@field F integer (70)
---@field G integer (71)
---@field H integer (72)
---@field I integer (73)
---@field J integer (74)
---@field K integer (75)
---@field L integer (76)
---@field M integer (77)
---@field N integer (78)
---@field O integer (79)
---@field P integer (80)
---@field Q integer (81)
---@field R integer (82)
---@field S integer (83)
---@field T integer (84)
---@field U integer (85)
---@field V integer (86)
---@field W integer (87)
---@field X integer (88)
---@field Y integer (89)
---@field Z integer (90)
---@field Up integer (265)
---@field Down integer (264)
---@field Left integer (263)
---@field Right integer (262)
---@field F1 integer (290)
---@field F2 integer (291)
---@field F3 integer (292)
---@field F4 integer (293)
---@field F5 integer (294)
---@field F6 integer (295)
---@field F7 integer (296)
---@field F8 integer (297)
---@field F9 integer (298)
---@field F10 integer (299)
---@field F11 integer (300)
---@field F12 integer (301)
---@field LeftShift integer (340)
---@field RightShift integer (344)
---@field LeftCtrl integer (341)
---@field RightCtrl integer (345)
---@field LeftAlt integer (342)
---@field RightAlt integer (346)
Keys = {}
)lua";
    file.close();
}

}  // namespace bestow
