# Bestow Game Development Guide

This is a **Lua-first game template** for the Bestow engine. Games are written entirely in Lua and run via the `bestow` CLI tool.

## Quick Start

```bash
# Run the game
bestow run main.lua

# Run with hot reload (recommended during development)
bestow run main.lua --hot-reload

# Run with debug overlay
bestow run main.lua --debug
```

## Core Architecture

**Bestow uses two Lua namespaces:**

| Namespace | Purpose | Hot Reload |
|-----------|---------|------------|
| `bestow.*` | C++ engine API (graphics, physics, audio, input) | No |
| `app.*` | Your game scripts (auto-discovered from folders) | Yes |

**Key principle:** Lua is the **director**. It tells the engine what to do, but never handles raw data. Assets are referenced by path/handle and the engine manages the actual data.

## Project Structure

```
my-game/
├── main.lua                 # Entry point (required)
├── app.config.lua           # Optional: folder ignore configuration
├── entities/                # Entity blueprints -> app.entities.*
│   ├── player.lua
│   └── enemies/
│       └── goblin.lua       -> app.entities.enemies.goblin
├── systems/                 # Game systems -> app.systems.*
│   ├── movement.lua
│   └── combat.lua
├── levels/                  # Level definitions -> app.levels.*
│   └── level1.lua
└── assets/                  # Game assets (textures, sounds, etc.)
    ├── textures/
    ├── sounds/
    └── materials/
```

## main.lua Template

```lua
return {
    -- Window configuration
    title = "My Game",
    width = 1280,
    height = 720,

    -- Called once at startup
    init = function()
        app.main.state = {
            player = app.entities.player.create(),
            enemies = {},
            score = 0,
            running = true
        }
    end,

    -- Called every frame (return false to quit)
    update = function(dt)
        local state = app.main.state
        app.systems.movement.update(dt)
        app.systems.combat.update(dt)
        return state.running
    end,

    -- Called every frame for rendering
    render = function()
        bestow.graphics3d.beginFrame()
        -- Entities with renderers are drawn automatically
        bestow.graphics3d.endFrame()
    end,

    -- Main entry point
    run = function()
        app.main.init()
        while app.main.update(bestow.core.deltaTime()) do
            app.main.render()
        end
    end
}
```

## Critical Rules

### 1. Hot Reload Safety

**Always access `app.*` inside functions, NEVER at file scope:**

```lua
-- WRONG: Cached reference breaks on hot reload
local movement = app.systems.movement
return {
    update = function(dt)
        movement.update(dt)  -- STALE after reload!
    end
}

-- CORRECT: Fresh reference each call
return {
    update = function(dt)
        local movement = app.systems.movement
        movement.update(dt)
    end
}
```

### 2. Assets Are Handles, Not Data

```lua
-- WRONG: io is disabled
local fileData = io.open("sounds/jump.wav")

-- CORRECT: Let the engine handle assets
local handle = bestow.assets.registerAsset(bestow.assets.Type.Sound, "sounds/jump.wav")
bestow.assets.loadAsset(handle)
bestow.audio.playOnChannel(bestow.audio.Channel.UI, { asset = handle, volume = 1.0 })
```

### 3. State Survives Hot Reload

```lua
-- Store ALL game state in app.main.state
app.main.state = {
    player = nil,
    enemies = {},
    score = 0,
    currentLevel = 1
}

-- Access anywhere
local state = app.main.state
state.score = state.score + 100
```

### 4. Disabled Lua Functions

For security and hot-reload correctness:
- `require()` -> Use `app.*` instead
- `dofile()`, `loadfile()`, `load()` -> Dynamic code not allowed
- `io.*` -> No file I/O (use bestow.assets)
- `os.execute()` -> No shell access

## Controls (Dvorak-Friendly)

The project owner uses Dvorak. Default movement should support both layouts:

| Action | Dvorak | QWERTY | Arrow Keys |
|--------|--------|--------|------------|
| Forward | `,` (comma) | W | Up |
| Back | `O` | S | Down |
| Left | `A` | A | Left |
| Right | `E` | D | Right |

```lua
-- Handle both layouts
local forward = bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W)
local back = bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S)
local left = bestow.input.isKeyDown(Keys.A)  -- Same on both
local right = bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D)
```

---

## Complete API Reference

### bestow.core - Core Engine

```lua
bestow.core.deltaTime() -> float  -- Time since last frame in seconds
```

### bestow.entity - Entity/Component System

