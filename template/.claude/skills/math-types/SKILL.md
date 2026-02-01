---
name: math-types
description: Use math types in Bestow including Vec2, Vec3, Vec4, Quat, Mat4, Color, Transform2D, Transform3D, AABB3D, and Ray3D. Use when doing vector math, rotations, transformations, color manipulation, or spatial calculations.
---

# Math Types

All math types are globally available in Lua. No import needed.

## Vec2

```lua
-- Construction
Vec2.new(x, y)
Vec2.zero()    -- (0, 0)
Vec2.one()     -- (1, 1)
Vec2.up()      -- (0, -1) -- Screen space: up is negative Y
Vec2.down()    -- (0, 1)
Vec2.left()    -- (-1, 0)
Vec2.right()   -- (1, 0)

-- Fields
v.x, v.y

-- Methods
v:length() -> float
v:lengthSquared() -> float    -- Faster than length() when comparing distances
v:normalize() -> Vec2         -- Returns new normalized vector
v:dot(other) -> float

-- Operators
v1 + v2      -- Component-wise add
v1 - v2      -- Component-wise subtract
v * scalar   -- Scale
v / scalar   -- Scale
-v           -- Negate
v1 == v2     -- Equality
```

## Vec3

```lua
-- Construction
Vec3.new(x, y, z)
Vec3.zero()     -- (0, 0, 0)
Vec3.one()      -- (1, 1, 1)
Vec3.up()       -- (0, 1, 0)
Vec3.down()     -- (0, -1, 0)
Vec3.left()     -- (-1, 0, 0)
Vec3.right()    -- (1, 0, 0)
Vec3.forward()  -- (0, 0, -1) -- Negative Z is forward (OpenGL convention)
Vec3.back()     -- (0, 0, 1)

-- Fields
v.x, v.y, v.z

-- Methods
v:length() -> float
v:lengthSquared() -> float
v:normalize() -> Vec3
v:dot(other) -> float
v:cross(other) -> Vec3    -- Cross product (perpendicular vector)

-- Operators (same as Vec2)
v1 + v2, v1 - v2, v * scalar, v / scalar, -v, v1 == v2
```

## Vec4

```lua
Vec4.new(x, y, z, w)
v.x, v.y, v.z, v.w
-- Same operators as Vec2/Vec3
```

## Quat (Quaternion)

Used for 3D rotations. Avoid Euler angles when possible.

```lua
-- Construction
Quat.identity()                          -- No rotation
Quat.new(w, x, y, z)                    -- Raw quaternion (rarely needed)
Quat.fromAxisAngle(axis, angleRad)       -- Rotate around axis
Quat.fromEuler(pitch, yaw, roll)         -- From Euler angles (radians)
Quat.lookAt(direction, up)               -- Rotation facing direction

-- Fields
q.w, q.x, q.y, q.z

-- Methods
q:normalize() -> Quat
q:inverse() -> Quat
q:rotateVector(vec3) -> Vec3    -- Apply rotation to a vector

-- Operators
q1 * q2      -- Combine rotations (apply q2 then q1)

-- Common usage
local facing = Quat.fromAxisAngle(Vec3.up(), math.rad(90))   -- 90 degrees around Y
local lookRot = Quat.lookAt(targetPos - myPos, Vec3.up())
local rotated = facing:rotateVector(Vec3.forward())            -- Get forward direction
```

## Color

```lua
-- Construction (auto-detects 0-1 float vs 0-255 int range)
Color.new(r, g, b, a)          -- e.g., Color.new(255, 0, 0, 255) or Color.new(1.0, 0, 0, 1.0)
Color.fromFloat(r, g, b, a)    -- Explicit 0.0-1.0 range

-- Presets
Color.white()       -- (255, 255, 255, 255)
Color.black()       -- (0, 0, 0, 255)
Color.red()         -- (255, 0, 0, 255)
Color.green()       -- (0, 255, 0, 255)
Color.blue()        -- (0, 0, 255, 255)
Color.transparent() -- (0, 0, 0, 0)

-- Fields
c.r, c.g, c.b, c.a
```

