# Particle System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 5
> **Dependencies:** Types, Entity, Graphics3D
> **Lua Paths:** `bestow.particles` (high-level), `bestow.particles.core` (low-level)

## Purpose

The Particle System provides CPU and GPU-accelerated particle emission, simulation, and rendering for visual effects such as fire, smoke, sparks, explosions, weather, and ambient particles. Emitters are configured through definition structs or Lua preset files, and can be attached to entities for automatic position tracking. The high-level API covers creating, playing, stopping, and positioning emitters with presets, while the low-level API exposes emission shapes, particle property overrides, sub-emitters, GPU simulation control, material configuration, and preset serialization. This is a new system in Bestow V2.

## High-Level API: `IParticleSystem`

The simplified particle interface for common visual effects. Position-based emitter creation, preset loading, basic play/stop control, and entity attachment. No lifecycle methods -- the engine calls `update()` and `render()` internally.

### Emitter Creation

| Method | Returns | Description |
|--------|---------|-------------|
| `createEmitter(Vec3 position, const EmitterDef& def)` | `Result<EmitterHandle>` | Create a new particle emitter at the given world position using the provided definition |
| `loadPreset(std::string_view presetPath, Vec3 position)` | `Result<EmitterHandle>` | Load a particle preset from a Lua file and create an emitter at the given position |

### Emitter Control

| Method | Returns | Description |
|--------|---------|-------------|
| `play(EmitterHandle h)` | `Result<void>` | Start or resume particle emission on the emitter |
| `stop(EmitterHandle h)` | `Result<void>` | Stop particle emission; existing particles continue to live out their lifetimes |
| `destroy(EmitterHandle h)` | `Result<void>` | Destroy the emitter and all its particles immediately |

### Emitter Transform

| Method | Returns | Description |
|--------|---------|-------------|
| `setPosition(EmitterHandle h, Vec3 position)` | `Result<void>` | Move the emitter to a new world-space position |
| `attachToEntity(EmitterHandle h, Entity entity)` | `Result<void>` | Attach the emitter to an entity so it automatically follows the entity's position |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getTotalParticleCount()` | `int` | Return the total number of active particles across all emitters |

## Low-Level API: `IParticleCore`

Full control API. Exposes the complete particle simulation pipeline including emitter lifecycle, transform, emission properties, shapes, materials, sub-emitters, GPU simulation, presets, and per-emitter queries.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Advance all particle simulations by the given delta time, spawning, moving, aging, and killing particles |
| `render()` | `void` | Submit all active particle draw calls to the rendering pipeline |

### Emitter Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createEmitter(const EmitterDef& def)` | `Result<EmitterHandle>` | Create a new emitter from a definition; position is set via the def or subsequent calls |
| `destroyEmitter(EmitterHandle handle)` | `Result<void>` | Destroy the emitter and immediately remove all its particles |

### Emitter Control

| Method | Returns | Description |
|--------|---------|-------------|
| `play(EmitterHandle h)` | `Result<void>` | Start or resume particle emission |
| `stop(EmitterHandle h)` | `Result<void>` | Stop emission; living particles continue until they expire |
| `pause(EmitterHandle h)` | `Result<void>` | Pause both emission and simulation; all particles freeze in place |
| `restart(EmitterHandle h)` | `Result<void>` | Reset the emitter to its initial state and start playing from the beginning |
| `isPlaying(EmitterHandle h)` | `bool` | Check whether the emitter is currently emitting particles |

### Emitter Transform

| Method | Returns | Description |
|--------|---------|-------------|
| `setPosition(EmitterHandle h, Vec3 position)` | `Result<void>` | Set the emitter's world-space position |
| `setRotation(EmitterHandle h, Quat rotation)` | `Result<void>` | Set the emitter's world-space rotation, affecting emission direction |
| `setScale(EmitterHandle h, float scale)` | `Result<void>` | Set a uniform scale factor for the emitter's emission shape and particle sizes |
| `attachToEntity(EmitterHandle h, Entity entity)` | `Result<void>` | Attach the emitter to an entity for automatic position/rotation tracking |
| `detachFromEntity(EmitterHandle h)` | `Result<void>` | Detach the emitter from its entity, keeping its current world position |

### Emitter Properties (Runtime Modification)