```lua
-- Lifecycle
bestow.entity.create() -> Entity
bestow.entity.destroy(entity: Entity)
bestow.entity.isValid(entity: Entity) -> bool
bestow.entity.count() -> int
bestow.entity.each(callback: function(Entity))

-- Components (string-based)
bestow.entity.addComponent(entity, typeName: string, data: table?) -> bool
bestow.entity.removeComponent(entity, typeName: string) -> bool
bestow.entity.hasComponent(entity, typeName: string) -> bool
bestow.entity.getComponent(entity, typeName: string) -> table | nil
bestow.entity.setComponent(entity, typeName: string, data: table) -> bool
bestow.entity.getField(entity, typeName, fieldName) -> value | nil
bestow.entity.setField(entity, typeName, fieldName, value) -> bool

-- Reflection
bestow.entity.getRegisteredTypes() -> table<string>
bestow.entity.getTypeInfo(typeName: string) -> ComponentTypeInfo
bestow.entity.getFields(typeName: string) -> table<ComponentFieldInfo>
```

### bestow.input - Input System

```lua
-- Actions (mapped inputs)
bestow.input.isActionActive(action: string) -> bool
bestow.input.wasActionJustPressed(action: string) -> bool
bestow.input.wasActionJustReleased(action: string) -> bool
bestow.input.getActionValue(action: string) -> float

-- Keyboard
bestow.input.isKeyDown(keyCode: int) -> bool
bestow.input.wasKeyJustPressed(keyCode: int) -> bool
bestow.input.wasKeyJustReleased(keyCode: int) -> bool

-- Mouse
bestow.input.getMousePosition() -> Vec2
bestow.input.getMouseDelta() -> Vec2
bestow.input.isMouseButtonDown(button: int) -> bool
bestow.input.wasMouseButtonJustPressed(button: int) -> bool
bestow.input.getScrollDelta() -> float

-- Modifiers
bestow.input.isShiftPressed() -> bool
bestow.input.isCtrlPressed() -> bool
bestow.input.isAltPressed() -> bool

-- Mapping
bestow.input.registerMapping(mapping: InputMapping)
bestow.input.removeMapping(binding: InputBinding)
bestow.input.getMappings() -> table<InputMapping>

-- Text Input (for UI)
bestow.input.enableTextInput()
bestow.input.disableTextInput()
bestow.input.getTextInput() -> string
bestow.input.clearTextInput()

-- Controller
bestow.input.getConnectedControllerCount() -> int
bestow.input.isControllerConnected(index: int) -> bool
```

**Key Constants:**
```lua
Keys.A-Z, Keys.Space, Keys.Escape, Keys.Enter, Keys.Tab
Keys.Up, Keys.Down, Keys.Left, Keys.Right
Keys.Comma, Keys.O, Keys.E  -- Dvorak equivalents
Keys.F1-F12, Keys.LeftShift, Keys.LeftCtrl, Keys.LeftAlt
bestow.input.Mouse.LEFT, .RIGHT, .MIDDLE
```

### bestow.audio - Audio System

```lua
-- Channel Audio (music, ambience, UI)
bestow.audio.playOnChannel(channel: Channel, sound: ChannelSound | table)
bestow.audio.stopChannel(channel, fadeOutTime?: float)
bestow.audio.pauseChannel(channel)
bestow.audio.resumeChannel(channel)
bestow.audio.setChannelVolume(channel, volume: float)
bestow.audio.setChannelPitch(channel, pitch: float)
bestow.audio.isChannelPlaying(channel) -> bool
bestow.audio.getChannelState(channel) -> ChannelState

-- Positional (3D) Audio
bestow.audio.playPositional(sound: PositionalSound) -> SoundHandle
bestow.audio.stopPositional(handle: SoundHandle)
bestow.audio.updatePositionalPosition(handle, position: Vec3)
bestow.audio.isPositionalPlaying(handle) -> bool

-- 3D Listener
bestow.audio.setListener(listener: AudioListener)
bestow.audio.getListener() -> AudioListener

-- Global
bestow.audio.setMasterVolume(volume: float)
bestow.audio.getMasterVolume() -> float
bestow.audio.pauseAll()
bestow.audio.resumeAll()
bestow.audio.stopAll()

-- Channel Groups
bestow.audio.setGroupVolume(group: string, volume: float)
bestow.audio.assignChannelToGroup(channel, group: string)

-- Predefined Channels
bestow.audio.Channel.Music
bestow.audio.Channel.Ambience
bestow.audio.Channel.UI
```

**ChannelSound table:**
```lua
{ asset = AssetHandle, volume = 1.0, pitch = 1.0, looping = false, fadeInTime = 0.0 }
```

**PositionalSound table:**
```lua
{ asset = AssetHandle, position = Vec3, volume = 1.0, pitch = 1.0, minDistance = 1.0, maxDistance = 100.0 }
```

### bestow.physics - 2D Physics (Box2D)

