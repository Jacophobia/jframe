# Bestow Shader Library

A comprehensive collection of production-ready GLSL shaders for the Bestow game engine, covering stylized rendering, special effects, materials, and utilities.

## Overview

All shaders are written in **GLSL 4.10 Core** for macOS compatibility and use modern shader techniques. The library is organized into the following categories:

- **Stylized Rendering** - Cel-shading, painterly effects, retro looks
- **Special Effects** - Outlines, glows, holograms, dissolve, portals
- **Materials** - Glass, water, ice, metal, fabric, skin, lava
- **Debug/Development** - Wireframe, normals, depth, UV visualization
- **Post-Processing** - Blur, bloom, vignette, chromatic aberration
- **Utilities** - Basic transforms, skinning, instancing, billboards, skybox, shadows

## Directory Structure

```
shaders/
├── cel/              # Cel-shading and toon styles
├── painterly/        # Artistic painting effects
├── retro/            # Retro/vintage effects
├── effects/          # Special visual effects
├── materials/        # Physical material shaders
├── debug/            # Debug visualization
├── post/             # Post-processing effects
└── util/             # Utility shaders
```

## Shader Catalog

### Stylized Rendering

#### Cel/Toon Shaders

**`cel/toon.vert` + `cel/toon.frag`**
- Classic cel-shading with discrete light bands
- Configurable parameters:
  - `uBands`: Number of discrete light levels (2-8)
  - `uRimPower`: Rim light intensity
  - `uRimColor`: Rim light color
- Use case: Anime/cartoon character rendering

**`cel/comic.vert` + `cel/comic.frag`**
- Comic book style with halftone dots and bold outlines
- Configurable parameters:
  - `uHalftoneScale`: Dot pattern size
  - `uOutlineThickness`: Ink outline width
  - `uColorBoost`: Saturation enhancement
- Use case: Comic book aesthetics

**`cel/anime.vert` + `cel/anime.frag`**
- Anime-style with soft gradients and sharp specular
- Configurable parameters:
  - `uShadowSoftness`: Shadow transition smoothness
  - `uSpecularPower`: Highlight sharpness
  - `uSpecularStrength`: Highlight intensity
- Use case: Anime character rendering

#### Painterly Shaders

**`painterly/watercolor.vert` + `watercolor.frag`**
- Watercolor painting effect with paper texture
- Configurable parameters:
  - `uPaperScale`: Paper texture detail
  - `uPigmentDensity`: Paint accumulation
  - `uWaterBleed`: Color bleeding amount
- Use case: Artistic watercolor look

**`painterly/oilpaint.vert` + `oilpaint.frag`**
- Oil painting with visible brush strokes and impasto
- Configurable parameters:
  - `uBrushScale`: Brush stroke size
  - `uImpasto`: Paint thickness
  - `uColorVariation`: Random color variation per stroke
- Use case: Oil painting aesthetic

**`painterly/impressionist.frag`**
- Impressionist style with dabs of color (post-process)
- Configurable parameters:
  - `uStrokeSize`: Brush dab size
  - `uColorJitter`: Color randomness
  - `uStrokeIntensity`: Effect strength
- Use case: Impressionist art filter

#### Retro Shaders

**`retro/pixelart.frag`**
- Pixelated/low-res effect with color quantization
- Configurable parameters:
  - `uPixelSize`: Pixel grid size
  - `uColorDepth`: Bits per color channel
  - `uDithering`: Enable ordered dithering
- Use case: Retro pixel art look

**`retro/crt.frag`**
- CRT monitor with scanlines and screen curvature
- Configurable parameters:
  - `uScanlineIntensity`: Scanline darkness
  - `uCurvature`: Screen bend amount
  - `uVignetteStrength`: Corner darkening
  - `uBrightness`: Overall brightness boost
- Use case: Retro CRT display simulation

**`retro/dither.frag`**
- Classic ordered dithering for low-color palettes
- Configurable parameters:
  - `uColors`: Number of colors per channel
  - `uDitherScale`: Dither pattern scale
- Use case: Old-school dithered graphics

### Special Effects

**`effects/outline.vert` + `outline.frag`**
- Object silhouette/outline rendering
- Usage: Render first with backface culling off, then normal object
- Configurable: `uOutlineWidth`, `uOutlineColor`

**`effects/glow.frag`**
- Emissive glow with pulse animation
- Configurable parameters:
  - `uGlowColor`, `uGlowIntensity`
  - `uPulseSpeed`, `uPulseAmount`