| Method | Returns | Description |
|--------|---------|-------------|
| `setEmissionRate(EmitterHandle h, float rate)` | `Result<void>` | Set the number of particles emitted per second |
| `setMaxParticles(EmitterHandle h, int max)` | `Result<void>` | Set the maximum number of simultaneous alive particles for this emitter |
| `setParticleLifetime(EmitterHandle h, float min, float max)` | `Result<void>` | Set the min and max lifetime range for newly spawned particles in seconds |
| `setParticleSpeed(EmitterHandle h, float min, float max)` | `Result<void>` | Set the min and max initial speed range for newly spawned particles |
| `setParticleSize(EmitterHandle h, float startMin, float startMax, float endMin, float endMax)` | `Result<void>` | Set the size range at birth and at death; particles interpolate between start and end |
| `setParticleColor(EmitterHandle h, Color start, Color end)` | `Result<void>` | Set the color at birth and at death; particles interpolate between the two colors |
| `setGravity(EmitterHandle h, Vec3 gravity)` | `Result<void>` | Set the gravity vector applied to all particles in this emitter |

### Emission Shapes

| Method | Returns | Description |
|--------|---------|-------------|
| `setEmissionShape(EmitterHandle h, EmissionShape shape)` | `Result<void>` | Set the shape from which new particles are spawned |

### Material

| Method | Returns | Description |
|--------|---------|-------------|
| `setParticleTexture(EmitterHandle h, TextureHandle texture)` | `Result<void>` | Set the texture used for rendering particles in this emitter |
| `setBlendMode(EmitterHandle h, BlendMode mode)` | `Result<void>` | Set the blend mode for particle rendering (Additive, Alpha, etc.) |

### Sub-Emitters

| Method | Returns | Description |
|--------|---------|-------------|
| `addSubEmitter(EmitterHandle parent, const EmitterDef& child, SubEmitterTrigger trigger)` | `Result<void>` | Add a sub-emitter that spawns automatically when the trigger condition occurs on parent particles |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getParticleCount(EmitterHandle h)` | `int` | Return the number of active particles for a specific emitter |
| `getTotalParticleCount()` | `int` | Return the total number of active particles across all emitters |
| `getBounds(EmitterHandle h)` | `AABB3D` | Return the axis-aligned bounding box enclosing all particles of the emitter |

### GPU Particles

| Method | Returns | Description |
|--------|---------|-------------|
| `isGPUSimulationSupported()` | `bool` | Check whether the current graphics backend supports GPU-accelerated particle simulation |
| `setGPUSimulation(EmitterHandle h, bool enabled)` | `Result<void>` | Enable or disable GPU-accelerated simulation for the emitter; falls back to CPU if unsupported |

### Presets

| Method | Returns | Description |
|--------|---------|-------------|
| `loadPreset(AssetHandle luaPreset)` | `Result<EmitterHandle>` | Create an emitter from a Lua preset asset loaded through the asset system |
| `savePreset(EmitterHandle h, std::string_view path)` | `Result<void>` | Serialize the current emitter configuration to a Lua file at the given path |

## Types

### EmitterDef

Complete definition for creating a particle emitter. All fields have sensible defaults for immediate use.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `emissionRate` | `float` | `10.0f` | Number of particles emitted per second |
| `maxParticles` | `int` | `1000` | Maximum number of simultaneous alive particles |
| `duration` | `float` | `0.0f` | Total emission duration in seconds; 0 means infinite emission |
| `playOnCreate` | `bool` | `true` | Whether the emitter starts playing immediately upon creation |
| `lifetimeMin` | `float` | `1.0f` | Minimum lifetime of a spawned particle in seconds |
| `lifetimeMax` | `float` | `2.0f` | Maximum lifetime of a spawned particle in seconds |
| `speedMin` | `float` | `1.0f` | Minimum initial speed of a spawned particle |
| `speedMax` | `float` | `5.0f` | Maximum initial speed of a spawned particle |
| `sizeStart` | `float` | `1.0f` | Particle size at birth |
| `sizeEnd` | `float` | `0.0f` | Particle size at death; interpolated over lifetime |
| `colorStart` | `Color` | `Color::white()` | Particle color at birth |
| `colorEnd` | `Color` | `{1, 1, 1, 0}` | Particle color at death; alpha fades to 0 by default |
| `rotationSpeedMin` | `float` | `0.0f` | Minimum rotation speed of particles in radians per second |
| `rotationSpeedMax` | `float` | `0.0f` | Maximum rotation speed of particles in radians per second |
| `gravity` | `Vec3` | `{0, -9.81f, 0}` | Gravity vector applied to all particles |
| `drag` | `float` | `0.0f` | Drag coefficient that slows particles over time; 0 means no drag |
| `shape` | `EmissionShape` | `Point` | The emission shape from which particles are spawned |
| `blendMode` | `BlendMode` | `Additive` | Blend mode for particle rendering |
| `sortByDepth` | `bool` | `false` | Whether to sort particles by distance to camera for correct alpha blending |
| `billboard` | `bool` | `true` | Whether particles always face the camera |

### EmitterHandle

A strong typed handle referencing an active particle emitter.

```cpp
using EmitterHandle = Handle<struct EmitterTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Internal identifier; 0 means invalid/no emitter |