```lua
-- Body Management
bestow.physics.createBody(entity, def: PhysicsBodyDef)
bestow.physics.destroyBody(entity)
bestow.physics.hasBody(entity) -> bool

-- Transform
bestow.physics.setPosition(entity, position: Vec2)
bestow.physics.getPosition(entity) -> Vec2
bestow.physics.setRotation(entity, radians: float)
bestow.physics.getRotation(entity) -> float
bestow.physics.setVelocity(entity, velocity: Vec2)
bestow.physics.getVelocity(entity) -> Vec2

-- Forces
bestow.physics.applyForce(entity, force: Vec2, point?: Vec2)
bestow.physics.applyImpulse(entity, impulse: Vec2, point?: Vec2)
bestow.physics.applyTorque(entity, torque: float)

-- Collision
bestow.physics.setCollisionLayer(entity, layer: int)
bestow.physics.setCollisionMask(entity, mask: int)
bestow.physics.setSensor(entity, isSensor: bool)

-- Queries
bestow.physics.queryAABB(min: Vec2, max: Vec2) -> table<Entity>
bestow.physics.queryCircle(center: Vec2, radius: float) -> table<Entity>
bestow.physics.raycast(origin, direction, maxDistance, mask?) -> RaycastHit | nil
bestow.physics.raycastAll(origin, direction, maxDistance, mask?) -> table<RaycastHit>

-- Ground Detection
bestow.physics.checkGrounded(entity, params?: GroundCheckParams) -> GroundCheckResult

-- World
bestow.physics.setGravity(gravity: Vec2)
bestow.physics.getGravity() -> Vec2

-- Collision Layers
bestow.physics.Layer.Player, .Enemy, .Projectile, .Terrain, .Trigger, .Collectible, .Ground
```

**PhysicsBodyDef:**
```lua
{
    type = "Static" | "Kinematic" | "Dynamic",
    transform = Transform2D,
    size = Vec2,
    fixedRotation = false,
    density = 1.0, friction = 0.3, restitution = 0.0,
    isSensor = false
}
```

### bestow.physics3d - 3D Physics (Jolt)

```lua
-- Body Management
bestow.physics3d.createBody(entity, def: PhysicsBodyDef3D) -> bool
bestow.physics3d.destroyBody(entity) -> bool
bestow.physics3d.hasBody(entity) -> bool
bestow.physics3d.getAllBodies() -> table<Entity>

-- Transform
bestow.physics3d.setPosition(entity, position: Vec3) -> bool
bestow.physics3d.getPosition(entity) -> Vec3 | nil
bestow.physics3d.setRotation(entity, rotation: Quat) -> bool
bestow.physics3d.getRotation(entity) -> Quat | nil
bestow.physics3d.setTransform(entity, transform: Transform3D) -> bool
bestow.physics3d.getTransform(entity) -> Transform3D | nil

-- Velocity
bestow.physics3d.setLinearVelocity(entity, velocity: Vec3) -> bool
bestow.physics3d.getLinearVelocity(entity) -> Vec3 | nil
bestow.physics3d.setAngularVelocity(entity, velocity: Vec3) -> bool
bestow.physics3d.getAngularVelocity(entity) -> Vec3 | nil

-- Forces & Impulses
bestow.physics3d.applyForce(entity, force: Vec3) -> bool
bestow.physics3d.applyForceAtPoint(entity, force: Vec3, point: Vec3) -> bool
bestow.physics3d.applyTorque(entity, torque: Vec3) -> bool
bestow.physics3d.applyImpulse(entity, impulse: Vec3) -> bool
bestow.physics3d.applyImpulseAtPoint(entity, impulse: Vec3, point: Vec3) -> bool
bestow.physics3d.applyAngularImpulse(entity, impulse: Vec3) -> bool

-- Properties
bestow.physics3d.setMass(entity, mass: float) -> bool
bestow.physics3d.getMass(entity) -> float | nil
bestow.physics3d.setLinearDamping(entity, damping: float) -> bool
bestow.physics3d.setAngularDamping(entity, damping: float) -> bool
bestow.physics3d.setGravityFactor(entity, factor: float) -> bool
bestow.physics3d.setFriction(entity, friction: float) -> bool
bestow.physics3d.setRestitution(entity, restitution: float) -> bool

-- Collision & Sleep
bestow.physics3d.setCollisionLayer(entity, layer) -> bool
bestow.physics3d.setCollisionMask(entity, mask) -> bool
bestow.physics3d.setSensor(entity, isSensor: bool) -> bool
bestow.physics3d.isAwake(entity) -> bool | nil
bestow.physics3d.wakeUp(entity) -> bool
bestow.physics3d.putToSleep(entity) -> bool

-- Queries
bestow.physics3d.raycast(origin, direction, maxDistance, filter?) -> RaycastHit3D | nil
bestow.physics3d.raycastAll(origin, direction, maxDistance, filter?) -> table<RaycastHit3D>
bestow.physics3d.sphereCast(origin, radius, direction, maxDistance, filter?) -> ShapeCastHit3D | nil
bestow.physics3d.boxCast(origin, halfExtents, rotation, direction, maxDistance, filter?) -> ShapeCastHit3D | nil
bestow.physics3d.overlapSphere(center, radius, filter?) -> table<Entity>
bestow.physics3d.overlapBox(center, halfExtents, rotation, filter?) -> table<Entity>
bestow.physics3d.queryAABB(min, max, filter?) -> table<Entity>

-- Character Controller
bestow.physics3d.createCharacter(entity, def: CharacterControllerDef) -> bool
bestow.physics3d.destroyCharacter(entity) -> bool
bestow.physics3d.moveCharacter(entity, velocity: Vec3, dt: float) -> bool
bestow.physics3d.getCharacterPosition(entity) -> Vec3 | nil
bestow.physics3d.setCharacterPosition(entity, position: Vec3) -> bool
bestow.physics3d.getCharacterGroundInfo(entity) -> CharacterGroundInfo | nil
bestow.physics3d.getCharacterVelocity(entity) -> Vec3 | nil

-- World & Callbacks
bestow.physics3d.setGravity(gravity: Vec3)
bestow.physics3d.getGravity() -> Vec3
bestow.physics3d.setCollisionCallback(callback: function(CollisionEvent3D))
bestow.physics3d.setTriggerEnterCallback(callback: function(TriggerEvent3D))
bestow.physics3d.setTriggerExitCallback(callback: function(TriggerEvent3D))

-- Collision Layers
bestow.physics3d.Layer.Default, .Static, .Dynamic, .Character, .Trigger, .Debris
```