## Mat4 (4x4 Matrix)

```lua
-- Construction
Mat4.identity()
Mat4.new(diagonal)              -- Diagonal matrix

-- Transform matrices
Mat4.translation(x, y, z)       -- or Mat4.translate(vec3)
Mat4.rotate(quat)                -- or Mat4.rotate(angleRad, axisVec3)
Mat4.scale(vec3)                 -- or Mat4.scale(x, y, z) or Mat4.scale(uniform)
Mat4.trs(position, rotation, scale)  -- Combined transform (most common)
Mat4.fromTransform(transform3d)      -- From Transform3D

-- View/projection
Mat4.lookAt(eye, target, up)
Mat4.perspective(fovRad, aspect, near, far)
Mat4.ortho(left, right, bottom, top, near, far)

-- Operators
m1 * m2       -- Matrix multiply
m * vec4      -- Transform Vec4
m * vec3      -- Transform Vec3 (assumes w=1)
```

## Transform2D

```lua
-- Construction
Transform2D.new(x, y, rotation, scaleX, scaleY)

-- Fields
t.x, t.y
t.rotation            -- Radians
t.scaleX, t.scaleY
t.position            -- Vec2
t.scale               -- Vec2
```

## Transform3D

```lua
-- Construction
Transform3D.identity()
Transform3D.new(position, rotation, scale)

-- Fields
t.position    -- Vec3
t.rotation    -- Quat
t.scale       -- Vec3

-- Common usage
local transform = Transform3D.identity()
transform.position = Vec3.new(5, 0, 10)
transform.rotation = Quat.fromAxisAngle(Vec3.up(), math.rad(45))
transform.scale = Vec3.new(2, 2, 2)
```

## AABB3D (Axis-Aligned Bounding Box)

```lua
AABB3D.new(min, max)

aabb.min       -- Vec3
aabb.max       -- Vec3
aabb.center    -- Vec3 (computed)
aabb.extents   -- Vec3 (half-size, computed)
aabb.size      -- Vec3 (full size, computed)
```

## Ray3D

```lua
Ray3D.new(origin, direction)

ray.origin     -- Vec3
ray.direction  -- Vec3
ray:pointAt(distance) -> Vec3   -- Get point along ray
```

## Size and Coordinate

```lua
-- Size (integer dimensions)
Size.new(width, height)
s.width, s.height

-- Coordinate (integer position)
Coordinate.new(x, y)
c.x, c.y
```

## Common Patterns

### Direction from Rotation

```lua
local forward = rotation:rotateVector(Vec3.forward())
local right = rotation:rotateVector(Vec3.right())
local up = rotation:rotateVector(Vec3.up())
```

### Distance Between Points

```lua
local dist = (posA - posB):length()
-- Or for comparison (avoids sqrt):
local distSq = (posA - posB):lengthSquared()
if distSq < range * range then -- within range
```

### Smooth Interpolation (Lerp)

```lua
-- Manual lerp for vectors
local function lerp(a, b, t)
    return a + (b - a) * t
end

local smoothPos = lerp(currentPos, targetPos, 5.0 * dt)
```

### Camera Look-At Rotation

```lua
local direction = (targetPos - cameraPos):normalize()
local rotation = Quat.lookAt(direction, Vec3.up())
```

### Angle Between Vectors

```lua
local dot = dirA:normalize():dot(dirB:normalize())
local angle = math.acos(math.max(-1, math.min(1, dot)))  -- Clamp for safety
```

### Random Direction

```lua
local angle = math.random() * math.pi * 2
local dir = Vec3.new(math.cos(angle), 0, math.sin(angle))
```

### Flatten Vector (Remove Y)

```lua
local flat = Vec3.new(v.x, 0, v.z)
if flat:lengthSquared() > 0.001 then
    flat = flat:normalize()
end
```