- Use case: Glowing objects, pickups, power-ups

**`effects/hologram.frag`**
- Sci-fi hologram with scanlines and glitches
- Configurable parameters:
  - `uHologramColor`, `uScanlineSpeed`
  - `uGlitchIntensity`, `uFlickerSpeed`
- Use case: Futuristic UI, holographic displays

**`effects/dissolve.frag`**
- Burning dissolve/disintegration effect
- Configurable parameters:
  - `uDissolveAmount` (0=solid, 1=gone)
  - `uEdgeWidth`, `uEdgeColor`
  - `uNoiseScale`
- Use case: Death effects, teleportation

**`effects/fresnel.frag`**
- Fresnel rim lighting effect
- Configurable: `uRimColor`, `uRimPower`, `uRimIntensity`
- Use case: Energy shields, magic auras

**`effects/forcefield.frag`**
- Sci-fi force field with hexagonal pattern
- Configurable parameters:
  - `uFieldColor`, `uHexScale`
  - `uPulseSpeed`, `uIntersectionGlow`
- Use case: Energy barriers, shields

**`effects/portal.frag`**
- Swirling vortex portal effect
- Configurable parameters:
  - `uPortalColor1`, `uPortalColor2`
  - `uRotationSpeed`, `uDistortionAmount`
- Use case: Portal gates, wormholes

**`effects/fire.frag`**
- Procedural fire using fractal noise
- Configurable: `uFireSpeed`, `uFireIntensity`, `uFlameHeight`
- Use case: Fire effects, torches

**`effects/energy.frag`**
- Energy beam/plasma effect
- Configurable: `uEnergyColor`, `uCoreColor`, `uFlowSpeed`, `uTurbulence`
- Use case: Laser beams, lightning, plasma

### Material Shaders

**`materials/glass.frag`**
- Transparent glass with refraction
- Configurable: `uRefractiveIndex`, `uThickness`, `uTint`
- Use case: Windows, glass objects

**`materials/water.vert` + `water.frag`**
- Animated water with waves and foam
- Vertex shader: Wave animation
- Fragment shader: Reflections, foam, depth
- Configurable: `uWaveHeight`, `uWaveFrequency`, `uWaterColor`, `uShallowColor`
- Use case: Lakes, oceans, rivers

**`materials/ice.frag`**
- Frozen surface with cracks and frost
- Configurable: `uIceColor`, `uCrackDensity`, `uFrostAmount`
- Use case: Ice blocks, frozen water

**`materials/metal.frag`**
- Stylized metallic surface
- Configurable: `uMetalColor`, `uRoughness`, `uAnisotropy`
- Use case: Metal objects without full PBR

**`materials/fabric.frag`**
- Cloth/fabric with velvet sheen
- Configurable: `uFabricColor`, `uRoughness`, `uSheenColor`, `uSheenIntensity`
- Use case: Clothing, curtains, tapestries

**`materials/skin.frag`**
- Skin with subsurface scattering
- Configurable: `uSkinColor`, `uSubsurfaceColor`, `uScatterAmount`
- Use case: Character skin

**`materials/lava.frag`**
- Flowing molten lava
- Configurable: `uLavaColor1`, `uLavaColor2`, `uFlowSpeed`, `uGlowIntensity`
- Use case: Lava pools, volcanic effects

### Debug Shaders

**`debug/wireframe.frag`** - Shows triangle edges (approximation)
**`debug/normals.frag`** - Visualizes normals as RGB colors
**`debug/depth.frag`** - Shows depth as grayscale
**`debug/uv.frag`** - Displays UV coordinates as colors
**`debug/grid.frag`** - Checkerboard pattern for testing
**`debug/solid.frag`** - Flat color for debugging

### Post-Processing Shaders

**`post/blur.frag`** - Gaussian blur (two-pass)
**`post/bloom.frag`** - Bloom/glow effect (extract + combine)
**`post/vignette.frag`** - Darkens screen edges
**`post/grayscale.frag`** - Black and white conversion
**`post/sepia.frag`** - Sepia tone filter
**`post/chromatic.frag`** - Chromatic aberration (lens effect)
**`post/sharpen.frag`** - Sharpening filter

### Utility Shaders

