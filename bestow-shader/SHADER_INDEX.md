# Bestow Shader Library - Quick Reference Index

Complete built-in shader library for the Bestow game engine.

**Total Shaders: 44 shader programs (52 files)**
**Total Material Presets: 8 Lua files**

## Quick Find by Use Case

### For Character Rendering
- **Toon/Anime Look**: `cel/toon` or `cel/anime`
- **Comic Book Style**: `cel/comic`
- **Skin Material**: `materials/skin`

### For Stylized Games
- **Watercolor**: `painterly/watercolor`
- **Oil Painting**: `painterly/oilpaint`
- **Impressionist**: `painterly/impressionist`

### For Retro Games
- **Pixel Art**: `retro/pixelart`
- **CRT Monitor**: `retro/crt`
- **Dithered Graphics**: `retro/dither`

### For Sci-Fi Games
- **Hologram**: `effects/hologram` (Material: `hologram.lua`)
- **Force Field**: `effects/forcefield` (Material: `forcefield.lua`)
- **Energy Beams**: `effects/energy`
- **Portal**: `effects/portal`

### For Visual Effects
- **Glowing Objects**: `effects/glow` (Material: `glow.lua`)
- **Dissolve/Death**: `effects/dissolve` (Material: `dissolve.lua`)
- **Fire**: `effects/fire`
- **Outline/Silhouette**: `effects/outline`
- **Rim Lighting**: `effects/fresnel`

### For Environment
- **Water**: `materials/water` (Material: `water.lua`)
- **Lava**: `materials/lava` (Material: `lava.lua`)
- **Ice**: `materials/ice`
- **Glass**: `materials/glass`
- **Metal**: `materials/metal`

### For Post-Processing
- **Bloom/Glow**: `post/bloom`
- **Blur**: `post/blur`
- **Vignette**: `post/vignette`
- **Chromatic Aberration**: `post/chromatic`
- **Color Grading**: `post/grayscale`, `post/sepia`

### For Debugging
- **Show Normals**: `debug/normals`
- **Show Depth**: `debug/depth`
- **Show UVs**: `debug/uv`
- **Wireframe**: `debug/wireframe`
- **Solid Color**: `debug/solid`

## All Shaders by Category

### Stylized Rendering (9 shaders)

| Shader | Files | Description | Material Preset |
|--------|-------|-------------|-----------------|
| Toon | `cel/toon.vert`, `cel/toon.frag` | Classic cel-shading | `toon.lua` |
| Comic | `cel/comic.vert`, `cel/comic.frag` | Comic book halftone | `comic.lua` |
| Anime | `cel/anime.vert`, `cel/anime.frag` | Anime-style shading | - |
| Watercolor | `painterly/watercolor.vert`, `watercolor.frag` | Watercolor painting | - |
| Oil Paint | `painterly/oilpaint.vert`, `oilpaint.frag` | Oil painting texture | - |
| Impressionist | `painterly/impressionist.frag` | Impressionist strokes (post) | - |
| Pixel Art | `retro/pixelart.frag` | Pixelated retro (post) | - |
| CRT | `retro/crt.frag` | CRT monitor (post) | - |
| Dither | `retro/dither.frag` | Ordered dithering (post) | - |

### Special Effects (9 shaders)

| Shader | Files | Description | Material Preset |
|--------|-------|-------------|-----------------|
| Outline | `effects/outline.vert`, `outline.frag` | Object silhouette | - |
| Glow | `effects/glow.frag` | Emissive pulsing glow | `glow.lua` |
| Hologram | `effects/hologram.frag` | Sci-fi hologram | `hologram.lua` |
| Dissolve | `effects/dissolve.frag` | Burning dissolve | `dissolve.lua` |
| Fresnel | `effects/fresnel.frag` | Rim lighting | - |
| Force Field | `effects/forcefield.frag` | Energy shield | `forcefield.lua` |
| Portal | `effects/portal.frag` | Swirling vortex | - |
| Fire | `effects/fire.frag` | Procedural flames | - |
| Energy | `effects/energy.frag` | Plasma/laser beam | - |

### Materials (7 shaders)

| Shader | Files | Description | Material Preset |
|--------|-------|-------------|-----------------|
| Glass | `materials/glass.frag` | Transparent glass | - |
| Water | `materials/water.vert`, `water.frag` | Animated water | `water.lua` |
| Ice | `materials/ice.frag` | Frozen surface | - |
| Metal | `materials/metal.frag` | Stylized metal | - |
| Fabric | `materials/fabric.frag` | Cloth/velvet | - |
| Skin | `materials/skin.frag` | Subsurface scattering | - |
| Lava | `materials/lava.frag` | Molten lava | `lava.lua` |

### Debug (6 shaders)