**CharacterControllerDef:**
```lua
{ radius = 0.4, height = 1.8, stepHeight = 0.35, maxSlopeAngle = 45.0, mass = 80.0 }
```

**CharacterGroundInfo:**
```lua
{ grounded = bool, groundEntity = Entity, contactPoint = Vec3, surfaceNormal = Vec3, slopeAngle = float }
```

### bestow.graphics3d - 3D Graphics

```lua
-- Lifecycle
bestow.graphics3d.initialize(config: table) -> bool
bestow.graphics3d.shutdown()
bestow.graphics3d.beginFrame()
bestow.graphics3d.endFrame()

-- Primitive Meshes
bestow.graphics3d.createCubeMesh(size?: float) -> MeshHandle
bestow.graphics3d.createSphereMesh(radius?, segments?, rings?) -> MeshHandle
bestow.graphics3d.createCylinderMesh(radius?, height?, segments?) -> MeshHandle
bestow.graphics3d.createCapsuleMesh(radius?, height?, segments?, rings?) -> MeshHandle
bestow.graphics3d.createPlaneMesh(width?, height?, widthSeg?, heightSeg?) -> MeshHandle
bestow.graphics3d.destroyMesh(handle: MeshHandle)
bestow.graphics3d.getMeshBounds(handle) -> AABB3D

-- Materials
bestow.graphics3d.createMaterial(mat: PBRMaterial) -> MaterialHandle
bestow.graphics3d.createUnlitMaterial(mat: UnlitMaterial) -> MaterialHandle
bestow.graphics3d.destroyMaterial(handle)
bestow.graphics3d.getDefaultPBRMaterial() -> MaterialHandle
bestow.graphics3d.getDefaultUnlitMaterial() -> MaterialHandle
bestow.graphics3d.setMaterialBaseColor(handle, color: Vec4) -> bool
bestow.graphics3d.setMaterialMetallicRoughness(handle, metallic, roughness) -> bool
bestow.graphics3d.setMaterialEmissive(handle, emissive: Vec3) -> bool

-- Drawing
bestow.graphics3d.drawMesh(mesh, material, worldMatrix | transform, castShadow?, receiveShadow?)

-- Camera
bestow.graphics3d.setCamera(camera: Camera3D)
bestow.graphics3d.getCamera() -> Camera3D
bestow.graphics3d.screenToWorldRay(screenPos: Vec2) -> Ray3D
bestow.graphics3d.worldToScreen(worldPos: Vec3) -> Vec2 | nil

-- Lighting
bestow.graphics3d.setDirectionalLight(light: DirectionalLight)
bestow.graphics3d.clearDirectionalLight()
bestow.graphics3d.addPointLight(light: PointLight, position: Vec3) -> uint
bestow.graphics3d.addSpotLight(light: SpotLight, position: Vec3) -> uint
bestow.graphics3d.setLightPosition(lightId, position: Vec3)
bestow.graphics3d.removeLight(lightId)
bestow.graphics3d.clearLights()
bestow.graphics3d.setAmbientLight(color: Vec3, intensity?: float)

-- Environment
bestow.graphics3d.setSkybox(skybox: Skybox)
bestow.graphics3d.clearSkybox()
bestow.graphics3d.setFog(fog: Fog)

-- Shadows
bestow.graphics3d.setShadowsEnabled(enabled: bool)
bestow.graphics3d.areShadowsEnabled() -> bool
bestow.graphics3d.setShadowDistance(distance: float)

-- Debug Drawing
bestow.graphics3d.debugDrawLine(start, end, color?, duration?, depthTest?)
bestow.graphics3d.debugDrawBox(center, halfExtents, rotation?, color?, duration?, depthTest?)
bestow.graphics3d.debugDrawSphere(center, radius, color?, duration?, depthTest?)
bestow.graphics3d.debugDrawRay(origin, direction, length, color?, duration?, depthTest?)
bestow.graphics3d.debugDrawAxes(transform, size?, duration?, depthTest?)
bestow.graphics3d.debugClear()
bestow.graphics3d.setDebugRenderingEnabled(enabled: bool)

-- Window
bestow.graphics3d.getWindowSize() -> Size
bestow.graphics3d.setWindowSize(size: Size)
bestow.graphics3d.isFullscreen() -> bool
bestow.graphics3d.setFullscreen(fullscreen: bool)
bestow.graphics3d.shouldClose() -> bool

-- Render State
bestow.graphics3d.setClearColor(color: Color)
bestow.graphics3d.setVSync(enabled: bool)
bestow.graphics3d.setRenderScale(scale: float)

-- Post-Processing
bestow.graphics3d.setToneMapping(enabled: bool)
bestow.graphics3d.setExposure(exposure: float)
bestow.graphics3d.setBloom(enabled, threshold?, intensity?)
bestow.graphics3d.setSSAO(enabled, radius?, intensity?)

-- Statistics
bestow.graphics3d.getStats() -> RenderStats
```

