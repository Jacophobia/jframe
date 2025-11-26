# JFrame TODO

> Last Updated: 2025-11-25

## High Priority

### AI System - Recast/Detour Integration
- [ ] Add Recast/Detour to vcpkg.json or build from source
- [ ] Implement `loadNavMesh()` to parse binary navmesh format
- [ ] Implement `findPath()` with actual Detour pathfinding
- [ ] Implement `isPointOnNavMesh()` with proper navmesh query
- [ ] Implement `getClosestPointOnNavMesh()` with Detour projection

### AI System - Physics Integration
- [ ] Connect `hasLineOfSight()` to `IPhysicsSystem::raycast()`
- [ ] Connect `findEntitiesInRadius()` to physics AABB queries
- [ ] Connect `findClosestEntity()` to physics spatial queries

## Medium Priority

### Core Engine
- [ ] Implement Application composition root with Fruit DI
- [ ] Create main game loop with proper frame timing
- [ ] Wire up all systems with dependency injection

### Platformer Example
- [ ] Implement player movement system
- [ ] Implement camera follow system
- [ ] Load levels from Lua files
- [ ] Create basic gameplay loop

### Audio-Asset Integration
- [ ] Connect audio system to asset system for path resolution
- [ ] Support AssetHandle-based audio loading

## Low Priority

### Enhancements
- [ ] Audio: FMOD fade-out using DSP
- [ ] Graphics: Custom font loading via AssetSystem
- [ ] Level: Store entity definitions for spawning
- [ ] Save: Track game version in metadata
- [ ] Save: Track playtime in metadata
- [ ] Save: Track completion percentage

### Dev Tools (jframe-dev)
- [ ] Hot reload file watcher (efsw)
- [ ] Debug overlay (ImGui backend)
- [ ] Profiler integration (Tracy)

## Completed

- [x] Events System (100%)
- [x] Entity System (100%)
- [x] Input System (100%)
- [x] Physics System (100%)
- [x] Audio System (100%)
- [x] Graphics System (100%)
- [x] Assets System (100% - all 9 types)
- [x] Save System (100%)
- [x] Level System (100%)
- [x] AI System (95% - navmesh stubbed)

## Notes

- Graphics tests are disabled (require display context)
- AI navmesh uses distance-based heuristics as fallback
- All 378 enabled tests pass (100% pass rate)
