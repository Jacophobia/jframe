# Tween System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 2
> **Dependencies:** Types
> **Lua Paths:** `bestow.tween` (high-level), `bestow.tween.core` (low-level)

## Purpose

The Tween System provides value interpolation over time for animation, UI transitions, and procedural effects. It supports tweening floats, 2D/3D vectors, and colors with a comprehensive set of easing functions. The high-level API offers fire-and-forget one-liner tweens with sensible defaults, while the low-level API exposes the full range of tween configuration including delays, repeats, yoyo, custom easing curves, completion callbacks, sequencing, and bulk control. This is a new system in Bestow V2.

## High-Level API: `ITweenSystem`

The simplified tween interface for common interpolation tasks. One-liner tween creation with default OutQuad easing, simple cancel and query methods. No lifecycle methods -- the engine calls `update()` internally.

### Tween Creation

| Method | Returns | Description |
|--------|---------|-------------|
| `to(float from, float to, float duration, std::function<void(float)> setter, EasingType easing)` | `TweenHandle` | Create a float tween that calls the setter each frame with the interpolated value |
| `toVec2(Vec2 from, Vec2 to, float duration, std::function<void(Vec2)> setter, EasingType easing)` | `TweenHandle` | Create a Vec2 tween that interpolates both components and calls the setter each frame |
| `toVec3(Vec3 from, Vec3 to, float duration, std::function<void(Vec3)> setter, EasingType easing)` | `TweenHandle` | Create a Vec3 tween that interpolates all three components and calls the setter each frame |
| `toColor(Color from, Color to, float duration, std::function<void(Color)> setter, EasingType easing)` | `TweenHandle` | Create a Color tween that interpolates all RGBA channels and calls the setter each frame |

### Control

| Method | Returns | Description |
|--------|---------|-------------|
| `cancel(TweenHandle h)` | `Result<void>` | Cancel an active tween, stopping it at its current value |
| `isActive(TweenHandle h)` | `bool` | Check whether a tween is still running |
| `cancelAll()` | `void` | Cancel all active tweens across the entire system |

## Low-Level API: `ITweenCore`

Full control API. Exposes tween lifecycle, creation with typed constructors, easing configuration, chaining (delay, repeat, yoyo, callbacks), individual tween control, bulk operations, and sequence building.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Advance all active tweens by the given delta time, invoking setters and callbacks |

### Tween Creation

| Method | Returns | Description |
|--------|---------|-------------|
| `tweenFloat(float from, float to, float duration, std::function<void(float)> setter)` | `TweenHandle` | Create a float tween with no easing (Linear by default); configure easing separately |
| `tweenVec2(Vec2 from, Vec2 to, float duration, std::function<void(Vec2)> setter)` | `TweenHandle` | Create a Vec2 tween with Linear easing by default |
| `tweenVec3(Vec3 from, Vec3 to, float duration, std::function<void(Vec3)> setter)` | `TweenHandle` | Create a Vec3 tween with Linear easing by default |
| `tweenColor(Color from, Color to, float duration, std::function<void(Color)> setter)` | `TweenHandle` | Create a Color tween with Linear easing by default |

### Easing

| Method | Returns | Description |
|--------|---------|-------------|
| `setEasing(TweenHandle h, EasingType easing)` | `Result<void>` | Set the easing function for a tween from the built-in easing library |
| `setCustomEasing(TweenHandle h, std::function<float(float)> curve)` | `Result<void>` | Set a custom easing curve function that maps t in [0,1] to an output value |

### Chaining

| Method | Returns | Description |
|--------|---------|-------------|
| `setDelay(TweenHandle h, float delay)` | `Result<void>` | Set a delay in seconds before the tween begins interpolating |
| `setRepeat(TweenHandle h, int count)` | `Result<void>` | Set the number of times the tween repeats; -1 for infinite repetition |
| `setYoyo(TweenHandle h, bool yoyo)` | `Result<void>` | Enable yoyo mode where the tween reverses direction on each repeat |
| `onComplete(TweenHandle h, std::function<void()> cb)` | `Result<void>` | Set a callback invoked when the tween finishes all repetitions |
| `onStep(TweenHandle h, std::function<void(float)> cb)` | `Result<void>` | Set a callback invoked each frame with the current normalized progress (0.0 to 1.0) |

### Control

