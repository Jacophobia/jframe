// bestow-luabind/src/docs/graphics3d_binding_doc.cpp
// API documentation for bestow.graphics3d

module bestow.luabind;

import std;

namespace bestow {

void registerGraphics3DDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "graphics3d";
    sys.qualifiedName = "bestow.graphics3d";
    sys.description = "3D rendering system. Manages meshes, materials, lighting, cameras, shadows, post-processing, debug drawing, and lock-on targeting.";

    // --- Enums ---

    sys.enums.push_back(EnumDoc{
        .name = "Graphics3DError",
        .qualifiedName = "Graphics3DError",
        .description = "Error codes returned by graphics3d operations.",
        .values = {
            {"Success", "Operation completed successfully"},
            {"InvalidMesh", "The mesh handle is invalid"},
            {"InvalidMaterial", "The material handle is invalid"},
            {"InvalidTexture", "The texture handle is invalid"},
            {"InvalidShader", "The shader handle is invalid"},
            {"ShaderCompilationFailed", "Shader failed to compile"},
            {"OutOfMemory", "GPU or system memory exhausted"},
            {"ContextLost", "Render context was lost"},
            {"InternalError", "An internal error occurred"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "BlendMode",
        .qualifiedName = "BlendMode",
        .description = "Blend modes for materials.",
        .values = {
            {"Opaque", "No blending, fully opaque"},
            {"AlphaBlend", "Standard alpha blending"},
            {"Additive", "Additive blending (glow effects)"},
            {"Multiply", "Multiply blending"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "CullMode",
        .qualifiedName = "CullMode",
        .description = "Face culling modes for materials.",
        .values = {
            {"None", "No face culling (render both sides)"},
            {"Back", "Cull back faces (default)"},
            {"Front", "Cull front faces"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "TextAlignment3D",
        .qualifiedName = "TextAlignment3D",
        .description = "Horizontal alignment for 3D text.",
        .values = {
            {"Left", "Left-aligned"},
            {"Center", "Center-aligned"},
            {"Right", "Right-aligned"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "TextVerticalAlign3D",
        .qualifiedName = "TextVerticalAlign3D",
        .description = "Vertical alignment for 3D text.",
        .values = {
            {"Top", "Top-aligned"},
            {"Middle", "Middle-aligned"},
            {"Bottom", "Bottom-aligned"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "PresentMode",
        .qualifiedName = "PresentMode",
        .description = "Presentation mode for frame delivery.",
        .values = {
            {"Immediate", "No vsync, minimal latency, may tear"},
            {"FIFO", "VSync enabled, no tearing"},
            {"Mailbox", "Triple-buffered, low latency, no tearing"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "ProjectionType",
        .qualifiedName = "ProjectionType",
        .description = "Camera projection type.",
        .values = {
            {"Perspective", "Perspective projection (3D depth)"},
            {"Orthographic", "Orthographic projection (no depth foreshortening)"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "LockPointSource",
        .qualifiedName = "LockPointSource",
        .description = "Source type for lock-on target points.",
        .values = {
            {"Socket", "Lock point follows an animation socket"},
            {"Offset", "Lock point uses a fixed local offset"},
        }
    });

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "PBRMaterial",
        .qualifiedName = "PBRMaterial",
        .description = "Physically-based rendering material definition.",
        .fields = {
            {"baseColorFactor", "Vec4", "Base color RGBA (accepts Vec4 or Color)"},
            {"baseColorTexture", "TextureHandle", "Base color texture"},
            {"metallicFactor", "number", "Metallic factor (0-1)"},
            {"roughnessFactor", "number", "Roughness factor (0-1)"},
            {"metallicRoughnessTexture", "TextureHandle", "Combined metallic-roughness texture"},
            {"normalTexture", "TextureHandle", "Normal map texture"},
            {"normalScale", "number", "Normal map intensity"},
            {"occlusionTexture", "TextureHandle", "Ambient occlusion texture"},
            {"occlusionStrength", "number", "Occlusion strength (0-1)"},
            {"emissiveFactor", "Vec3", "Emissive color RGB"},
            {"emissiveTexture", "TextureHandle", "Emissive map texture"},
            {"blendMode", "BlendMode", "Blend mode"},
            {"cullMode", "CullMode", "Face culling mode"},
            {"alphaCutoff", "number", "Alpha cutoff threshold"},
            {"doubleSided", "boolean", "Render both sides"},
            {"receiveShadows", "boolean", "Whether this material receives shadows"},
            {"castShadows", "boolean", "Whether this material casts shadows"},
        },
        .example = "local mat = PBRMaterial.new()\nmat.baseColorFactor = Vec4.new(1, 0.5, 0.5, 1)\nmat.metallicFactor = 0.0\nmat.roughnessFactor = 0.8",
    });

    sys.types.push_back(TypeDoc{
        .name = "UnlitMaterial",
        .qualifiedName = "UnlitMaterial",
        .description = "Unlit material with no lighting calculations.",
        .fields = {
            {"color", "Vec4", "Base color RGBA"},
            {"texture", "TextureHandle", "Base color texture"},
            {"blendMode", "BlendMode", "Blend mode"},
            {"cullMode", "CullMode", "Face culling mode"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "DirectionalLight",
        .qualifiedName = "DirectionalLight",
        .description = "A directional light (like sunlight).",
        .fields = {
            {"direction", "Vec3", "Light direction vector"},
            {"color", "Vec3", "Light color RGB"},
            {"intensity", "number", "Light intensity multiplier"},
            {"castShadows", "boolean", "Whether this light casts shadows"},
            {"shadowMapResolution", "number", "Shadow map resolution in pixels"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "PointLight",
        .qualifiedName = "PointLight",
        .description = "A point light that emits in all directions from a position.",
        .fields = {
            {"color", "Vec3", "Light color RGB"},
            {"intensity", "number", "Light intensity multiplier"},
            {"range", "number", "Maximum range of the light"},
            {"castShadows", "boolean", "Whether this light casts shadows"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "SpotLight",
        .qualifiedName = "SpotLight",
        .description = "A spot light with a cone-shaped beam.",
        .fields = {
            {"direction", "Vec3", "Light direction vector"},
            {"color", "Vec3", "Light color RGB"},
            {"intensity", "number", "Light intensity multiplier"},
            {"range", "number", "Maximum range of the light"},
            {"innerConeAngle", "number", "Inner cone angle in radians"},
            {"outerConeAngle", "number", "Outer cone angle in radians"},
            {"castShadows", "boolean", "Whether this light casts shadows"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "Skybox",
        .qualifiedName = "Skybox",
        .description = "Skybox configuration for environment rendering.",
        .fields = {
            {"cubemapTexture", "TextureHandle", "Cubemap texture handle"},
            {"rotation", "number", "Rotation angle around Y axis"},
            {"exposure", "number", "Exposure multiplier"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "Fog",
        .qualifiedName = "Fog",
        .description = "Fog settings for atmospheric effects.",
        .fields = {
            {"enabled", "boolean", "Whether fog is active"},
            {"color", "Vec3", "Fog color RGB"},
            {"density", "number", "Fog density"},
            {"startDistance", "number", "Distance where fog begins"},
            {"endDistance", "number", "Distance where fog is fully opaque"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "Camera3D",
        .qualifiedName = "Camera3D",
        .description = "3D camera configuration.",
        .fields = {
            {"transform", "Transform3D", "Camera position and orientation"},
            {"projection", "ProjectionType", "Projection type"},
            {"fovY", "number", "Vertical field of view in degrees"},
            {"aspectRatio", "number", "Aspect ratio (width / height)"},
            {"orthoWidth", "number", "Orthographic projection width"},
            {"orthoHeight", "number", "Orthographic projection height"},
            {"nearPlane", "number", "Near clipping plane distance"},
            {"farPlane", "number", "Far clipping plane distance"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "RenderStats",
        .qualifiedName = "RenderStats",
        .description = "Per-frame rendering statistics.",
        .fields = {
            {"drawCalls", "number", "Number of draw calls this frame"},
            {"triangles", "number", "Number of triangles rendered"},
            {"vertices", "number", "Number of vertices processed"},
            {"meshes", "number", "Number of active meshes"},
            {"materials", "number", "Number of active materials"},
            {"textures", "number", "Number of active textures"},
            {"lights", "number", "Number of active lights"},
            {"visibleObjects", "number", "Number of objects that passed culling"},
            {"culledObjects", "number", "Number of objects culled"},
            {"frameTimeMs", "number", "CPU frame time in milliseconds"},
            {"gpuTimeMs", "number", "GPU frame time in milliseconds"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "Graphics3DConfig",
        .qualifiedName = "Graphics3DConfig",
        .description = "Configuration for initializing the 3D graphics system.",
        .fields = {
            {"windowWidth", "number", "Window width in pixels"},
            {"windowHeight", "number", "Window height in pixels"},
            {"windowTitle", "string", "Window title text"},
            {"vsync", "boolean", "Enable vertical sync"},
            {"fullscreen", "boolean", "Start in fullscreen mode"},
            {"enableValidation", "boolean", "Enable GPU validation layers (debug)"},
            {"nativeWindowHandle", "userdata", "Native window handle for embedding"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "Text3DStyle",
        .qualifiedName = "Text3DStyle",
        .description = "Style configuration for 3D text rendering.",
        .fields = {
            {"font", "Font3DHandle", "Font handle"},
            {"fontSize", "number", "Font size"},
            {"color", "Color", "Text color"},
            {"alignment", "TextAlignment3D", "Horizontal alignment"},
            {"verticalAlign", "TextVerticalAlign3D", "Vertical alignment"},
            {"lineSpacing", "number", "Line spacing multiplier"},
            {"letterSpacing", "number", "Letter spacing offset"},
            {"billboard", "boolean", "Always face camera"},
            {"outlineWidth", "number", "Outline width"},
            {"outlineColor", "Color", "Outline color"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "Text3DItem",
        .qualifiedName = "Text3DItem",
        .description = "A complete 3D text item with text, style, and transform.",
        .fields = {
            {"text", "string", "Text content"},
            {"style", "Text3DStyle", "Text style configuration"},
            {"transform", "Transform3D", "World transform"},
            {"layer", "number", "Render layer"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "LockPointDef",
        .qualifiedName = "LockPointDef",
        .description = "Defines a point on an entity that can be locked onto.",
        .fields = {
            {"name", "string", "Lock point name"},
            {"source", "LockPointSource", "Whether to use a socket or offset"},
            {"socketName", "string", "Animation socket name (if source is Socket)"},
            {"localOffset", "Vec3", "Local offset from entity origin (if source is Offset)"},
            {"priority", "number", "Priority weight for target selection"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "LockableTarget",
        .qualifiedName = "LockableTarget",
        .description = "Component that makes an entity targetable by the lock-on system.",
        .fields = {
            {"lockPoints", "LockPointDef[]", "Array of lock point definitions"},
            {"enabled", "boolean", "Whether this target is currently active"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "LockOnConfig",
        .qualifiedName = "LockOnConfig",
        .description = "Configuration for the lock-on targeting system.",
        .fields = {
            {"maxRange", "number", "Maximum range for target detection"},
            {"fovMargin", "number", "Field of view margin for target visibility"},
            {"centerBias", "number", "Bias toward screen center when scoring targets"},
            {"priorityWeight", "number", "Weight given to lock point priority"},
            {"preferCurrentTarget", "boolean", "Prefer the current target in scoring"},
            {"hysteresis", "number", "Score bonus for keeping current target"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "LockOnResult",
        .qualifiedName = "LockOnResult",
        .description = "Result of a lock-on targeting query.",
        .fields = {
            {"entity", "Entity", "The targeted entity"},
            {"lockPointIndex", "number", "Index of the selected lock point"},
            {"worldPosition", "Vec3", "World-space position of the lock point"},
            {"screenPosition", "Vec2", "Screen-space position of the lock point"},
            {"distance", "number", "Distance from camera to target"},
            {"score", "number", "Targeting score (higher = better match)"},
            {"isValid", "boolean", "Whether this result contains a valid target"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "AnimationClip",
        .qualifiedName = "AnimationClip",
        .description = "Information about a loaded animation clip.",
        .fields = {
            {"name", "string", "Name of the animation clip"},
            {"duration", "number", "Duration of the clip in seconds"},
            {"looping", "boolean", "Whether the clip loops by default"},
            {"ticksPerSecond", "number", "Playback tick rate"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "AnimationState",
        .qualifiedName = "AnimationState",
        .description = "Current state of an animation being played on a skeleton.",
        .fields = {
            {"clip", "AnimationClipHandle", "Handle to the animation clip"},
            {"time", "number", "Current playback time in seconds"},
            {"speed", "number", "Playback speed multiplier"},
            {"weight", "number", "Blend weight (0.0 to 1.0)"},
            {"playing", "boolean", "Whether the animation is currently playing"},
            {"looping", "boolean", "Whether the animation loops"},
        },
    });

    // --- Methods: Lifecycle ---

    sys.methods.push_back(MethodDoc{
        .name = "initialize",
        .qualifiedName = "bestow.graphics3d.initialize",
        .description = "Initialize the 3D graphics system with the given configuration.",
        .params = {
            {.name = "config", .type = "table", .description = "Configuration table with fields: windowWidth, windowHeight, windowTitle, vsync, fullscreen, enableValidation"},
        },
        .returns = {{.type = "boolean", .description = "true if initialization succeeded"}},
        .example = "bestow.graphics3d.initialize({\n    windowWidth = 1280,\n    windowHeight = 720,\n    windowTitle = \"My Game\",\n    vsync = true\n})",
    });

    sys.methods.push_back(MethodDoc{
        .name = "shutdown",
        .qualifiedName = "bestow.graphics3d.shutdown",
        .description = "Shut down the graphics system and release all resources.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "getNativeWindowHandle",
        .qualifiedName = "bestow.graphics3d.getNativeWindowHandle",
        .description = "Get the native OS window handle.",
        .returns = {{.type = "userdata", .description = "Native window handle"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isInitialized",
        .qualifiedName = "bestow.graphics3d.isInitialized",
        .description = "Check if the graphics system is initialized.",
        .returns = {{.type = "boolean", .description = "true if initialized"}},
    });

    // --- Methods: Frame Lifecycle ---

    sys.methods.push_back(MethodDoc{
        .name = "beginFrame",
        .qualifiedName = "bestow.graphics3d.beginFrame",
        .description = "Begin a new render frame. Call before any draw commands.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "endFrame",
        .qualifiedName = "bestow.graphics3d.endFrame",
        .description = "End the current render frame. Presents to screen.",
    });

    // --- Methods: Primitive Mesh Generation ---

    sys.methods.push_back(MethodDoc{
        .name = "createCubeMesh",
        .qualifiedName = "bestow.graphics3d.createCubeMesh",
        .description = "Create a cube mesh.",
        .params = {
            {.name = "size", .type = "number", .description = "Side length of the cube", .optional = true, .defaultVal = "1.0"},
        },
        .returns = {{.type = "MeshHandle|nil", .description = "Handle to the created mesh, or nil on error"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "createSphereMesh",
        .qualifiedName = "bestow.graphics3d.createSphereMesh",
        .description = "Create a sphere mesh.",
        .params = {
            {.name = "radius", .type = "number", .description = "Sphere radius", .optional = true, .defaultVal = "0.5"},
            {.name = "segments", .type = "number", .description = "Number of horizontal segments", .optional = true, .defaultVal = "32"},
            {.name = "rings", .type = "number", .description = "Number of vertical rings", .optional = true, .defaultVal = "16"},
        },
        .returns = {{.type = "MeshHandle|nil", .description = "Handle to the created mesh, or nil on error"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "createCylinderMesh",
        .qualifiedName = "bestow.graphics3d.createCylinderMesh",
        .description = "Create a cylinder mesh.",
        .params = {
            {.name = "radius", .type = "number", .description = "Cylinder radius", .optional = true, .defaultVal = "0.5"},
            {.name = "height", .type = "number", .description = "Cylinder height", .optional = true, .defaultVal = "1.0"},
            {.name = "segments", .type = "number", .description = "Number of segments around the circumference", .optional = true, .defaultVal = "32"},
        },
        .returns = {{.type = "MeshHandle|nil", .description = "Handle to the created mesh, or nil on error"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "createCapsuleMesh",
        .qualifiedName = "bestow.graphics3d.createCapsuleMesh",
        .description = "Create a capsule mesh (cylinder with hemispherical caps).",
        .params = {
            {.name = "radius", .type = "number", .description = "Capsule radius", .optional = true, .defaultVal = "0.5"},
            {.name = "height", .type = "number", .description = "Capsule height (cylinder portion)", .optional = true, .defaultVal = "1.0"},
            {.name = "segments", .type = "number", .description = "Number of segments around the circumference", .optional = true, .defaultVal = "32"},
            {.name = "rings", .type = "number", .description = "Number of rings on each hemisphere", .optional = true, .defaultVal = "8"},
        },
        .returns = {{.type = "MeshHandle|nil", .description = "Handle to the created mesh, or nil on error"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "createPlaneMesh",
        .qualifiedName = "bestow.graphics3d.createPlaneMesh",
        .description = "Create a flat plane mesh.",
        .params = {
            {.name = "width", .type = "number", .description = "Plane width", .optional = true, .defaultVal = "1.0"},
            {.name = "height", .type = "number", .description = "Plane height", .optional = true, .defaultVal = "1.0"},
            {.name = "widthSegments", .type = "number", .description = "Number of width subdivisions", .optional = true, .defaultVal = "1"},
            {.name = "heightSegments", .type = "number", .description = "Number of height subdivisions", .optional = true, .defaultVal = "1"},
        },
        .returns = {{.type = "MeshHandle|nil", .description = "Handle to the created mesh, or nil on error"}},
    });

    // --- Methods: Mesh Management ---

    sys.methods.push_back(MethodDoc{
        .name = "destroyMesh",
        .qualifiedName = "bestow.graphics3d.destroyMesh",
        .description = "Destroy a mesh and free its GPU resources.",
        .params = {
            {.name = "handle", .type = "MeshHandle", .description = "Handle of the mesh to destroy"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasMesh",
        .qualifiedName = "bestow.graphics3d.hasMesh",
        .description = "Check if a mesh handle is valid.",
        .params = {
            {.name = "handle", .type = "MeshHandle", .description = "Handle to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the mesh exists"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getMeshBounds",
        .qualifiedName = "bestow.graphics3d.getMeshBounds",
        .description = "Get the axis-aligned bounding box of a mesh.",
        .params = {
            {.name = "handle", .type = "MeshHandle", .description = "Handle to the mesh"},
        },
        .returns = {{.type = "AABB3D", .description = "Bounding box of the mesh"}},
    });

    // --- Methods: Material Management ---

    sys.methods.push_back(MethodDoc{
        .name = "createMaterial",
        .qualifiedName = "bestow.graphics3d.createMaterial",
        .description = "Create a PBR material.",
        .params = {
            {.name = "material", .type = "PBRMaterial", .description = "PBR material definition"},
        },
        .returns = {{.type = "MaterialHandle|nil", .description = "Handle to the created material, or nil on error"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "createUnlitMaterial",
        .qualifiedName = "bestow.graphics3d.createUnlitMaterial",
        .description = "Create an unlit material (no lighting calculations).",
        .params = {
            {.name = "material", .type = "UnlitMaterial", .description = "Unlit material definition"},
        },
        .returns = {{.type = "MaterialHandle|nil", .description = "Handle to the created material, or nil on error"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "destroyMaterial",
        .qualifiedName = "bestow.graphics3d.destroyMaterial",
        .description = "Destroy a material and free its resources.",
        .params = {
            {.name = "handle", .type = "MaterialHandle", .description = "Handle of the material to destroy"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasMaterial",
        .qualifiedName = "bestow.graphics3d.hasMaterial",
        .description = "Check if a material handle is valid.",
        .params = {
            {.name = "handle", .type = "MaterialHandle", .description = "Handle to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the material exists"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getDefaultPBRMaterial",
        .qualifiedName = "bestow.graphics3d.getDefaultPBRMaterial",
        .description = "Get the default PBR material handle (white, non-metallic).",
        .returns = {{.type = "MaterialHandle", .description = "Handle to the default PBR material"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getDefaultUnlitMaterial",
        .qualifiedName = "bestow.graphics3d.getDefaultUnlitMaterial",
        .description = "Get the default unlit material handle.",
        .returns = {{.type = "MaterialHandle", .description = "Handle to the default unlit material"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getErrorMaterial",
        .qualifiedName = "bestow.graphics3d.getErrorMaterial",
        .description = "Get the error material handle (magenta checkerboard for missing materials).",
        .returns = {{.type = "MaterialHandle", .description = "Handle to the error material"}},
    });

    // --- Methods: Material Property Updates ---

    sys.methods.push_back(MethodDoc{
        .name = "setMaterialBaseColor",
        .qualifiedName = "bestow.graphics3d.setMaterialBaseColor",
        .description = "Set the base color of an existing material.",
        .params = {
            {.name = "handle", .type = "MaterialHandle", .description = "Material to modify"},
            {.name = "color", .type = "Vec4", .description = "New base color RGBA"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setMaterialMetallicRoughness",
        .qualifiedName = "bestow.graphics3d.setMaterialMetallicRoughness",
        .description = "Set the metallic and roughness factors of an existing material.",
        .params = {
            {.name = "handle", .type = "MaterialHandle", .description = "Material to modify"},
            {.name = "metallic", .type = "number", .description = "Metallic factor (0-1)"},
            {.name = "roughness", .type = "number", .description = "Roughness factor (0-1)"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setMaterialEmissive",
        .qualifiedName = "bestow.graphics3d.setMaterialEmissive",
        .description = "Set the emissive color of an existing material.",
        .params = {
            {.name = "handle", .type = "MaterialHandle", .description = "Material to modify"},
            {.name = "emissive", .type = "Vec3", .description = "Emissive color RGB"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getMaterialProperties",
        .qualifiedName = "bestow.graphics3d.getMaterialProperties",
        .description = "Get the current properties of a material.",
        .params = {
            {.name = "handle", .type = "MaterialHandle", .description = "Material to query"},
        },
        .returns = {{.type = "PBRMaterial|nil", .description = "Material properties, or nil if invalid"}},
    });

    // --- Methods: Immediate Mode Rendering ---

    sys.methods.push_back(MethodDoc{
        .name = "drawMesh",
        .qualifiedName = "bestow.graphics3d.drawMesh",
        .description = "Draw a mesh with the given material and world transform. Accepts either a Mat4 or Transform3D for the transform.",
        .params = {
            {.name = "mesh", .type = "MeshHandle", .description = "Mesh to draw"},
            {.name = "material", .type = "MaterialHandle", .description = "Material to apply"},
            {.name = "transform", .type = "Mat4|Transform3D", .description = "World transform matrix or Transform3D"},
            {.name = "castShadow", .type = "boolean", .description = "Whether this mesh casts shadows", .optional = true, .defaultVal = "true"},
            {.name = "receiveShadow", .type = "boolean", .description = "Whether this mesh receives shadows", .optional = true, .defaultVal = "true"},
        },
    });

    // --- Methods: Camera ---

    sys.methods.push_back(MethodDoc{
        .name = "setCamera",
        .qualifiedName = "bestow.graphics3d.setCamera",
        .description = "Set the active camera. Accepts a Camera3D userdata or a look-at table with fields: position (Vec3), target (Vec3), up (Vec3, optional), fov/fovY (number), near/nearPlane (number), far/farPlane (number), aspectRatio (number, optional).",
        .params = {
            {.name = "camera", .type = "Camera3D|table", .description = "Camera3D object or look-at configuration table"},
        },
        .example = "bestow.graphics3d.setCamera({\n    position = Vec3.new(0, 5, 10),\n    target = Vec3.new(0, 0, 0),\n    fov = 60,\n    near = 0.1,\n    far = 1000\n})",
    });

    sys.methods.push_back(MethodDoc{
        .name = "getCamera",
        .qualifiedName = "bestow.graphics3d.getCamera",
        .description = "Get the current active camera.",
        .returns = {{.type = "Camera3D", .description = "The current camera"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "screenToWorldRay",
        .qualifiedName = "bestow.graphics3d.screenToWorldRay",
        .description = "Convert a screen position to a world-space ray.",
        .params = {
            {.name = "screenPos", .type = "Vec2", .description = "Screen position in pixels"},
        },
        .returns = {{.type = "Ray", .description = "World-space ray from the camera through the screen position"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "worldToScreen",
        .qualifiedName = "bestow.graphics3d.worldToScreen",
        .description = "Project a world position to screen coordinates.",
        .params = {
            {.name = "worldPos", .type = "Vec3", .description = "World-space position"},
        },
        .returns = {{.type = "Vec2|nil", .description = "Screen position, or nil if behind the camera"}},
    });

    // --- Methods: Lighting ---

    sys.methods.push_back(MethodDoc{
        .name = "setDirectionalLight",
        .qualifiedName = "bestow.graphics3d.setDirectionalLight",
        .description = "Set the scene's directional light (sun). Accepts a table with fields: direction (Vec3), color (Vec3), intensity (number), castShadows (boolean), shadowMapResolution (number).",
        .params = {
            {.name = "light", .type = "table", .description = "Directional light configuration table"},
        },
        .example = "bestow.graphics3d.setDirectionalLight({\n    direction = Vec3.new(-0.5, -1.0, -0.3),\n    color = Vec3.new(1.0, 0.95, 0.9),\n    intensity = 1.5,\n    castShadows = true\n})",
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearDirectionalLight",
        .qualifiedName = "bestow.graphics3d.clearDirectionalLight",
        .description = "Remove the directional light from the scene.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "addPointLight",
        .qualifiedName = "bestow.graphics3d.addPointLight",
        .description = "Add a point light to the scene.",
        .params = {
            {.name = "light", .type = "PointLight", .description = "Point light configuration"},
            {.name = "position", .type = "Vec3", .description = "World position of the light"},
        },
        .returns = {{.type = "number", .description = "Light ID for later modification or removal"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "addSpotLight",
        .qualifiedName = "bestow.graphics3d.addSpotLight",
        .description = "Add a spot light to the scene.",
        .params = {
            {.name = "light", .type = "SpotLight", .description = "Spot light configuration"},
            {.name = "position", .type = "Vec3", .description = "World position of the light"},
        },
        .returns = {{.type = "number", .description = "Light ID for later modification or removal"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setLightPosition",
        .qualifiedName = "bestow.graphics3d.setLightPosition",
        .description = "Update the position of an existing point or spot light.",
        .params = {
            {.name = "lightId", .type = "number", .description = "Light ID from addPointLight or addSpotLight"},
            {.name = "position", .type = "Vec3", .description = "New world position"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeLight",
        .qualifiedName = "bestow.graphics3d.removeLight",
        .description = "Remove a point or spot light from the scene.",
        .params = {
            {.name = "lightId", .type = "number", .description = "Light ID to remove"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearLights",
        .qualifiedName = "bestow.graphics3d.clearLights",
        .description = "Remove all point and spot lights from the scene.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "setAmbientLight",
        .qualifiedName = "bestow.graphics3d.setAmbientLight",
        .description = "Set the ambient light color and intensity.",
        .params = {
            {.name = "color", .type = "Vec3", .description = "Ambient light color RGB"},
            {.name = "intensity", .type = "number", .description = "Intensity multiplier", .optional = true, .defaultVal = "1.0"},
        },
    });

    // --- Methods: Environment ---

    sys.methods.push_back(MethodDoc{
        .name = "setSkybox",
        .qualifiedName = "bestow.graphics3d.setSkybox",
        .description = "Set the scene skybox.",
        .params = {
            {.name = "skybox", .type = "Skybox", .description = "Skybox configuration"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearSkybox",
        .qualifiedName = "bestow.graphics3d.clearSkybox",
        .description = "Remove the skybox from the scene.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "setFog",
        .qualifiedName = "bestow.graphics3d.setFog",
        .description = "Set the scene fog settings.",
        .params = {
            {.name = "fog", .type = "Fog", .description = "Fog configuration"},
        },
    });

    // --- Methods: Shadows ---

    sys.methods.push_back(MethodDoc{
        .name = "setShadowsEnabled",
        .qualifiedName = "bestow.graphics3d.setShadowsEnabled",
        .description = "Enable or disable shadow rendering.",
        .params = {
            {.name = "enabled", .type = "boolean", .description = "true to enable shadows"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "areShadowsEnabled",
        .qualifiedName = "bestow.graphics3d.areShadowsEnabled",
        .description = "Check if shadow rendering is enabled.",
        .returns = {{.type = "boolean", .description = "true if shadows are enabled"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setDirectionalShadowResolution",
        .qualifiedName = "bestow.graphics3d.setDirectionalShadowResolution",
        .description = "Set the shadow map resolution for directional lights.",
        .params = {
            {.name = "resolution", .type = "number", .description = "Shadow map resolution in pixels (e.g., 1024, 2048, 4096)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setShadowDistance",
        .qualifiedName = "bestow.graphics3d.setShadowDistance",
        .description = "Set the maximum distance at which shadows are rendered.",
        .params = {
            {.name = "distance", .type = "number", .description = "Maximum shadow distance in world units"},
        },
    });

    // --- Methods: Debug Rendering ---

    sys.methods.push_back(MethodDoc{
        .name = "debugDrawLine",
        .qualifiedName = "bestow.graphics3d.debugDrawLine",
        .description = "Draw a debug line in world space.",
        .params = {
            {.name = "start", .type = "Vec3", .description = "Line start position"},
            {.name = "end", .type = "Vec3", .description = "Line end position"},
            {.name = "color", .type = "Color", .description = "Line color", .optional = true, .defaultVal = "Color.white()"},
            {.name = "duration", .type = "number", .description = "Time in seconds to persist (0 = single frame)", .optional = true, .defaultVal = "0"},
            {.name = "depthTest", .type = "boolean", .description = "Whether to depth test the line", .optional = true, .defaultVal = "true"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "debugDrawBox",
        .qualifiedName = "bestow.graphics3d.debugDrawBox",
        .description = "Draw a debug wireframe box in world space.",
        .params = {
            {.name = "center", .type = "Vec3", .description = "Box center position"},
            {.name = "halfExtents", .type = "Vec3", .description = "Half-size in each axis"},
            {.name = "rotation", .type = "Quat", .description = "Box rotation", .optional = true, .defaultVal = "identity"},
            {.name = "color", .type = "Color", .description = "Box color", .optional = true, .defaultVal = "Color.white()"},
            {.name = "duration", .type = "number", .description = "Time in seconds to persist", .optional = true, .defaultVal = "0"},
            {.name = "depthTest", .type = "boolean", .description = "Whether to depth test", .optional = true, .defaultVal = "true"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "debugDrawSphere",
        .qualifiedName = "bestow.graphics3d.debugDrawSphere",
        .description = "Draw a debug wireframe sphere in world space.",
        .params = {
            {.name = "center", .type = "Vec3", .description = "Sphere center position"},
            {.name = "radius", .type = "number", .description = "Sphere radius"},
            {.name = "color", .type = "Color", .description = "Sphere color", .optional = true, .defaultVal = "Color.white()"},
            {.name = "duration", .type = "number", .description = "Time in seconds to persist", .optional = true, .defaultVal = "0"},
            {.name = "depthTest", .type = "boolean", .description = "Whether to depth test", .optional = true, .defaultVal = "true"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "debugDrawRay",
        .qualifiedName = "bestow.graphics3d.debugDrawRay",
        .description = "Draw a debug ray in world space.",
        .params = {
            {.name = "origin", .type = "Vec3", .description = "Ray origin"},
            {.name = "direction", .type = "Vec3", .description = "Ray direction (normalized)"},
            {.name = "length", .type = "number", .description = "Ray length"},
            {.name = "color", .type = "Color", .description = "Ray color", .optional = true, .defaultVal = "Color.white()"},
            {.name = "duration", .type = "number", .description = "Time in seconds to persist", .optional = true, .defaultVal = "0"},
            {.name = "depthTest", .type = "boolean", .description = "Whether to depth test", .optional = true, .defaultVal = "true"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "debugDrawAxes",
        .qualifiedName = "bestow.graphics3d.debugDrawAxes",
        .description = "Draw RGB axes at a transform (red=X, green=Y, blue=Z).",
        .params = {
            {.name = "transform", .type = "Transform3D", .description = "Transform for the axes"},
            {.name = "size", .type = "number", .description = "Length of each axis", .optional = true, .defaultVal = "1.0"},
            {.name = "duration", .type = "number", .description = "Time in seconds to persist", .optional = true, .defaultVal = "0"},
            {.name = "depthTest", .type = "boolean", .description = "Whether to depth test", .optional = true, .defaultVal = "true"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "debugDrawAABB",
        .qualifiedName = "bestow.graphics3d.debugDrawAABB",
        .description = "Draw a debug wireframe axis-aligned bounding box.",
        .params = {
            {.name = "aabb", .type = "AABB3D", .description = "The bounding box to draw"},
            {.name = "color", .type = "Color", .description = "Box color", .optional = true, .defaultVal = "Color.white()"},
            {.name = "duration", .type = "number", .description = "Time in seconds to persist", .optional = true, .defaultVal = "0"},
            {.name = "depthTest", .type = "boolean", .description = "Whether to depth test", .optional = true, .defaultVal = "true"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "debugClear",
        .qualifiedName = "bestow.graphics3d.debugClear",
        .description = "Clear all debug draw primitives.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "setDebugRenderingEnabled",
        .qualifiedName = "bestow.graphics3d.setDebugRenderingEnabled",
        .description = "Enable or disable debug rendering.",
        .params = {
            {.name = "enabled", .type = "boolean", .description = "true to enable debug rendering"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isDebugRenderingEnabled",
        .qualifiedName = "bestow.graphics3d.isDebugRenderingEnabled",
        .description = "Check if debug rendering is enabled.",
        .returns = {{.type = "boolean", .description = "true if debug rendering is enabled"}},
    });

    // --- Methods: Window Management ---

    sys.methods.push_back(MethodDoc{
        .name = "getWindowSize",
        .qualifiedName = "bestow.graphics3d.getWindowSize",
        .description = "Get the current window size.",
        .returns = {{.type = "Size", .description = "Window size in pixels"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setWindowSize",
        .qualifiedName = "bestow.graphics3d.setWindowSize",
        .description = "Set the window size.",
        .params = {
            {.name = "size", .type = "Size", .description = "Desired window size in pixels"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isFullscreen",
        .qualifiedName = "bestow.graphics3d.isFullscreen",
        .description = "Check if the window is in fullscreen mode.",
        .returns = {{.type = "boolean", .description = "true if fullscreen"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setFullscreen",
        .qualifiedName = "bestow.graphics3d.setFullscreen",
        .description = "Set fullscreen mode.",
        .params = {
            {.name = "fullscreen", .type = "boolean", .description = "true for fullscreen, false for windowed"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "shouldClose",
        .qualifiedName = "bestow.graphics3d.shouldClose",
        .description = "Check if the window close button has been pressed.",
        .returns = {{.type = "boolean", .description = "true if the window should close"}},
    });

    // --- Methods: Render State ---

    sys.methods.push_back(MethodDoc{
        .name = "setClearColor",
        .qualifiedName = "bestow.graphics3d.setClearColor",
        .description = "Set the background clear color.",
        .params = {
            {.name = "color", .type = "Color", .description = "Clear color"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setVSync",
        .qualifiedName = "bestow.graphics3d.setVSync",
        .description = "Enable or disable vertical sync.",
        .params = {
            {.name = "enabled", .type = "boolean", .description = "true to enable VSync"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setRenderScale",
        .qualifiedName = "bestow.graphics3d.setRenderScale",
        .description = "Set the render resolution scale (1.0 = native resolution).",
        .params = {
            {.name = "scale", .type = "number", .description = "Render scale (0.25 to 2.0)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getRenderScale",
        .qualifiedName = "bestow.graphics3d.getRenderScale",
        .description = "Get the current render resolution scale.",
        .returns = {{.type = "number", .description = "Current render scale"}},
    });

    // --- Methods: Culling ---

    sys.methods.push_back(MethodDoc{
        .name = "setFrustumCulling",
        .qualifiedName = "bestow.graphics3d.setFrustumCulling",
        .description = "Enable or disable frustum culling.",
        .params = {
            {.name = "enabled", .type = "boolean", .description = "true to enable frustum culling"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isFrustumCullingEnabled",
        .qualifiedName = "bestow.graphics3d.isFrustumCullingEnabled",
        .description = "Check if frustum culling is enabled.",
        .returns = {{.type = "boolean", .description = "true if frustum culling is enabled"}},
    });

    // --- Methods: Post-Processing ---

    sys.methods.push_back(MethodDoc{
        .name = "setToneMapping",
        .qualifiedName = "bestow.graphics3d.setToneMapping",
        .description = "Enable or disable HDR tone mapping.",
        .params = {
            {.name = "enabled", .type = "boolean", .description = "true to enable tone mapping"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setExposure",
        .qualifiedName = "bestow.graphics3d.setExposure",
        .description = "Set the HDR exposure value.",
        .params = {
            {.name = "exposure", .type = "number", .description = "Exposure value"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setBloom",
        .qualifiedName = "bestow.graphics3d.setBloom",
        .description = "Configure bloom post-processing effect.",
        .params = {
            {.name = "enabled", .type = "boolean", .description = "true to enable bloom"},
            {.name = "threshold", .type = "number", .description = "Brightness threshold for bloom", .optional = true, .defaultVal = "1.0"},
            {.name = "intensity", .type = "number", .description = "Bloom intensity", .optional = true, .defaultVal = "1.0"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setSSAO",
        .qualifiedName = "bestow.graphics3d.setSSAO",
        .description = "Configure screen-space ambient occlusion.",
        .params = {
            {.name = "enabled", .type = "boolean", .description = "true to enable SSAO"},
            {.name = "radius", .type = "number", .description = "Sampling radius", .optional = true, .defaultVal = "0.5"},
            {.name = "intensity", .type = "number", .description = "SSAO intensity", .optional = true, .defaultVal = "1.0"},
        },
    });

    // --- Methods: Statistics ---

    sys.methods.push_back(MethodDoc{
        .name = "getStats",
        .qualifiedName = "bestow.graphics3d.getStats",
        .description = "Get per-frame rendering statistics.",
        .returns = {{.type = "RenderStats", .description = "Current frame statistics"}},
    });

    // --- Methods: Skeletal Animation (Graphics-side) ---

    sys.methods.push_back(MethodDoc{
        .name = "getAnimationClipNames",
        .qualifiedName = "bestow.graphics3d.getAnimationClipNames",
        .description = "Get the names of animation clips associated with a skeleton.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton handle"},
        },
        .returns = {{.type = "string[]", .description = "Array of clip names"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAnimationClipInfo",
        .qualifiedName = "bestow.graphics3d.getAnimationClipInfo",
        .description = "Get information about an animation clip.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Animation clip handle"},
        },
        .returns = {{.type = "AnimationClip", .description = "Clip information"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "destroySkeleton",
        .qualifiedName = "bestow.graphics3d.destroySkeleton",
        .description = "Destroy a skeleton and free its resources.",
        .params = {
            {.name = "skeleton", .type = "SkeletonHandle", .description = "Skeleton handle to destroy"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "destroyAnimationClip",
        .qualifiedName = "bestow.graphics3d.destroyAnimationClip",
        .description = "Destroy an animation clip and free its resources.",
        .params = {
            {.name = "clip", .type = "AnimationClipHandle", .description = "Animation clip handle to destroy"},
        },
    });

    // --- Methods: 3D Text Rendering ---

    sys.methods.push_back(MethodDoc{
        .name = "destroyFont3D",
        .qualifiedName = "bestow.graphics3d.destroyFont3D",
        .description = "Destroy a 3D font and free its resources.",
        .params = {
            {.name = "font", .type = "Font3DHandle", .description = "Font handle to destroy"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "drawText3D",
        .qualifiedName = "bestow.graphics3d.drawText3D",
        .description = "Draw 3D text in the scene. Accepts either a Text3DItem or individual parameters (text, position, font, fontSize, color).",
        .params = {
            {.name = "itemOrText", .type = "Text3DItem|string", .description = "A Text3DItem object, or the text string"},
            {.name = "position", .type = "Vec3", .description = "World position (only when passing individual params)", .optional = true},
            {.name = "font", .type = "Font3DHandle", .description = "Font handle (only when passing individual params)", .optional = true},
            {.name = "fontSize", .type = "number", .description = "Font size", .optional = true, .defaultVal = "1.0"},
            {.name = "color", .type = "Color", .description = "Text color", .optional = true, .defaultVal = "Color.white()"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "measureText3D",
        .qualifiedName = "bestow.graphics3d.measureText3D",
        .description = "Measure the bounds of 3D text without drawing it.",
        .params = {
            {.name = "text", .type = "string", .description = "Text to measure"},
            {.name = "font", .type = "Font3DHandle", .description = "Font handle"},
            {.name = "fontSize", .type = "number", .description = "Font size", .optional = true, .defaultVal = "1.0"},
        },
        .returns = {{.type = "Vec2", .description = "Text bounds (width, height)"}},
    });

    // --- Methods: LOD ---

    sys.methods.push_back(MethodDoc{
        .name = "setLODBias",
        .qualifiedName = "bestow.graphics3d.setLODBias",
        .description = "Set the level-of-detail bias. Positive values use lower detail, negative values use higher detail.",
        .params = {
            {.name = "bias", .type = "number", .description = "LOD bias value"},
        },
    });

    // --- Methods: Lock-On Targeting System ---

    sys.methods.push_back(MethodDoc{
        .name = "setLockOnConfig",
        .qualifiedName = "bestow.graphics3d.setLockOnConfig",
        .description = "Configure the lock-on targeting system. Accepts a LockOnConfig object or a table.",
        .params = {
            {.name = "config", .type = "LockOnConfig|table", .description = "Lock-on configuration with fields: maxRange, fovMargin, centerBias, priorityWeight, preferCurrentTarget, hysteresis"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getLockOnConfig",
        .qualifiedName = "bestow.graphics3d.getLockOnConfig",
        .description = "Get the current lock-on targeting configuration.",
        .returns = {{.type = "LockOnConfig", .description = "Current lock-on configuration"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "lockOn",
        .qualifiedName = "bestow.graphics3d.lockOn",
        .description = "Acquire a lock-on target. Finds the best target based on current camera view and configuration.",
        .returns = {{.type = "LockOnResult|nil", .description = "Lock-on result with target info, or nil if no valid target found"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getLockTarget",
        .qualifiedName = "bestow.graphics3d.getLockTarget",
        .description = "Get the current lock-on target, if any.",
        .returns = {{.type = "LockOnResult|nil", .description = "Current lock target, or nil if not locked"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "pollLockPosition",
        .qualifiedName = "bestow.graphics3d.pollLockPosition",
        .description = "Get the current world position of the locked target (updated for animation/movement).",
        .returns = {{.type = "Vec3|nil", .description = "Updated world position, or nil if not locked"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "shiftLockTarget",
        .qualifiedName = "bestow.graphics3d.shiftLockTarget",
        .description = "Shift the lock-on target in a screen-space direction (e.g., for cycling targets with right stick).",
        .params = {
            {.name = "direction", .type = "Vec2", .description = "Screen-space direction to shift (e.g., Vec2(1,0) for right)"},
        },
        .returns = {{.type = "LockOnResult|nil", .description = "New lock target, or nil if no valid target"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "unlock",
        .qualifiedName = "bestow.graphics3d.unlock",
        .description = "Release the current lock-on target.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "isLocked",
        .qualifiedName = "bestow.graphics3d.isLocked",
        .description = "Check if currently locked onto a target.",
        .returns = {{.type = "boolean", .description = "true if locked on"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getPotentialTargets",
        .qualifiedName = "bestow.graphics3d.getPotentialTargets",
        .description = "Get all potential lock-on targets in range, sorted by score.",
        .returns = {{.type = "LockOnResult[]", .description = "Array of potential targets"}},
    });

    // --- Methods: LockableTarget Component Helpers ---

    sys.methods.push_back(MethodDoc{
        .name = "addLockableTarget",
        .qualifiedName = "bestow.graphics3d.addLockableTarget",
        .description = "Add a LockableTarget component to an entity, making it targetable by the lock-on system.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "Entity to add the component to"},
            {.name = "target", .type = "LockableTarget|table", .description = "Lock target configuration", .optional = true},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getLockableTarget",
        .qualifiedName = "bestow.graphics3d.getLockableTarget",
        .description = "Get the LockableTarget component from an entity.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "Entity to query"},
        },
        .returns = {{.type = "LockableTarget|nil", .description = "The LockableTarget component, or nil if not present"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasLockableTarget",
        .qualifiedName = "bestow.graphics3d.hasLockableTarget",
        .description = "Check if an entity has a LockableTarget component.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "Entity to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the entity has a LockableTarget"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeLockableTarget",
        .qualifiedName = "bestow.graphics3d.removeLockableTarget",
        .description = "Remove the LockableTarget component from an entity.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "Entity to remove the component from"},
        },
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