**Camera3D:**
```lua
{
    transform = Transform3D,
    projection = "Perspective" | "Orthographic",
    fovY = 45.0, aspectRatio = 16/9,
    nearPlane = 0.1, farPlane = 1000.0
}
```

**PBRMaterial:**
```lua
{
    baseColorFactor = Vec4, baseColorTexture = AssetHandle,
    metallicFactor = 0.0, roughnessFactor = 0.5,
    normalTexture = AssetHandle, normalScale = 1.0,
    emissiveFactor = Vec3, emissiveTexture = AssetHandle,
    blendMode = "Opaque" | "AlphaBlend" | "Additive",
    cullMode = "Back" | "Front" | "None",
    doubleSided = false, castShadows = true, receiveShadows = true
}
```

**DirectionalLight:**
```lua
{ direction = Vec3, color = Vec3, intensity = 1.0, castShadows = true }
```

**PointLight:**
```lua
{ color = Vec3, intensity = 1.0, range = 10.0, castShadows = false }
```

**SpotLight:**
```lua
{ direction = Vec3, color = Vec3, intensity = 1.0, range = 10.0, innerConeAngle = 15, outerConeAngle = 30, castShadows = false }
```

### bestow.animation - Skeletal Animation

```lua
-- Skeleton & Clips
bestow.animation.findBoneIndex(skeleton, boneName) -> int | nil
bestow.animation.getBoneNames(skeleton) -> table<string>
bestow.animation.getBoneCount(skeleton) -> int
bestow.animation.findClip(skeleton, clipName) -> AnimationClipHandle
bestow.animation.getClipNames(skeleton) -> table<string>

-- Animator
bestow.animation.createAnimator(skeleton) -> AnimatorHandle | nil
bestow.animation.destroyAnimator(animator)
bestow.animation.isValidAnimator(animator) -> bool

-- Playback
bestow.animation.play(animator, clip | clipName, transitionTime?)
bestow.animation.play(animator, config: AnimationPlayConfig)
bestow.animation.stop(animator, fadeOutTime?)
bestow.animation.setPaused(animator, paused: bool)
bestow.animation.isPaused(animator) -> bool
bestow.animation.setSpeed(animator, speed: float)
bestow.animation.getSpeed(animator) -> float
bestow.animation.isPlaying(animator) -> bool

-- Layers
bestow.animation.setLayerWeight(animator, layer, weight: float)
bestow.animation.getLayerWeight(animator, layer) -> float
bestow.animation.setLayerBlendMode(animator, layer, mode: "Override" | "Additive")

-- Time Control
bestow.animation.getCurrentTime(animator, layer?) -> float
bestow.animation.getNormalizedTime(animator, layer?) -> float
bestow.animation.setCurrentTime(animator, time, layer?)
bestow.animation.setNormalizedTime(animator, normalizedTime, layer?)
bestow.animation.getClipDuration(animator, layer?) -> float

-- Bone Transforms
bestow.animation.getBoneTransform(animator, boneIndex | boneName) -> Transform3D
bestow.animation.getBoneWorldTransform(animator, boneIndex, entityWorldMatrix) -> Transform3D

-- Sockets (Attachment Points)
bestow.animation.defineSocket(skeleton, def: SocketDef) -> SocketHandle | nil
bestow.animation.hasSocket(skeleton, socketName) -> bool
bestow.animation.getSocketTransform(animator, socketName | socket, entityWorldMatrix) -> SocketTransform | nil

-- IK
bestow.animation.defineIKChain(skeleton, chain: IKTwoBoneChain) -> bool
bestow.animation.defineIKAim(skeleton, config: IKAimConfig) -> bool
bestow.animation.setIKTarget(animator, target: IKTwoBoneTarget | IKAimTarget)
bestow.animation.clearIKTarget(animator, targetName)
bestow.animation.setIKWeight(animator, targetName, weight: float)

-- Root Motion
bestow.animation.setRootMotionEnabled(animator, enabled: bool)
bestow.animation.isRootMotionEnabled(animator) -> bool
bestow.animation.getRootMotion(animator) -> RootMotion
bestow.animation.consumeRootMotion(animator)

-- Events
bestow.animation.subscribeToEvents(animator, callback: function(AnimationEvent)) -> SubscriptionId
bestow.animation.subscribeToComplete(animator, callback) -> SubscriptionId
bestow.animation.unsubscribe(id: SubscriptionId)
```