| Method | Returns | Description |
|--------|---------|-------------|
| `pause(TweenHandle h)` | `Result<void>` | Pause an active tween, freezing it at its current value |
| `resume(TweenHandle h)` | `Result<void>` | Resume a paused tween from where it was frozen |
| `cancel(TweenHandle h)` | `Result<void>` | Cancel an active tween, stopping it at its current value without invoking onComplete |
| `complete(TweenHandle h)` | `Result<void>` | Immediately jump a tween to its end value and invoke onComplete |
| `isActive(TweenHandle h)` | `bool` | Check whether a tween is still running (not completed, cancelled, or never created) |
| `getProgress(TweenHandle h)` | `float` | Return the current normalized progress of the tween (0.0 to 1.0) |

### Bulk Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `pauseAll()` | `void` | Pause all active tweens |
| `resumeAll()` | `void` | Resume all paused tweens |
| `cancelAll()` | `void` | Cancel all active tweens |
| `activeCount()` | `int` | Return the number of currently active tweens |

### Sequences

| Method | Returns | Description |
|--------|---------|-------------|
| `createSequence()` | `TweenHandle` | Create an empty tween sequence that plays its children in order |
| `appendToSequence(TweenHandle seq, TweenHandle tween)` | `Result<void>` | Append a tween to the end of a sequence; it will play after all previously appended tweens finish |
| `insertInSequence(TweenHandle seq, float atTime, TweenHandle tween)` | `Result<void>` | Insert a tween into a sequence at a specific time offset in seconds from the sequence start |

## Types

### TweenHandle

A strong typed handle referencing an active tween or sequence.

```cpp
using TweenHandle = Handle<struct TweenTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Internal identifier; 0 means invalid/no tween |

### EasingType

Enum of all built-in easing functions. Each In/Out/InOut variant is available for the standard curves, plus a Spring ease.

```cpp
enum class EasingType : std::uint8_t {
    Linear,
    InQuad, OutQuad, InOutQuad,
    InCubic, OutCubic, InOutCubic,
    InQuart, OutQuart, InOutQuart,
    InQuint, OutQuint, InOutQuint,
    InSine, OutSine, InOutSine,
    InExpo, OutExpo, InOutExpo,
    InCirc, OutCirc, InOutCirc,
    InElastic, OutElastic, InOutElastic,
    InBack, OutBack, InOutBack,
    InBounce, OutBounce, InOutBounce,
    Spring
};
```

| Value | Description |
|-------|-------------|
| `Linear` | Constant speed interpolation with no acceleration |
| `InQuad` | Quadratic ease-in: starts slow, accelerates |
| `OutQuad` | Quadratic ease-out: starts fast, decelerates (default for high-level API) |
| `InOutQuad` | Quadratic ease-in-out: slow start and end, fast middle |
| `InCubic` | Cubic ease-in: stronger acceleration than quadratic |
| `OutCubic` | Cubic ease-out: stronger deceleration than quadratic |
| `InOutCubic` | Cubic ease-in-out |
| `InQuart` | Quartic ease-in |
| `OutQuart` | Quartic ease-out |
| `InOutQuart` | Quartic ease-in-out |
| `InQuint` | Quintic ease-in |
| `OutQuint` | Quintic ease-out |
| `InOutQuint` | Quintic ease-in-out |
| `InSine` | Sinusoidal ease-in |
| `OutSine` | Sinusoidal ease-out |
| `InOutSine` | Sinusoidal ease-in-out |
| `InExpo` | Exponential ease-in: very slow start, explosive acceleration |
| `OutExpo` | Exponential ease-out: explosive start, slow finish |
| `InOutExpo` | Exponential ease-in-out |
| `InCirc` | Circular ease-in |
| `OutCirc` | Circular ease-out |
| `InOutCirc` | Circular ease-in-out |
| `InElastic` | Elastic ease-in: oscillates before reaching start |
| `OutElastic` | Elastic ease-out: oscillates past end before settling |
| `InOutElastic` | Elastic ease-in-out: oscillates on both ends |
| `InBack` | Back ease-in: pulls back slightly before moving forward |
| `OutBack` | Back ease-out: overshoots the target before settling |
| `InOutBack` | Back ease-in-out: pullback and overshoot |
| `InBounce` | Bounce ease-in: bounces at the start |
| `OutBounce` | Bounce ease-out: bounces at the end like a dropped ball |
| `InOutBounce` | Bounce ease-in-out: bounces on both ends |
| `Spring` | Physically-based spring interpolation with slight overshoot and dampening |

## Lua Examples

```lua
-- High-level: Fire-and-forget tweens
local handle = bestow.tween.to(0, 100, 2.0, function(v)
    bestow.entity.setField(player, "Transform2D", "x", v)
end)  -- Default OutQuad easing

