// bestow-luabind/src/docs/types_binding_doc.cpp
// API documentation for core types (Vec2, Vec3, Quat, Color, etc.)

module bestow.luabind;

import std;

namespace bestow {

void registerTypesDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "types";
    sys.qualifiedName = "bestow.types";
    sys.description = "Core math types and data structures used throughout the API.";

    // --- Vec2 ---
    sys.types.push_back(TypeDoc{
        .name = "Vec2",
        .qualifiedName = "Vec2",
        .description = "2D vector with x, y components. Supports arithmetic operators (+, -, *, /).",
        .fields = {
            {"x", "number", "X component"},
            {"y", "number", "Y component"},
        },
        .methods = {
            {.name = "length", .qualifiedName = "Vec2:length",
             .description = "Get the magnitude of the vector.",
             .returns = {{.type = "number", .description = "Length of the vector"}}},
            {.name = "lengthSquared", .qualifiedName = "Vec2:lengthSquared",
             .description = "Get the squared magnitude (avoids sqrt).",
             .returns = {{.type = "number", .description = "Squared length"}}},
            {.name = "normalize", .qualifiedName = "Vec2:normalize",
             .description = "Return a unit-length copy of this vector.",
             .returns = {{.type = "Vec2", .description = "Normalized vector"}}},
            {.name = "dot", .qualifiedName = "Vec2:dot",
             .description = "Dot product with another vector.",
             .params = {{.name = "other", .type = "Vec2", .description = "Vector to dot with"}},
             .returns = {{.type = "number", .description = "Dot product"}}},
        },
        .example = "local v = Vec2.new(3, 4)\nprint(v:length())  -- 5\nlocal n = v:normalize()\nlocal sum = v + Vec2.one()",
    });

    // --- Vec3 ---
    sys.types.push_back(TypeDoc{
        .name = "Vec3",
        .qualifiedName = "Vec3",
        .description = "3D vector with x, y, z components. Supports arithmetic operators (+, -, *, /).",
        .fields = {
            {"x", "number", "X component"},
            {"y", "number", "Y component"},
            {"z", "number", "Z component"},
        },
        .methods = {
            {.name = "length", .qualifiedName = "Vec3:length",
             .description = "Get the magnitude of the vector.",
             .returns = {{.type = "number", .description = "Length"}}},
            {.name = "lengthSquared", .qualifiedName = "Vec3:lengthSquared",
             .description = "Get the squared magnitude.",
             .returns = {{.type = "number", .description = "Squared length"}}},
            {.name = "normalize", .qualifiedName = "Vec3:normalize",
             .description = "Return a unit-length copy.",
             .returns = {{.type = "Vec3", .description = "Normalized vector"}}},
            {.name = "dot", .qualifiedName = "Vec3:dot",
             .description = "Dot product.",
             .params = {{.name = "other", .type = "Vec3", .description = "Vector to dot with"}},
             .returns = {{.type = "number", .description = "Dot product"}}},
            {.name = "cross", .qualifiedName = "Vec3:cross",
             .description = "Cross product.",
             .params = {{.name = "other", .type = "Vec3", .description = "Vector to cross with"}},
             .returns = {{.type = "Vec3", .description = "Cross product vector"}}},
        },
        .example = "local pos = Vec3.new(1, 2, 3)\nlocal dir = Vec3.forward()  -- (0, 0, -1)\nlocal up = Vec3.up()        -- (0, 1, 0)",
    });

    // --- Vec4 ---
    sys.types.push_back(TypeDoc{
        .name = "Vec4",
        .qualifiedName = "Vec4",
        .description = "4D vector with x, y, z, w components.",
        .fields = {
            {"x", "number", "X component"},
            {"y", "number", "Y component"},
            {"z", "number", "Z component"},
            {"w", "number", "W component"},
        },
    });

    // --- Quat ---
    sys.types.push_back(TypeDoc{
        .name = "Quat",
        .qualifiedName = "Quat",
        .description = "Quaternion for 3D rotations. Multiply quaternions to compose rotations.",
        .fields = {
            {"w", "number", "W (scalar) component"},
            {"x", "number", "X component"},
            {"y", "number", "Y component"},
            {"z", "number", "Z component"},
        },
        .methods = {
            {.name = "normalize", .qualifiedName = "Quat:normalize",
             .description = "Return a normalized quaternion.",
             .returns = {{.type = "Quat", .description = "Normalized quaternion"}}},
            {.name = "inverse", .qualifiedName = "Quat:inverse",
             .description = "Return the inverse quaternion.",
             .returns = {{.type = "Quat", .description = "Inverse quaternion"}}},
            {.name = "rotateVector", .qualifiedName = "Quat:rotateVector",
             .description = "Rotate a Vec3 by this quaternion.",
             .params = {{.name = "v", .type = "Vec3", .description = "Vector to rotate"}},
             .returns = {{.type = "Vec3", .description = "Rotated vector"}}},
        },
        .example = "local rot = Quat.fromAxisAngle(Vec3.up(), math.rad(90))\nlocal dir = rot:rotateVector(Vec3.forward())\nlocal look = Quat.lookAt(Vec3.new(0,0,-1), Vec3.up())",
    });

