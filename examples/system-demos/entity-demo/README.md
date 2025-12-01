# Entity System Demo

Comprehensive demonstration of the JFrame Entity System API using EnTT.

## Overview

This demo exercises **every API method** from `IEntitySystem` to showcase the full capabilities of JFrame's ECS (Entity Component System).

## Building

```bash
cmake --build --preset macos-debug --target entity-demo
```

## Running

```bash
./build/macos-debug/bin/entity-demo
```

## API Coverage

### Entity Lifecycle
- `createEntity()` - Create new entities
- `destroyEntity()` - Remove entities from the world
- `isValid()` - Check if entity handle is still valid
- `entityCount()` - Count total entities

### Component Access
- `emplace<T>()` - Add components with construction arguments
- `get<T>()` - Retrieve component references
- `tryGet<T>()` - Safe component access (returns nullptr if missing)
- `remove<T>()` - Remove components from entities

### Component Queries
- `allOf<T...>()` - Check if entity has all specified components
- `anyOf<T...>()` - Check if entity has any of the specified components

### Entity Groups
- `groupCount<T...>()` - Count entities matching component criteria
- `hasAny<T...>()` - Check if any entities exist with components
- `first<T...>()` - Get first matching entity (or nullopt)
- `single<T...>()` - Get single matching entity (or nullopt if 0 or 2+)
- `collect<T...>()` - Gather all matching entities into a vector
- `collectExcluding<Include, Exclude...>()` - Collect with exclusion filters

### Views and Iteration
- `view<T...>()` - Create EnTT views for efficient iteration
- `each()` - Iterate all entities with callback
- `update()` - Per-frame update hook

### Advanced Queries
- `query(EntitySelector)` - Custom predicate-based entity filtering
- `getRegistry()` - Direct access to underlying EnTT registry

## Sample Components

The demo defines several example components:

**Data Components:**
- `Position` - 2D position (x, y)
- `Velocity` - 2D velocity (dx, dy)
- `Health` - Current and max health
- `Name` - String identifier

**Tag Components:**
- `PlayerTag` - Marks player entities
- `EnemyTag` - Marks enemy entities
- `ProjectileTag` - Marks projectile entities
- `DeadTag` - Marks entities for removal

## Demo Sections

1. **Entity Lifecycle** - Creation, validation, destruction
2. **Component Access** - Adding, getting, removing components
3. **Component Queries** - Testing allOf/anyOf predicates
4. **Entity Groups** - Counting, finding, collecting entities
5. **View Iteration** - Efficient component-based iteration
6. **Entity Selectors** - Custom queries with predicates
7. **Each Iteration** - Global entity iteration
8. **Update** - Frame update demonstration
9. **Complex Scenario** - Mini game simulation

## Key Takeaways

- **Type Safety**: All component access is compile-time type-checked
- **Performance**: EnTT views provide cache-friendly iteration
- **Flexibility**: Entity selectors support arbitrary predicates
- **Safety**: `tryGet<T>()` prevents crashes from missing components
- **Convenience**: Helper methods like `first<T>()`, `single<T>()`, `collect<T>()` simplify common patterns

## Notes

- Tag components require at least one member due to EnTT requirements
- Type-erased component access (`addComponent`, `getComponent`) is not fully implemented - use typed methods instead
- Entity selectors work best with predicates; type-erased component filtering is partially implemented
