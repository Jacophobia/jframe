# Bestow V2 Contract Specification

This directory contains the complete V2 API contract specification, split into individual per-system documents for easy navigation and independent agent work.

## Document Index

### Foundation
- [`DEPENDENCY-DIAGRAM.md`](DEPENDENCY-DIAGRAM.md) — System relationships, visibility tiers, dependency graph
- [`SHARED-PATTERNS.md`](SHARED-PATTERNS.md) — Strong handles, error types, callback patterns, update phases

### Protected Systems (C++ peer-only, not in Lua API)
- [`protected/assets.md`](protected/assets.md) — File system gateway, caching, hot reload
- [`protected/shader.md`](protected/shader.md) — GLSL/SPIR-V compilation pipeline
- [`protected/graphics-context.md`](protected/graphics-context.md) — Base interface for 2D/3D renderers
- [`protected/ui-render-backend.md`](protected/ui-render-backend.md) — GPU primitives for UI
- [`protected/blueprints.md`](protected/blueprints.md) — Entity template factory
- [`protected/metrics.md`](protected/metrics.md) — Tracy profiling integration

### Public Systems (Lua + C++ API, dual-level)
- [`public/entity.md`](public/entity.md) — ECS: entities, components, hierarchy, queries
- [`public/events.md`](public/events.md) — Pub/sub event bus (single-level)
- [`public/input.md`](public/input.md) — Actions, phases, keyboard/mouse/gamepad
- [`public/audio.md`](public/audio.md) — Sound playback, music, 3D audio, mixing
- [`public/physics2d.md`](public/physics2d.md) — 2D rigid bodies, shapes, joints, queries
- [`public/physics3d.md`](public/physics3d.md) — 3D rigid bodies, character controller, vehicles
- [`public/graphics2d.md`](public/graphics2d.md) — Sprites, primitives, text, camera
- [`public/graphics3d.md`](public/graphics3d.md) — Meshes, materials, lighting, environment
- [`public/animation.md`](public/animation.md) — Skeletal animation, blending, IK, state machines
- [`public/camera.md`](public/camera.md) — 2D follow, 3D orbit/FPS/third-person, effects
- [`public/scene.md`](public/scene.md) — Stack-based scene management
- [`public/state.md`](public/state.md) — Save/load, key-value persistence, profiles
- [`public/config.md`](public/config.md) — Lua configuration files
- [`public/ui.md`](public/ui.md) — Documents, elements, data binding, events
- [`public/ai.md`](public/ai.md) — Behavior trees, navigation, steering, perception
- [`public/gas.md`](public/gas.md) — Gameplay tags, attributes, effects, abilities
- [`public/gamestate.md`](public/gamestate.md) — State machine for game modes (single-level)
- [`public/tween.md`](public/tween.md) — Value interpolation, easing, sequences (NEW)
- [`public/particles.md`](public/particles.md) — CPU/GPU particle emitters (NEW)
- [`public/network.md`](public/network.md) — Client-server multiplayer, replication (NEW)

## Guiding Principles

| Principle | Applies To | Meaning |
|-----------|-----------|---------|
| **Usability** | High-Level API | Common tasks in 1-3 calls |
| **Configurability** | Low-Level API | Every knob exposed |
| **Abstraction** | Both | Contracts define interaction, not implementation |