    // --- Color ---
    sys.types.push_back(TypeDoc{
        .name = "Color",
        .qualifiedName = "Color",
        .description = "RGBA color. Use Color.new() with 0-1 floats or 0-255 integers (auto-detected).",
        .fields = {
            {"r", "number", "Red component (0-255)"},
            {"g", "number", "Green component (0-255)"},
            {"b", "number", "Blue component (0-255)"},
            {"a", "number", "Alpha component (0-255)"},
        },
        .example = "local red = Color.red()\nlocal custom = Color.new(0.2, 0.5, 1.0, 1.0)  -- float range\nlocal pixel = Color.new(255, 128, 0, 255)      -- int range",
    });

    // --- Transform2D ---
    sys.types.push_back(TypeDoc{
        .name = "Transform2D",
        .qualifiedName = "Transform2D",
        .description = "2D transformation with position, rotation, and scale.",
        .fields = {
            {"x", "number", "X position"},
            {"y", "number", "Y position"},
            {"rotation", "number", "Rotation in radians"},
            {"scaleX", "number", "X scale factor"},
            {"scaleY", "number", "Y scale factor"},
            {"position", "Vec2", "Position as Vec2 (read/write)"},
            {"scale", "Vec2", "Scale as Vec2 (read/write)"},
        },
        .example = "local t = Transform2D.new(100, 200, 0, 1, 1)",
    });

    // --- Transform3D ---
    sys.types.push_back(TypeDoc{
        .name = "Transform3D",
        .qualifiedName = "Transform3D",
        .description = "3D transformation with position, rotation (quaternion), and scale.",
        .fields = {
            {"position", "Vec3", "World position"},
            {"rotation", "Quat", "Orientation as quaternion"},
            {"scale", "Vec3", "Scale factors"},
        },
        .example = "local t = Transform3D.new(Vec3.zero(), Quat.identity(), Vec3.one())\nlocal t2 = Transform3D.identity()",
    });

    // --- AABB3D ---
    sys.types.push_back(TypeDoc{
        .name = "AABB3D",
        .qualifiedName = "AABB3D",
        .description = "Axis-aligned bounding box in 3D space.",
        .fields = {
            {"min", "Vec3", "Minimum corner"},
            {"max", "Vec3", "Maximum corner"},
        },
        .methods = {
            {.name = "center", .qualifiedName = "AABB3D:center",
             .description = "Get the center point.",
             .returns = {{.type = "Vec3", .description = "Center of the AABB"}}},
            {.name = "extents", .qualifiedName = "AABB3D:extents",
             .description = "Get half-size extents.",
             .returns = {{.type = "Vec3", .description = "Half-size in each axis"}}},
            {.name = "size", .qualifiedName = "AABB3D:size",
             .description = "Get full size.",
             .returns = {{.type = "Vec3", .description = "Full size in each axis"}}},
        },
    });

    // --- Ray3D ---
    sys.types.push_back(TypeDoc{
        .name = "Ray3D",
        .qualifiedName = "Ray3D",
        .description = "A ray in 3D space defined by an origin point and direction.",
        .fields = {
            {"origin", "Vec3", "Starting point of the ray"},
            {"direction", "Vec3", "Direction of the ray (should be normalized)"},
        },
        .methods = {
            {.name = "pointAt", .qualifiedName = "Ray3D:pointAt",
             .description = "Get a point along the ray at a given distance.",
             .params = {{.name = "t", .type = "number", .description = "Distance along the ray"}},
             .returns = {{.type = "Vec3", .description = "Point at origin + direction * t"}}},
        },
    });