### EmissionShape

Defines the spatial region from which particles are spawned.

```cpp
struct EmissionShape {
    enum class Type : std::uint8_t {
        Point, Sphere, Hemisphere, Cone, Box, Circle, Ring, Edge
    };
    Type type = Type::Point;
    float radius = 1.0f;
    float angle = 45.0f;
    Vec3 extents = {1, 1, 1};
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `EmissionShape::Type` | `Point` | The geometric shape used for spawning |
| `radius` | `float` | `1.0f` | Radius for Sphere, Hemisphere, Circle, Ring, and Cone shapes |
| `angle` | `float` | `45.0f` | Half-angle in degrees for Cone shape |
| `extents` | `Vec3` | `{1, 1, 1}` | Half-extents for Box shape |

### EmissionShape::Type

| Value | Description |
|-------|-------------|
| `Point` | All particles spawn at the emitter origin |
| `Sphere` | Particles spawn at random points on the surface or inside a sphere |
| `Hemisphere` | Particles spawn at random points on the upper half of a sphere |
| `Cone` | Particles spawn at the apex of a cone and travel outward within the cone angle |
| `Box` | Particles spawn at random points within a 3D box defined by extents |
| `Circle` | Particles spawn at random points on the circumference or area of a circle (XZ plane) |
| `Ring` | Particles spawn at random points on the circumference of a circle (not the interior) |
| `Edge` | Particles spawn along a line segment defined by the extents.x dimension |

### SubEmitterTrigger

Enum defining when a sub-emitter should activate.

```cpp
enum class SubEmitterTrigger : std::uint8_t {
    OnBirth, OnDeath, OnCollision
};
```

| Value | Description |
|-------|-------------|
| `OnBirth` | The sub-emitter fires a burst when a parent particle is born |
| `OnDeath` | The sub-emitter fires a burst when a parent particle dies |
| `OnCollision` | The sub-emitter fires a burst when a parent particle collides with geometry |

### BlendMode

Blend mode used for particle rendering.

```cpp
enum class BlendMode : std::uint8_t {
    Opaque, Alpha, Additive, Multiply, PreMultipliedAlpha
};
```

| Value | Description |
|-------|-------------|
| `Opaque` | No transparency; particles are fully opaque |
| `Alpha` | Standard alpha blending; requires depth sorting for correctness |
| `Additive` | Additive blending; particles add to the background color (fire, sparks, glow) |
| `Multiply` | Multiplicative blending; darkens the background (shadows, dust) |
| `PreMultipliedAlpha` | Pre-multiplied alpha blending; avoids fringing artifacts at transparency edges |

## Lua Examples

```lua
-- High-level: Create a fire effect
local fire, err = bestow.particles.createEmitter(
    { x = 0, y = 0, z = 0 },
    {
        emissionRate = 50,
        maxParticles = 500,
        lifetimeMin = 0.5,
        lifetimeMax = 1.5,
        speedMin = 2,
        speedMax = 5,
        sizeStart = 0.5,
        sizeEnd = 0.0,
        colorStart = { r = 1, g = 0.8, b = 0.2, a = 1 },
        colorEnd = { r = 1, g = 0, b = 0, a = 0 },
        gravity = { x = 0, y = 2, z = 0 },
    }
)

-- Load a preset
local explosion = bestow.particles.loadPreset("particles/explosion.lua",
    { x = 10, y = 0, z = 5 })

-- Control
bestow.particles.play(fire)
bestow.particles.stop(fire)
bestow.particles.setPosition(fire, { x = 5, y = 0, z = 5 })
bestow.particles.attachToEntity(fire, torch)
bestow.particles.destroy(fire)

print("Total particles: " .. bestow.particles.getTotalParticleCount())