| Shader | Files | Description |
|--------|-------|-------------|
| Wireframe | `debug/wireframe.frag` | Show triangle edges |
| Normals | `debug/normals.frag` | Visualize normals as RGB |
| Depth | `debug/depth.frag` | Show depth buffer |
| UV | `debug/uv.frag` | Show UV coordinates |
| Grid | `debug/grid.frag` | Checkerboard pattern |
| Solid | `debug/solid.frag` | Flat color |

### Post-Processing (7 shaders)

| Shader | Files | Description |
|--------|-------|-------------|
| Blur | `post/blur.frag` | Gaussian blur |
| Bloom | `post/bloom.frag` | Bloom/glow effect |
| Vignette | `post/vignette.frag` | Edge darkening |
| Grayscale | `post/grayscale.frag` | Black & white |
| Sepia | `post/sepia.frag` | Sepia tone |
| Chromatic | `post/chromatic.frag` | Chromatic aberration |
| Sharpen | `post/sharpen.frag` | Sharpening filter |

### Utilities (6 shaders)

| Shader | Files | Description |
|--------|-------|-------------|
| Basic | `util/basic.vert` | Standard MVP transform |
| Skinned | `util/skinned.vert` | Skeletal animation |
| Instanced | `util/instanced.vert` | GPU instancing |
| Billboard | `util/billboard.vert` | Camera-facing sprites |
| Skybox | `util/skybox.vert`, `skybox.frag` | Cubemap skybox |
| Shadow | `util/shadow.vert`, `shadow.frag` | Shadow map generation |

## Material Presets

Pre-configured Lua material files for common use cases:

1. **`toon.lua`** - Classic cel-shaded look
2. **`comic.lua`** - Comic book with halftone
3. **`hologram.lua`** - Sci-fi holographic display
4. **`glow.lua`** - Glowing emissive object
5. **`water.lua`** - Realistic animated water
6. **`lava.lua`** - Flowing molten lava
7. **`dissolve.lua`** - Burning dissolve effect
8. **`forcefield.lua`** - Energy shield barrier

## Usage Examples

### Loading a Shader Pair

```cpp
auto result = shaderSystem->loadShader(
    "shaders/cel/toon.vert",
    "shaders/cel/toon.frag",
    true  // Enable hot reload
);
```

### Loading a Complete Material

```cpp
auto result = shaderSystem->loadMaterial("materials/toon.lua");
if (result) {
    MaterialHandle mat = *result;
    shaderSystem->bindMaterial(mat);
}
```

### Using Post-Process Shader

```cpp
// Post-process shaders typically only need a fragment shader
// Use with a fullscreen quad
auto result = shaderSystem->loadShader(
    "shaders/util/basic.vert",  // Use basic vert for fullscreen quad
    "shaders/post/bloom.frag",
    false
);
```

## Shader Compatibility

All shaders use:
- **GLSL Version**: 4.10 Core (macOS compatible)
- **Standard Uniforms**: Model, View, Projection, Time, Camera, Light
- **Hot Reload**: Supported for all file-based shaders
- **Texture Binding**: Standard sampler2D/samplerCube

## Directory Structure

```
bestow-shader/
├── shaders/
│   ├── cel/              # 6 files - Cel-shading
│   ├── painterly/        # 5 files - Painting effects
│   ├── retro/            # 3 files - Retro effects
│   ├── effects/          # 11 files - Special effects
│   ├── materials/        # 8 files - Physical materials
│   ├── debug/            # 6 files - Debug visualization
│   ├── post/             # 7 files - Post-processing
│   ├── util/             # 8 files - Utility shaders
│   └── README.md         # Full documentation
├── materials/            # 8 Lua material presets
└── SHADER_INDEX.md       # This file
```

## Feature Matrix

| Feature | Shaders |
|---------|---------|
| Animated | water, lava, fire, energy, portal, hologram, glow |
| Transparent | glass, water, ice, hologram, forcefield, dissolve |
| Emissive | glow, lava, fire, energy, hologram |
| Procedural | fire, energy, portal, dissolve, lava, water |
| Post-Process | All in `post/`, `retro/`, `painterly/impressionist` |
| Vertex Animation | water, outline |
| Requires Vertex Shader | All except post-process and some effects |

## Common Parameters

Most shaders share these common uniforms:

```glsl
// Transformations
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

// Lighting
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uCameraPos;

// Material
uniform vec4 uBaseColor;
uniform bool uUseTexture;
uniform sampler2D uTexture;

// Animation
uniform float uTime;
```

## Performance Classification

- **Low Cost**: debug/*, util/basic, util/shadow
- **Medium Cost**: cel/*, materials/metal, materials/fabric
- **High Cost**: painterly/*, materials/water, effects/dissolve
- **Very High Cost**: retro/crt, post/bloom, effects/fire

## Notes

- Post-processing shaders require a framebuffer texture as input
- Some effects (hologram, forcefield, dissolve) require alpha blending
- Water shader animates vertices in vertex shader
- Outline shader requires two-pass rendering
- Shadow shaders output to depth buffer only
- Debug shaders are for development/testing only

---

For detailed documentation on each shader, see `shaders/README.md`
