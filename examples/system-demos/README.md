# JFrame System Demos

Comprehensive examples demonstrating each JFrame system's complete API.

## Overview

Each demo in this directory focuses on **one system** and demonstrates its **entire API**. These serve as:

1. **Reference implementations** - See how to use every method
2. **API verification** - Confirm the system works correctly
3. **Learning resources** - Understand system capabilities
4. **Test fixtures** - Automated testing validates functionality

## Available Demos

| Demo | System | Description |
|------|--------|-------------|
| `entity-demo` | Entity System | ECS with EnTT - entities, components, queries, views |
| `events-demo` | Events System | Pub/sub events - publish, subscribe, queue |
| `config-demo` | Config System | Lua configuration loading and hot reload |
| `assets-demo` | Assets System | Asset loading, caching, hot reload |
| `input-demo` | Input System | Keyboard, mouse, controller, action mapping |
| `save-demo` | Save System | Game state serialization, profiles, auto-save |
| `graphics-demo` | Graphics System | Rendering sprites, primitives, text, camera |
| `audio-demo` | Audio System | Channel audio, 3D positional, volume groups |
| `physics-demo` | Physics System | Box2D bodies, forces, raycasts, collision |
| `level-demo` | Level System | Lua level loading, spawn points, transitions |
| `ai-demo` | AI System | Behavior trees, pathfinding, patrol, steering |
| `camera-demo` | Camera System | Following, smoothing, bounds, shake, zoom |
| `gas-demo` | GAS | Gameplay abilities, effects, attributes, tags |
| `blueprints-demo` | Blueprints | Data-driven entity creation from Lua |

## Building

```bash
# From project root
cmake --preset macos-debug
cmake --build --preset macos-debug

# Or build specific demo
cmake --build --preset macos-debug --target entity-demo
```

## Running

```bash
# From build directory
./bin/entity-demo
./bin/events-demo
./bin/config-demo
# etc.
```

## Demo Types

### Console Demos (Non-Interactive)
Most demos print output to console demonstrating API usage:
- entity-demo, events-demo, config-demo, assets-demo
- save-demo, audio-demo, physics-demo, level-demo
- ai-demo, camera-demo, gas-demo, blueprints-demo

### Interactive Demos
Some demos require user interaction:
- `input-demo` - Opens window, responds to keyboard/mouse/controller
- `graphics-demo` - Opens window, renders visuals

## Testing

Automated pytest tests verify all demos work correctly:

```bash
cd tests
pip install -r requirements.txt
./run_tests.sh
```

See `tests/README.md` for full testing documentation.

## Directory Structure

```
system-demos/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── entity-demo/            # Entity System demo
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.cpp
│   │   └── EntityDemo.cppm
│   └── README.md
├── events-demo/            # Events System demo
│   ├── CMakeLists.txt
│   └── src/main.cpp
├── config-demo/            # Config System demo
│   ├── CMakeLists.txt
│   ├── src/main.cpp
│   └── data/               # Lua config files
│       ├── game.lua
│       └── player.lua
├── ... (other demos)
└── tests/                  # Pytest test suite
    ├── conftest.py
    ├── requirements.txt
    ├── run_tests.sh
    ├── test_all_demos.py
    └── ...
```

## API Coverage

Each demo exercises **100% of its system's interface**. Check individual README files for detailed API coverage:

### Entity System (`entity-demo/README.md`)
- Entity lifecycle: createEntity, destroyEntity, isValid, entityCount
- Components: emplace, remove, get, tryGet, allOf, anyOf
- Groups: groupCount, hasAny, first, single, collect, collectExcluding
- Queries: view, query with EntitySelector
- Iteration: each, update

### Events System
- Publishing: publish (immediate), queue (deferred)
- Subscribing: subscribe, unsubscribe, unsubscribeAll
- Processing: processQueue, clearQueue, queueSize

### Config System
- Loading: loadConfig, loadConfigAsset, reloadAll, reloadConfig
- Access: getFloat/Int/Bool/String, getFloatOr/IntOr/BoolOr/StringOr
- Arrays: getIntArray, getFloatArray, getStringArray
- Modification: setFloat/Int/Bool/String
- Queries: hasKey, getKeysWithPrefix, getLoadedConfigs, getMetadata
- Hot reload: enableHotReload, isHotReloadEnabled
- Notifications: onConfigChanged, onKeyChanged, unsubscribe

### Assets System
- Registration: registerAsset, unregisterAsset
- Loading: loadAsset, loadAssetAsync, unloadAsset
- Queries: getAssetState, getAssetMetadata, isLoaded, getRawAsset, getAsset<T>
- Bulk: loadAll, unloadAll, getAssetsOfType
- Hot reload: enableHotReload, checkForReloads, reloadAsset