**`util/basic.vert`** - Standard MVP vertex shader with all attributes
**`util/skinned.vert`** - Skeletal animation (4 bones per vertex)
**`util/instanced.vert`** - GPU instancing with per-instance transforms
**`util/billboard.vert`** - Billboard/sprite rendering (camera-facing)
**`util/skybox.vert` + `skybox.frag`** - Skybox rendering with cubemap
**`util/shadow.vert` + `shadow.frag`** - Shadow map generation

## Common Uniforms

Most shaders expect these standard uniforms:

### Transformation Matrices
- `mat4 uModel` - Model matrix
- `mat4 uView` - View matrix
- `mat4 uProjection` - Projection matrix
- `mat3 uNormalMatrix` - Normal transformation matrix

### Lighting
- `vec3 uLightDir` - Directional light direction
- `vec3 uLightColor` - Light color/intensity
- `vec3 uAmbientColor` - Ambient light color
- `vec3 uCameraPos` - Camera world position

### Material Properties
- `vec4 uBaseColor` - Base color/tint
- `bool uUseTexture` - Whether to sample texture
- `sampler2D uTexture` - Main texture sampler

### Animation
- `float uTime` - Time in seconds (for animated effects)

## Usage Examples

### Loading a Shader in C++

```cpp
// Load shader from files
auto shaderResult = shaderSystem->loadShader(
    "shaders/cel/toon.vert",
    "shaders/cel/toon.frag",
    true  // Enable hot reload
);

if (shaderResult) {
    ShaderProgramHandle shader = *shaderResult;

    // Use the shader
    shaderSystem->bindShader(shader);
    shaderSystem->setUniform("uBands", 4);
    shaderSystem->setUniform("uRimPower", 3.0f);
}
```

### Loading a Material from Lua

```cpp
// Load complete material definition
auto matResult = shaderSystem->loadMaterial("materials/toon.lua");

if (matResult) {
    MaterialHandle material = *matResult;

    // Material contains shader + uniforms + textures + render state
    shaderSystem->bindMaterial(material);

    // Override uniforms if needed
    shaderSystem->setMaterialUniform(material, "uBands", 5);
}
```

### Using in Render Loop

```cpp
void render() {
    // Bind material (sets shader, uniforms, and render state)
    shaderSystem->bindMaterial(toonMaterial);

    // Set per-frame uniforms
    shaderSystem->setUniform("uModel", modelMatrix);
    shaderSystem->setUniform("uView", viewMatrix);
    shaderSystem->setUniform("uProjection", projectionMatrix);
    shaderSystem->setUniform("uTime", currentTime);

    // Draw mesh
    mesh->draw();
}
```

## Hot Reload

All shaders support hot reloading during development:

1. Enable hot reload when loading:
   ```cpp
   shaderSystem->loadShader("shader.vert", "shader.frag", true);
   ```

2. Call `update()` each frame:
   ```cpp
   shaderSystem->update();  // Checks for file changes
   ```

3. Register callbacks for reload events:
   ```cpp
   shaderSystem->setShaderReloadCallback([](ShaderProgramHandle h, bool success, std::string error) {
       if (success) {
           std::cout << "Shader reloaded successfully\n";
       } else {
           std::cerr << "Shader reload failed: " << error << "\n";
       }
   });
   ```

## Best Practices

1. **Use Materials for Art Assets** - Define materials in Lua for artist-friendly tweaking
2. **Reuse Shaders** - Create multiple materials from the same shader with different parameters
3. **Hot Reload** - Keep hot reload enabled during development, disable in release builds
4. **Uniform Caching** - The shader system caches uniform locations for performance
5. **Post-Process Order** - Apply post-processing in this order: Blur → Bloom → Tone Mapping → Vignette → CRT/Retro
6. **Transparency Sorting** - Render opaque objects first, then transparent objects back-to-front

## Performance Tips

- **Minimize Shader Switches** - Batch objects by material
- **Use Instancing** - For many similar objects, use `util/instanced.vert`
- **Simplify Post-Processing** - Expensive effects like bloom should be optional
- **LOD Shaders** - Use simpler shaders for distant objects
- **Conditional Features** - Use `#ifdef` or uniforms to disable expensive features

## Extending the Library

To add a new shader:

1. Create `.vert` and `.frag` files in appropriate category folder
2. Follow GLSL 4.10 Core syntax
3. Document uniforms and parameters in comments
4. Create a Lua material file in `materials/` for common use cases
5. Test with hot reload enabled
6. Add entry to this README

## License

Part of the Bestow game engine. See main project LICENSE file.
