-- Example: Common Math Patterns
-- Shows vectors, quaternions, transforms, and spatial calculations

-- All math types are globally available (no import needed)

local function mathExamples()
    -- === Vectors ===
    local pos = Vec3.new(5, 0, 10)
    local target = Vec3.new(20, 0, 15)

    -- Distance between points
    local dist = (target - pos):length()
    -- Or for comparison (avoids sqrt):
    local distSq = (target - pos):lengthSquared()
    local inRange = distSq < (15 * 15)

    -- Direction from pos to target
    local direction = (target - pos):normalize()

    -- Move towards target
    local speed = 5.0
    local dt = 0.016
    local newPos = pos + direction * speed * dt

    -- Dot product (how much two directions align: 1=same, 0=perpendicular, -1=opposite)
    local facing = Vec3.forward()
    local alignment = facing:dot(direction)
    local isInFront = alignment > 0

    -- Cross product (perpendicular vector)
    local right = Vec3.forward():cross(Vec3.up())

    -- Flatten vector to XZ plane (remove Y)
    local flat = Vec3.new(direction.x, 0, direction.z)
    if flat:lengthSquared() > 0.001 then
        flat = flat:normalize()
    end

    -- === Quaternions (Rotations) ===
    local noRotation = Quat.identity()

    -- Rotate 90 degrees around Y axis
    local facing90 = Quat.fromAxisAngle(Vec3.up(), math.rad(90))

    -- Look at a target
    local lookDir = (target - pos):normalize()
    local lookRot = Quat.lookAt(lookDir, Vec3.up())

    -- Get forward/right/up from a rotation
    local forward = lookRot:rotateVector(Vec3.forward())
    local right = lookRot:rotateVector(Vec3.right())
    local up = lookRot:rotateVector(Vec3.up())

    -- Face movement direction (for character)
    local moveDir = Vec3.new(1, 0, -1):normalize()
    local faceAngle = math.atan2(-moveDir.x, -moveDir.z)
    local faceRot = Quat.fromAxisAngle(Vec3.up(), faceAngle)

    -- Combine rotations (apply q2 then q1)
    local combined = facing90 * lookRot

    -- === Lerp (Linear Interpolation) ===
    local function lerp(a, b, t)
        return a + (b - a) * t
    end

    -- Smooth movement (frame-rate independent)
    local smoothPos = lerp(pos, target, 5.0 * dt)

    -- === Angle Between Vectors ===
    local dot = Vec3.forward():dot(direction)
    local angle = math.acos(math.max(-1, math.min(1, dot)))  -- Clamp for safety

    -- === Random Direction ===
    local randomAngle = math.random() * math.pi * 2
    local randomDir = Vec3.new(math.cos(randomAngle), 0, math.sin(randomAngle))

    -- === Color ===
    local red = Color.red()
    local custom = Color.new(128, 64, 200, 255)  -- Auto-detects 0-255 range
    local floatColor = Color.fromFloat(0.5, 0.25, 0.8, 1.0)

    -- === Transform3D ===
    local transform = Transform3D.identity()
    transform.position = Vec3.new(5, 0, 10)
    transform.rotation = Quat.fromAxisAngle(Vec3.up(), math.rad(45))
    transform.scale = Vec3.new(2, 2, 2)

    -- === Ray ===
    local ray = Ray3D.new(pos, direction)
    local pointAlong = ray:pointAt(10.0)  -- Point 10 units along the ray

    -- === AABB ===
    local bounds = AABB3D.new(Vec3.new(-5, 0, -5), Vec3.new(5, 10, 5))
    local center = bounds.center   -- Vec3(0, 5, 0)
    local size = bounds.size       -- Vec3(10, 10, 10)
end