### Input System
- Mapping: registerMapping, removeMapping, clearMappings, getMappings
- Actions: getActionState, getAllActionStates, isActionActive, wasActionJustPressed, wasActionJustReleased, getActionValue
- Rebinding: getLastInput, isListeningForInput, startListeningForInput, stopListeningForInput
- Mouse: getMousePosition, getMouseDelta, isMouseButtonDown
- Controller: getConnectedControllerCount, isControllerConnected, getControllerName

### Save System
- Registration: registerSaveable, unregisterSaveable
- Operations: save, load, deleteSave, quickSave, quickLoad
- Auto-save: autoSave, enableAutoSave, disableAutoSave
- Metadata: getAllSaveMetadata, getSaveMetadata, saveExists
- Profiles: setActiveProfile, getActiveProfile, getProfiles

### Graphics System
- Frame: beginFrame, endFrame
- Sprites: draw, drawBatch, drawSprite, drawAnimatedSprite
- Primitives: drawRect, drawLine, drawCircle, drawPolygon
- Text: drawText, drawTextCentered, measureText
- Camera: setCamera, getCamera, worldToScreen, screenToWorld
- Window: getWindowSize, setWindowSize, isFullscreen, setFullscreen, shouldClose
- Rendering: setClearColor, setVSync, renderEntities, setViewportCulling

### Audio System
- Channels: playOnChannel, stopChannel, pauseChannel, resumeChannel
- Properties: setChannelVolume, setChannelPitch, seekChannel
- Queries: getChannelState, isChannelPlaying
- Positional: playPositional, stopPositional, updatePositionalPosition, isPositionalPlaying
- Listener: setListener, getListener
- Global: setMasterVolume, getMasterVolume, pauseAll, resumeAll, stopAll
- Groups: setGroupVolume, assignChannelToGroup

### Physics System
- Bodies: createBody, destroyBody, hasBody
- Properties: setBodyType, getBodyType, setPosition, getPosition, setRotation, getRotation, getBodySize
- Velocity: setVelocity, getVelocity, setAngularVelocity, getAngularVelocity
- Forces: applyForce, applyImpulse, applyTorque
- Collision: setCollisionLayer, setCollisionMask, setSensor, getCollisionLayer, setCollisionCallback
- Queries: queryAABB, queryCircle, raycast, raycastAll
- World: setGravity, getGravity
- Ground: checkGrounded

### Level System
- Management: loadLevel, unloadLevel, setActiveLevel
- Transitions: transition
- Queries: getActiveLevel, getLevelState, getLevelMetadata, getLoadedLevels
- Spawn: getSpawnPoint, getSpawnPointNames
- Entities: getLevelEntities, getEntityDefs

### AI System
- Behavior: attachBehaviorTree, detachBehaviorTree, hasBehaviorTree
- Blackboard: setBehaviorTreeBlackboard, getBehaviorTreeBlackboard
- Navigation: loadNavMesh, unloadNavMesh, hasNavMesh, findPath, isPointOnNavMesh, getClosestPointOnNavMesh
- Steering: setNavigationTarget, clearNavigationTarget, getNavigationTarget, setMaxSpeed, setMaxAcceleration
- Patrol: setPatrolBehavior, clearPatrolBehavior, getPatrolBehavior
- Queries: findEntitiesInRadius, findClosestEntity, hasLineOfSight

### Camera System
- Target: setTarget, clearTarget, getTarget
- Follow: setFollowSmoothing, setOffset, setDeadzone
- Bounds: setBounds, clearBounds
- Effects: shake, stopShake
- Zoom: setZoom, getZoom
- State: update, getCamera, getPosition
- Conversion: screenToWorld, worldToScreen

### GAS (Gameplay Ability System)
- Tags: registerTag, findTag, isParentOf, addTag, removeTag, hasTag, getTags
- Attributes: registerAttribute, getAttributeDef, initializeAttribute, getAttributeValue, getAttributeBaseValue, setAttributeBaseValue, modifyAttribute
- Effects: registerEffect, getEffectDef, applyEffect, removeEffect, removeAllEffects, hasEffect, getActiveEffects
- Abilities: registerAbility, getAbilityDef, grantAbility, removeAbility, hasAbility, canActivateAbility, tryActivateAbility, endAbility, isAbilityActive, getAbilityCooldown
- Components: initializeComponent, removeComponent, hasComponent, getComponent
- Callbacks: setAttributeChangeCallback, setEffectAppliedCallback, setAbilityActivatedCallback
- Lua: loadDefinitionsFromLua

### Blueprints System
- Loading: loadBlueprints, reloadBlueprints, clearBlueprints
- Queries: hasBlueprint, getBlueprintNames, getBlueprint
- Creation: create (4 overloads with position, size, overrides)
- Registration: registerComponent, isComponentRegistered

## Contributing

When adding new system demos:
1. Create directory: `<system>-demo/`
2. Add CMakeLists.txt with proper linking
3. Create comprehensive main.cpp exercising all API methods
4. Add README.md documenting API coverage
5. Add tests in `tests/test_<system>_demo.py`
6. Update this README with the new demo