-- Low-level: Full emitter configuration
local emitter = bestow.particles.core.createEmitter({
    emissionRate = 100,
    maxParticles = 2000,
    duration = 3.0,
    playOnCreate = false,
})

bestow.particles.core.setEmissionShape(emitter, {
    type = "Cone",
    radius = 0.5,
    angle = 30,
})

bestow.particles.core.setParticleLifetime(emitter, 0.5, 2.0)
bestow.particles.core.setParticleSpeed(emitter, 3, 8)
bestow.particles.core.setParticleSize(emitter, 0.3, 0.8, 0.0, 0.1)
bestow.particles.core.setParticleColor(
    emitter,
    { r = 1, g = 1, b = 1, a = 1 },
    { r = 0.5, g = 0.5, b = 0.5, a = 0 }
)
bestow.particles.core.setGravity(emitter, { x = 0, y = -4.9, z = 0 })

-- Sub-emitter for sparks on death
bestow.particles.core.addSubEmitter(emitter, {
    emissionRate = 5,
    maxParticles = 20,
    lifetimeMin = 0.1,
    lifetimeMax = 0.3,
    speedMin = 1,
    speedMax = 3,
}, "OnDeath")

-- GPU particles
if bestow.particles.core.isGPUSimulationSupported() then
    bestow.particles.core.setGPUSimulation(emitter, true)
end

bestow.particles.core.play(emitter)

local count = bestow.particles.core.getParticleCount(emitter)
local bounds = bestow.particles.core.getBounds(emitter)

-- Save configuration as preset
bestow.particles.core.savePreset(emitter, "particles/custom_fire.lua")
```

## C++ Examples

```cpp
// High-level: Create and control emitters
EmitterDef fireDef{
    .emissionRate = 50.0f,
    .maxParticles = 500,
    .lifetimeMin = 0.5f, .lifetimeMax = 1.5f,
    .speedMin = 2.0f, .speedMax = 5.0f,
    .sizeStart = 0.5f, .sizeEnd = 0.0f,
    .colorStart = {1, 0.8f, 0.2f, 1},
    .colorEnd = {1, 0, 0, 0},
    .gravity = {0, 2, 0}
};

auto fireResult = particle->createEmitter(Vec3{0, 0, 0}, fireDef);
if (fireResult) {
    EmitterHandle fire = fireResult.value();
    particle->play(fire);
    particle->attachToEntity(fire, torchEntity);
}

auto explosion = particle->loadPreset("particles/explosion.lua", Vec3{10, 0, 5});

int totalParticles = particle->getTotalParticleCount();

// Low-level: Full configuration
EmitterDef sparkDef{
    .emissionRate = 100.0f,
    .maxParticles = 2000,
    .duration = 3.0f,
    .playOnCreate = false
};

auto emitterResult = particleCore->createEmitter(sparkDef);
if (emitterResult) {
    EmitterHandle emitter = emitterResult.value();

    particleCore->setEmissionShape(emitter, {
        .type = EmissionShape::Type::Cone,
        .radius = 0.5f,
        .angle = 30.0f
    });

    particleCore->setParticleLifetime(emitter, 0.5f, 2.0f);
    particleCore->setParticleSpeed(emitter, 3.0f, 8.0f);
    particleCore->setParticleSize(emitter, 0.3f, 0.8f, 0.0f, 0.1f);
    particleCore->setParticleColor(emitter, Color::white(), Color{0.5f, 0.5f, 0.5f, 0});
    particleCore->setGravity(emitter, Vec3{0, -4.9f, 0});

    // Sub-emitter
    EmitterDef sparkSubDef{
        .emissionRate = 5.0f,
        .maxParticles = 20,
        .lifetimeMin = 0.1f, .lifetimeMax = 0.3f,
        .speedMin = 1.0f, .speedMax = 3.0f
    };
    particleCore->addSubEmitter(emitter, sparkSubDef, SubEmitterTrigger::OnDeath);

    // GPU simulation
    if (particleCore->isGPUSimulationSupported()) {
        particleCore->setGPUSimulation(emitter, true);
    }

    particleCore->setRotation(emitter, Quat{0, 0, 0, 1});
    particleCore->setScale(emitter, 2.0f);
    particleCore->attachToEntity(emitter, torchEntity);

    particleCore->play(emitter);

    int count = particleCore->getParticleCount(emitter);
    AABB3D bounds = particleCore->getBounds(emitter);

    particleCore->savePreset(emitter, "particles/custom_fire.lua");
}
```
