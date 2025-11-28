# Blueprint Factory Test Fixes

## Issues Fixed

### 1. Physics System Not Initialized (All Physics Tests)
**Root Cause**: The `BlueprintFactoryTest` fixture was creating a physics system but not calling `initialize()`, causing all physics body creation to silently fail.

**Tests Affected**:
- CreateEntityWithSize
- PhysicsBodyCreation
- SensorBodyCreation
- KinematicBodyCreation
- PhysicsWithAllProperties
- PhysicsSizeOverrideWithExplicitSize
- PhysicsInheritsSizeFromDebugRect
- DebugRectSyncsToPhysicsBodySize
- DebugCircleSyncsToPhysicsBodySize
- InheritancePreservesPhysics
- ChildPhysicsOverridesParent
- DefaultPhysicsSizeUsedWhenNoSizeAvailable

**Fix**: Added initialization call in test fixture `SetUp()`:
```cpp
// Initialize physics system
auto* implPtr = dynamic_cast<Box2DPhysicsSystem*>(physicsSystem_.get());
if (implPtr) {
    implPtr->initialize();
}
```

### 2. Blueprint Property Merging Inverted (Inheritance Tests)
**Root Cause**: The `mergeBlueprints` function had backwards parameter semantics. It was iterating over parent properties and adding them to child only if child didn't have them, instead of child properties overriding parent properties.

**Tests Affected**:
- InheritanceWorks
- MultiLevelInheritance
- CreateWithSizeAndOverrides

**Fix**:
1. Renamed parameters from `child`/`parent` to `base`/`override` for clarity
2. Reversed the merging logic so override (child) properties overwrite base (parent) properties
3. Added recursive `mergeProperties()` helper function to handle nested property maps

**Code Changes**:
```cpp
void BlueprintFactory::mergeBlueprints(BlueprintDef& base, const BlueprintDef& override) const {
    // base starts as a copy of parent, override is the child
    // We want child properties to override parent properties
    base.name = override.name;

    // Merge components - child components override parent components
    for (const auto& overrideComp : override.components) {
        bool found = false;
        for (auto& baseComp : base.components) {
            if (baseComp.name == overrideComp.name) {
                // Merge properties - child properties override parent properties
                mergeProperties(baseComp.properties, overrideComp.properties);
                found = true;
                break;
            }
        }
        if (!found) {
            base.components.push_back(overrideComp);
        }
    }

    // Child physics overrides parent physics
    if (override.physics) {
        base.physics = override.physics;
    }

    // Merge metadata - child metadata overrides parent metadata
    for (const auto& [key, value] : override.metadata) {
        base.metadata[key] = value;
    }
}

void BlueprintFactory::mergeProperties(PropertyMap& base, const PropertyMap& override) const {
    for (const auto& [key, value] : override) {
        // If override has a PropertyMap (nested table), merge recursively
        if (auto* overrideMap = std::any_cast<PropertyMap>(&value)) {
            auto baseIt = base.find(key);
            if (baseIt != base.end()) {
                if (auto* baseMap = std::any_cast<PropertyMap>(&baseIt->second)) {
                    // Both are PropertyMaps, merge recursively
                    mergeProperties(*baseMap, *overrideMap);
                    continue;
                }
            }
            // Base doesn't have this property or it's not a PropertyMap, just overwrite
            base[key] = value;
        } else {
            // Simple value, just overwrite
            base[key] = value;
        }
    }
}
```

## Expected Results

After these fixes, all 15 failing tests should pass:

1. **CreateEntityWithSize** - Physics system initialized, body created with size from parameters
2. **InheritanceWorks** - Child fillColor overrides parent while preserving parent size
3. **PhysicsBodyCreation** - Physics system initialized, dynamic body created
4. **SensorBodyCreation** - Physics system initialized, sensor body created
5. **CreateWithSizeAndOverrides** - Property overrides work correctly with size parameters
6. **MultiLevelInheritance** - Three-level inheritance chain resolves correctly
7. **KinematicBodyCreation** - Physics system initialized, kinematic body created
8. **PhysicsWithAllProperties** - Physics system initialized, all properties applied
9. **PhysicsSizeOverrideWithExplicitSize** - Explicit physics size used, not DebugRect size
10. **PhysicsInheritsSizeFromDebugRect** - When no physics size given, uses DebugRect size
11. **DebugRectSyncsToPhysicsBodySize** - After physics creation, DebugRect syncs to body size
12. **DebugCircleSyncsToPhysicsBodySize** - After physics creation, DebugCircle syncs to body size
13. **InheritancePreservesPhysics** - Child without physics inherits parent physics
14. **ChildPhysicsOverridesParent** - Child physics replaces parent physics
15. **DefaultPhysicsSizeUsedWhenNoSizeAvailable** - Default 32x32 size used when no size specified

## Files Modified

1. `/Users/jaaaacob/Documents/GameDev/jframe/tests/unit/BlueprintFactoryTests.cpp`
   - Added physics system initialization in SetUp()

2. `/Users/jaaaacob/Documents/GameDev/jframe/jframe-blueprints/src/BlueprintFactory.cpp`
   - Fixed mergeBlueprints() logic
   - Added mergeProperties() helper function

3. `/Users/jaaaacob/Documents/GameDev/jframe/jframe-blueprints/src/jframe.blueprints.impl.cppm`
   - Updated method signatures
   - Added mergeProperties() declaration

## Verification

To verify these fixes work, run:
```bash
cd /Users/jaaaacob/Documents/GameDev/jframe
cmake --build --preset macos-debug
ctest --preset macos-debug -R BlueprintFactoryTest --output-on-failure
```

All 49 BlueprintFactory tests should pass.