bestow.tween.toVec2(
    { x = 0, y = 0 },
    { x = 100, y = 200 },
    1.5,
    function(v)
        bestow.entity.setField(player, "Transform2D", "x", v.x)
        bestow.entity.setField(player, "Transform2D", "y", v.y)
    end
)

bestow.tween.toColor(
    { r = 1, g = 1, b = 1, a = 1 },
    { r = 1, g = 0, b = 0, a = 0.5 },
    0.5,
    function(c)
        -- Apply color to sprite
    end,
    "InOutSine"
)

if bestow.tween.isActive(handle) then
    bestow.tween.cancel(handle)
end

bestow.tween.cancelAll()

-- Low-level: Full tween configuration
local t = bestow.tween.core.tweenFloat(0, 360, 3.0, function(v)
    -- Rotate something
end)
bestow.tween.core.setEasing(t, "InOutCubic")
bestow.tween.core.setDelay(t, 0.5)
bestow.tween.core.setRepeat(t, -1)  -- Infinite
bestow.tween.core.setYoyo(t, true)
bestow.tween.core.onComplete(t, function()
    print("Tween finished!")
end)
bestow.tween.core.onStep(t, function(progress)
    print("Progress: " .. progress)
end)

-- Control
bestow.tween.core.pause(t)
bestow.tween.core.resume(t)
local progress = bestow.tween.core.getProgress(t)
bestow.tween.core.complete(t)  -- Jump to end

-- Sequences
local seq = bestow.tween.core.createSequence()
local moveRight = bestow.tween.core.tweenFloat(0, 100, 1.0, function(v) end)
local moveUp = bestow.tween.core.tweenFloat(0, 200, 0.5, function(v) end)
bestow.tween.core.appendToSequence(seq, moveRight)
bestow.tween.core.appendToSequence(seq, moveUp)

-- Insert a tween at a specific time in the sequence
local flash = bestow.tween.core.tweenFloat(0, 1, 0.2, function(v) end)
bestow.tween.core.insertInSequence(seq, 0.5, flash)

-- Bulk operations
print("Active tweens: " .. bestow.tween.core.activeCount())
bestow.tween.core.pauseAll()
bestow.tween.core.resumeAll()
```

## C++ Examples

```cpp
// High-level: Simple tweens
auto handle = tween->to(0.0f, 100.0f, 2.0f,
    [&](float v) { position.x = v; },
    EasingType::OutQuad);

tween->toVec3(Vec3{0, 0, 0}, Vec3{10, 20, 30}, 1.5f,
    [&](Vec3 v) { transform.position = v; },
    EasingType::InOutCubic);

tween->toColor(Color::white(), Color::red(), 0.5f,
    [&](Color c) { sprite.tint = c; });

if (tween->isActive(handle)) {
    tween->cancel(handle);
}

tween->cancelAll();

// Low-level: Full configuration
auto t = tweenCore->tweenFloat(0.0f, 360.0f, 3.0f,
    [&](float v) { rotation = v; });
tweenCore->setEasing(t, EasingType::InOutCubic);
tweenCore->setDelay(t, 0.5f);
tweenCore->setRepeat(t, -1);
tweenCore->setYoyo(t, true);
tweenCore->onComplete(t, [&]() {
    spdlog::info("Tween complete");
});
tweenCore->onStep(t, [&](float progress) {
    // Track progress
});

tweenCore->pause(t);
tweenCore->resume(t);
float progress = tweenCore->getProgress(t);
tweenCore->complete(t);

// Sequences
auto seq = tweenCore->createSequence();
auto moveX = tweenCore->tweenFloat(0.0f, 100.0f, 1.0f,
    [&](float v) { pos.x = v; });
auto moveY = tweenCore->tweenFloat(0.0f, 200.0f, 0.5f,
    [&](float v) { pos.y = v; });
tweenCore->appendToSequence(seq, moveX);
tweenCore->appendToSequence(seq, moveY);

auto flash = tweenCore->tweenFloat(0.0f, 1.0f, 0.2f,
    [&](float v) { alpha = v; });
tweenCore->insertInSequence(seq, 0.5f, flash);

// Bulk
int count = tweenCore->activeCount();
tweenCore->pauseAll();
tweenCore->resumeAll();
tweenCore->cancelAll();
```