### bestow.ui - UI System (RML-based)

```lua
-- Lifecycle
bestow.ui.initialize(config?: table) -> bool
bestow.ui.shutdown()
bestow.ui.update(dt: float)
bestow.ui.render()

-- Documents
bestow.ui.loadDocument(path: string) -> UIDocumentHandle | nil
bestow.ui.loadDocumentFromString(content, sourceName?) -> UIDocumentHandle | nil
bestow.ui.unloadDocument(doc)
bestow.ui.showDocument(doc)
bestow.ui.hideDocument(doc)
bestow.ui.isDocumentVisible(doc) -> bool

-- Elements
bestow.ui.getElementById(doc, id: string) -> UIElementHandle | nil
bestow.ui.getElementsByClass(doc, className) -> table<UIElementHandle>
bestow.ui.getElementsByTag(doc, tagName) -> table<UIElementHandle>
bestow.ui.getChildren(elem) -> table<UIElementHandle>
bestow.ui.getParent(elem) -> UIElementHandle | nil

-- Element Properties
bestow.ui.setElementText(elem, text: string)
bestow.ui.getElementText(elem) -> string
bestow.ui.setElementVisible(elem, visibility: "Visible" | "Hidden" | "Collapsed")
bestow.ui.addClass(elem, className: string)
bestow.ui.removeClass(elem, className: string)
bestow.ui.hasClass(elem, className) -> bool
bestow.ui.setAttribute(elem, name, value: string)
bestow.ui.getAttribute(elem, name) -> string | nil
bestow.ui.setStyle(elem, property, value: string)
bestow.ui.getBounds(elem) -> UIRect
bestow.ui.focus(elem)
bestow.ui.blur(elem)

-- Dynamic Creation
bestow.ui.createElement(doc, tagName) -> UIElementHandle
bestow.ui.appendChild(parent, child)
bestow.ui.removeElement(elem)
bestow.ui.setInnerContent(elem, content: string)

-- Events
bestow.ui.onEvent(eventType, callback: function(UIEventData))
bestow.ui.onElementEvent(elem, eventType, callback)
bestow.ui.offEvent(eventType)

-- Input
bestow.ui.processInput(event: table) -> bool
bestow.ui.wantsKeyboardInput() -> bool
bestow.ui.wantsMouseInput() -> bool
```

### bestow.assets - Asset Management

```lua
-- Registration
bestow.assets.registerAsset(type: AssetType, path: string) -> AssetHandle
bestow.assets.unregisterAsset(handle)

-- Loading
bestow.assets.loadAsset(handle)       -- Sync
bestow.assets.loadAssetAsync(handle)  -- Async
bestow.assets.unloadAsset(handle)

-- State
bestow.assets.getAssetState(handle) -> AssetState
bestow.assets.isLoaded(handle) -> bool

-- Bulk
bestow.assets.loadAll()
bestow.assets.unloadAll()
bestow.assets.getAssetsOfType(type) -> table<AssetHandle>

-- Hot Reload
bestow.assets.enableHotReload(enable: bool)
bestow.assets.checkForReloads()
bestow.assets.reloadAsset(handle)

-- Library Discovery
bestow.assets.listLibraryAssets(relativeDir?) -> table<LibraryAssetInfo>
bestow.assets.listLibraryAssetsRecursive(relativeDir?) -> table<LibraryAssetInfo>
bestow.assets.listLibraryCategories() -> table<string>

-- Asset Types
bestow.assets.Type.Texture, .Sound, .Music, .Font, .Level, .Data, .Shader, .NavMesh, .BehaviorTree, .Mesh, .Model, .Material, .Cubemap

-- Asset States
bestow.assets.State.Unloaded, .Loading, .Loaded, .Failed
```