    // --- Mat4 ---
    sys.types.push_back(TypeDoc{
        .name = "Mat4",
        .qualifiedName = "Mat4",
        .description = "4x4 matrix for transformations, projections, and view matrices. Supports multiplication with other matrices and vectors.",
        .methods = {
            {.name = "identity", .qualifiedName = "Mat4.identity",
             .description = "Create an identity matrix.",
             .returns = {{.type = "Mat4", .description = "Identity matrix"}}},
            {.name = "translation", .qualifiedName = "Mat4.translation",
             .description = "Create a translation matrix from x, y, z offsets.",
             .params = {
                 {.name = "x", .type = "number", .description = "X offset"},
                 {.name = "y", .type = "number", .description = "Y offset"},
                 {.name = "z", .type = "number", .description = "Z offset"},
             },
             .returns = {{.type = "Mat4", .description = "Translation matrix"}}},
            {.name = "translate", .qualifiedName = "Mat4.translate",
             .description = "Create a translation matrix from a Vec3.",
             .params = {{.name = "offset", .type = "Vec3", .description = "Translation offset vector"}},
             .returns = {{.type = "Mat4", .description = "Translation matrix"}}},
            {.name = "rotate", .qualifiedName = "Mat4.rotate",
             .description = "Create a rotation matrix from a quaternion or axis-angle.",
             .params = {{.name = "q_or_angle", .type = "Quat|number", .description = "Quaternion or angle in radians"}},
             .returns = {{.type = "Mat4", .description = "Rotation matrix"}}},
            {.name = "scale", .qualifiedName = "Mat4.scale",
             .description = "Create a scale matrix.",
             .params = {{.name = "s", .type = "number|Vec3", .description = "Uniform scale or Vec3 scale"}},
             .returns = {{.type = "Mat4", .description = "Scale matrix"}}},
            {.name = "lookAt", .qualifiedName = "Mat4.lookAt",
             .description = "Create a view matrix.",
             .params = {
                 {.name = "eye", .type = "Vec3", .description = "Camera position"},
                 {.name = "target", .type = "Vec3", .description = "Look-at target"},
                 {.name = "up", .type = "Vec3", .description = "Up direction"},
             },
             .returns = {{.type = "Mat4", .description = "View matrix"}}},
            {.name = "perspective", .qualifiedName = "Mat4.perspective",
             .description = "Create a perspective projection matrix.",
             .params = {
                 {.name = "fovY", .type = "number", .description = "Vertical field of view in degrees"},
                 {.name = "aspect", .type = "number", .description = "Aspect ratio (width/height)"},
                 {.name = "near", .type = "number", .description = "Near clipping plane"},
                 {.name = "far", .type = "number", .description = "Far clipping plane"},
             },
             .returns = {{.type = "Mat4", .description = "Projection matrix"}}},
            {.name = "ortho", .qualifiedName = "Mat4.ortho",
             .description = "Create an orthographic projection matrix.",
             .params = {
                 {.name = "left", .type = "number", .description = "Left boundary"},
                 {.name = "right", .type = "number", .description = "Right boundary"},
                 {.name = "bottom", .type = "number", .description = "Bottom boundary"},
                 {.name = "top", .type = "number", .description = "Top boundary"},
                 {.name = "near", .type = "number", .description = "Near plane"},
                 {.name = "far", .type = "number", .description = "Far plane"},
             },
             .returns = {{.type = "Mat4", .description = "Orthographic projection matrix"}}},
            {.name = "trs", .qualifiedName = "Mat4.trs",
             .description = "Create a composite Translation * Rotation * Scale matrix.",
             .params = {
                 {.name = "position", .type = "Vec3", .description = "Translation"},
                 {.name = "rotation", .type = "Quat", .description = "Rotation"},
                 {.name = "scale", .type = "Vec3", .description = "Scale"},
             },
             .returns = {{.type = "Mat4", .description = "Combined TRS matrix"}}},
            {.name = "fromTransform", .qualifiedName = "Mat4.fromTransform",
             .description = "Create a matrix from a Transform3D.",
             .params = {{.name = "transform", .type = "Transform3D", .description = "The transform to convert"}},
             .returns = {{.type = "Mat4", .description = "Equivalent matrix"}}},
        },
    });

    // --- Size ---
    sys.types.push_back(TypeDoc{
        .name = "Size",
        .qualifiedName = "Size",
        .description = "Integer width/height pair.",
        .fields = {
            {"width", "number", "Width in pixels"},
            {"height", "number", "Height in pixels"},
        },
    });

    // --- Coordinate ---
    sys.types.push_back(TypeDoc{
        .name = "Coordinate",
        .qualifiedName = "Coordinate",
        .description = "Integer x/y coordinate pair.",
        .fields = {
            {"x", "number", "X coordinate"},
            {"y", "number", "Y coordinate"},
        },
    });

    // --- Entity ---
    sys.types.push_back(TypeDoc{
        .name = "Entity",
        .qualifiedName = "Entity",
        .description = "Opaque entity handle. Compare with == but do not construct directly.",
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