### bestow.config - Configuration

```lua
bestow.config.loadConfig(filePath: string) -> bool
bestow.config.parseLuaFile(filePath: string) -> any
bestow.config.reloadAll() -> bool

-- Typed Access
bestow.config.getFloat(key) -> float | nil
bestow.config.getInt(key) -> int | nil
bestow.config.getBool(key) -> bool | nil
bestow.config.getString(key) -> string | nil
bestow.config.getFloatOr(key, default) -> float
bestow.config.getIntOr(key, default) -> int
bestow.config.getBoolOr(key, default) -> bool
bestow.config.getStringOr(key, default) -> string
bestow.config.hasKey(key) -> bool

-- Hot Reload
bestow.config.enableHotReload(enable: bool)
bestow.config.isHotReloadEnabled() -> bool
```

### bestow.metrics - Performance Profiling

```lua
-- Frame
bestow.metrics.beginFrame()
bestow.metrics.endFrame()
bestow.metrics.frameCount() -> int

-- Zones
bestow.metrics.beginZone(name: string)
bestow.metrics.endZone(name: string)
bestow.metrics.getZoneTime(name) -> double  -- milliseconds
bestow.metrics.scopedZone(name) -> table    -- Call :finish() when done

-- Tracy Integration
bestow.metrics.plot(name, value: double)
bestow.metrics.message(text: string)
bestow.metrics.isTracyEnabled() -> bool

-- System Timing
bestow.metrics.beginSystem(name: string)
bestow.metrics.endSystem(name: string)
bestow.metrics.getSystemTiming(name) -> { callCount, totalTimeMs, lastFrameTimeMs, minTimeMs, maxTimeMs, avgTimeMs }

-- Export
bestow.metrics.getSummary() -> string
bestow.metrics.exportJson() -> string
bestow.metrics.reset()
bestow.metrics.getFrameMetrics() -> { frameNumber, totalFrameTimeMs, systems }
```

---

## Core Types Reference

### Math Types

```lua
-- Vec2
Vec2(x, y) or Vec2.new(x, y)
Vec2.zero(), Vec2.one(), Vec2.up(), Vec2.down(), Vec2.left(), Vec2.right()
v:length(), v:lengthSquared(), v:normalize(), v:dot(other)
v + other, v - other, v * scalar, v / scalar, -v

-- Vec3
Vec3(x, y, z) or Vec3.new(x, y, z)
Vec3.zero(), Vec3.one(), Vec3.up(), Vec3.down(), Vec3.left(), Vec3.right(), Vec3.forward(), Vec3.back()
v:length(), v:lengthSquared(), v:normalize(), v:dot(other), v:cross(other)
v + other, v - other, v * scalar, v / scalar, -v

-- Vec4
Vec4(x, y, z, w) or Vec4.new(x, y, z, w)
v.x, v.y, v.z, v.w

-- Quat (Quaternion)
Quat(w, x, y, z)
Quat.identity()
Quat.fromAxisAngle(axis: Vec3, angle: float)  -- angle in radians
Quat.fromEuler(pitch, yaw, roll)              -- angles in radians
Quat.lookAt(direction: Vec3, up: Vec3)
q:normalize(), q:inverse(), q:rotateVector(vec: Vec3)
q1 * q2  -- quaternion multiplication

-- Color (0-255 RGBA)
Color(r, g, b, a)
Color.new(r, g, b, a)          -- Auto-detects 0-1 or 0-255 range
Color.fromFloat(r, g, b, a)    -- Explicit 0.0-1.0 range
Color.white(), Color.black(), Color.red(), Color.green(), Color.blue(), Color.transparent()

-- Transform2D
Transform2D.new(x, y, rotation, scaleX, scaleY)
t.x, t.y, t.rotation, t.scaleX, t.scaleY, t.position, t.scale

-- Transform3D
Transform3D.identity()
Transform3D.new(position: Vec3, rotation: Quat, scale: Vec3)
t.position, t.rotation, t.scale

-- AABB3D
AABB3D.new(min: Vec3, max: Vec3)
aabb.min, aabb.max, aabb.center, aabb.extents, aabb.size

-- Ray3D
Ray3D.new(origin: Vec3, direction: Vec3)
ray.origin, ray.direction
ray:pointAt(distance) -> Vec3

-- Size
Size.new(width: int, height: int)
s.width, s.height

-- Entity
entity == other  -- comparison
tostring(entity) -- string representation
```

---

## Available Skills

Claude has access to specialized skills for all game development tasks. These are automatically invoked when relevant:

### Core Systems
- **entity-system** - Creating entities, adding/removing components
- **input-system** - Keyboard, mouse, controller input
- **graphics-system** - 3D rendering, cameras, lighting, fog
- **audio-system** - Music, sound effects, positional audio
- **physics-system** - 2D/3D physics, collision, raycasting
- **camera-system** - Camera following, shake effects, zoom
- **assets-system** - Loading and managing game assets
- **animation-system** - Skeletal animation, IK, sockets

### Game Features
- **level-system** - Loading levels, spawn points
- **save-system** - Saving and loading game progress
- **game-state** - Menu states, pause, transitions

### Common Patterns
- **player-controller** - Movement, jumping, abilities
- **enemies-and-ai** - Enemy behavior, pathfinding
- **collectibles-and-items** - Pickups, inventory
- **ui-and-menus** - HUD, menus, text

### Development
- **hot-reload** - Hot reload patterns and best practices
- **project-structure** - Organizing game code

---

## Common Patterns

### Entity Blueprint Pattern

```lua
-- entities/player.lua
return {
    defaults = {
        Transform3D = { position = Vec3.new(0, 1, 0), rotation = Quat.identity(), scale = Vec3.one() },
        Health = { current = 100, max = 100 },
        PlayerController = { speed = 5.0, jumpForce = 10.0 }
    },

    create = function(overrides)
        local self = app.entities.player
        local entity = bestow.entity.create()
        for compName, defaults in pairs(self.defaults) do
            local data = {}
            for k, v in pairs(defaults) do data[k] = v end
            if overrides and overrides[compName] then
                for k, v in pairs(overrides[compName]) do data[k] = v end
            end
            bestow.entity.addComponent(entity, compName, data)
        end
        return entity
    end,

    spawnAt = function(position)
        return app.entities.player.create({ Transform3D = { position = position } })
    end
}
```

### Platformer Movement Pattern

```lua
-- systems/player.lua
return {
    speed = 8.0,
    jumpForce = 12.0,
    gravity = -30.0,
    velocityY = 0,

    update = function(dt)
        local self = app.systems.player
        local state = app.main.state
        if not state.player then return end

        -- Input (Dvorak + QWERTY)
        local moveX = 0
        if bestow.input.isKeyDown(Keys.A) then moveX = moveX - 1 end
        if bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D) then moveX = moveX + 1 end

        local moveZ = 0
        if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then moveZ = moveZ - 1 end
        if bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S) then moveZ = moveZ + 1 end

        -- Ground check
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = groundInfo and groundInfo.grounded

        -- Gravity
        if not grounded then
            self.velocityY = self.velocityY + self.gravity * dt
        else
            self.velocityY = 0
            -- Jump
            if bestow.input.wasKeyJustPressed(Keys.Space) then
                self.velocityY = self.jumpForce
            end
        end

        -- Build velocity
        local velocity = Vec3.new(moveX * self.speed, self.velocityY, moveZ * self.speed)
        bestow.physics3d.moveCharacter(state.player, velocity, dt)
    end
}
```

### Follow Camera Pattern

```lua
-- systems/camera.lua
return {
    offset = Vec3.new(0, 10, -15),
    smoothing = 5.0,

    update = function(dt)
        local self = app.systems.camera
        local state = app.main.state
        if not state.player then return end

        local playerPos = bestow.entity.getField(state.player, "Transform3D", "position")
        local targetPos = playerPos + self.offset

        local currentCam = bestow.graphics3d.getCamera()
        local newPos = currentCam.transform.position + (targetPos - currentCam.transform.position) * self.smoothing * dt

        local lookAt = playerPos
        local forward = (lookAt - newPos):normalize()
        local rotation = Quat.lookAt(forward, Vec3.up())

        bestow.graphics3d.setCamera({
            transform = { position = newPos, rotation = rotation, scale = Vec3.one() },
            projection = "Perspective",
            fovY = 45.0,
            nearPlane = 0.1,
            farPlane = 1000.0
        })
    end
}
```

---

## Debugging

```bash
# Debug mode with overlay
bestow run main.lua --debug

# Hot reload (F5 to force reload)
bestow run main.lua --hot-reload

# FPS and stats (F1 during gameplay)
```

## Common Gotchas

1. **Forgetting to return from update()** - Must return true to continue, false to quit
2. **Caching app.* references at file scope** - Breaks hot reload
3. **Trying to read files directly** - Use asset paths instead
4. **Not storing state in app.main.state** - State lost on hot reload
5. **Using WASD only** - Remember Dvorak users (,AOE)
6. **Forgetting beginFrame/endFrame** - Graphics won't render
7. **Not checking nil returns** - Many functions return nil on failure

---

## Next Steps

1. Create entity blueprints in `entities/`
2. Create game systems in `systems/`
3. Define levels in `levels/`
4. Add assets to `assets/`
5. Run with `bestow run main.lua --hot-reload`
