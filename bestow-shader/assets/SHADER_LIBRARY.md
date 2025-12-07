# Bestow Shader Library

A comprehensive collection of 49+ GLSL shaders for the Bestow game engine, covering stylized rendering, materials, effects, debugging, post-processing, and environments.

## Directory Structure

```
bestow-shader/assets/
├── shaders/           # GLSL shader files (.vert, .frag)
└── materials/         # Lua material templates (.lua)
```

## Shader Categories

### 1. Stylized/Artistic Shaders (9 shaders)

#### Toon Shader
- **Files:** `shaders/toon.frag`, `materials/toon.lua`
- **Description:** Cel-shading with configurable bands, outlines, and rim lighting
- **Parameters:**
  - `uBands` - Number of lighting bands (default: 3)
  - `uOutlineWidth` - Outline thickness (default: 0.03)
  - `uSpecularSize` - Specular highlight size (default: 0.9)
  - `uShadowTint` - Shadow color tint
  - `uSmoothness` - Band edge smoothness

#### Hatching Shader
- **Files:** `shaders/hatching.frag`, `materials/hatching.lua`
- **Description:** Cross-hatching for pen-and-ink sketch look
- **Parameters:**
  - `uHatchDensity` - Lines per unit (default: 20.0)
  - `uLineThickness` - Line width (default: 0.3)
  - `uHatchLayers` - Cross-hatch directions (default: 3)
  - `uPaperColor`, `uInkColor` - Color scheme

#### Painterly Shader
- **Files:** `shaders/painterly.frag`, `materials/painterly.lua`
- **Description:** Oil painting/brushstroke effect
- **Parameters:**
  - `uBrushSize` - Brush stroke size (default: 0.05)
  - `uBrushStrength` - Texture strength (default: 0.3)
  - `uColorVariation` - Color variation (default: 0.15)
  - `uImpasto` - Thickness effect (default: 0.2)

#### Comic Shader
- **Files:** `shaders/comic.frag`, `materials/comic.lua`
- **Description:** Comic book style with halftone dots and bold outlines
- **Parameters:**
  - `uDotDensity` - Halftone dot density (default: 30.0)
  - `uOutlineWidth` - Outline width (default: 0.05)
  - `uDotContrast` - Dot contrast (default: 0.8)

#### Watercolor Shader
- **Files:** `shaders/watercolor.frag`, `materials/watercolor.lua`
- **Description:** Watercolor paint effect with soft edges
- **Parameters:**
  - `uBleedAmount` - Color bleeding (default: 0.3)
  - `uPaperTexture` - Paper grain strength (default: 0.2)
  - `uEdgeDarkening` - Dark edges (default: 0.4)
  - `uPigmentDensity` - Pigment concentration (default: 0.6)

#### Pixel Art Shader
- **Files:** `shaders/pixelart.frag`, `materials/pixelart.lua`
- **Description:** Retro pixel art with dithering
- **Parameters:**
  - `uPaletteSize` - Number of colors (default: 8)
  - `uPixelSize` - Pixel grid size (default: 0.1)
  - `uDithering` - Enable dithering (default: true)

#### Anime Shader
- **Files:** `shaders/anime.frag`, `materials/anime.lua`
- **Description:** Anime-style with sharp highlights and strong rim lighting
- **Parameters:**
  - `uShadowColor` - Shadow color tint
  - `uShadowSharpness` - Shadow edge sharpness (default: 0.02)
  - `uRimPower`, `uRimIntensity` - Rim lighting controls
  - `uSpecSharpness` - Specular sharpness (default: 0.98)

#### Oil Paint Shader
- **Files:** `shaders/oilpaint.frag`, `materials/oilpaint.lua`
- **Description:** Thick oil painting effect
- **Parameters:**
  - `uBrushSize` - Brush stroke size (default: 0.08)
  - `uThickness` - Paint thickness (default: 0.4)
  - `uGlossiness` - Surface glossiness (default: 0.3)

#### Posterize Shader
- **Files:** `shaders/posterize.frag`, `materials/posterize.lua`
- **Description:** Limited color palette effect
- **Parameters:**
  - `uLevels` - Color levels per channel (default: 4)
  - `uEdgeStrength` - Edge detection strength (default: 0.2)
  - `uQuantizeHSV` - Quantize in HSV space (default: false)

---

### 2. Material Shaders (12 shaders)

#### PBR Shader
- **Files:** `shaders/pbr.frag`, `materials/pbr.lua`
- **Description:** Physically Based Rendering with Cook-Torrance BRDF
- **Parameters:**
  - `uMetallic` - Metallic factor (default: 0.0)
  - `uRoughness` - Surface roughness (default: 0.5)
  - `uAO` - Ambient occlusion (default: 1.0)
  - `uF0` - Base reflectivity

#### Metallic Shader
- **Files:** `shaders/metallic.frag`, `materials/metallic.lua`
- **Description:** Enhanced metallic surfaces
- **Parameters:**
  - `uRoughness` - Surface roughness (default: 0.2)
  - `uTintColor` - Metallic tint
  - `uClearcoat` - Clearcoat layer (default: 0.0)

#### Glass Shader
- **Files:** `shaders/glass.frag`, `materials/glass.lua`
- **Description:** Transparent glass with refraction
- **Parameters:**
  - `uRefractiveIndex` - IOR (default: 1.5)
  - `uThickness` - Glass thickness (default: 0.1)
  - `uRoughness` - Surface roughness (default: 0.05)
  - `uOpacity` - Transparency (default: 0.3)

#### Water Shader (Animated)
- **Files:** `shaders/water.frag`, `materials/water.lua`
- **Description:** Animated water surface with waves and foam
- **Parameters:**
  - `uWaveSpeed`, `uWaveFrequency`, `uWaveAmplitude` - Wave animation
  - `uShallowColor`, `uDeepColor` - Water colors
  - `uFoamAmount` - Foam on peaks (default: 0.3)
  - `uTransparency` - Water transparency (default: 0.7)

#### Ice Shader
- **Files:** `shaders/ice.frag`, `materials/ice.lua`
- **Description:** Frozen/crystalline material
- **Parameters:**
  - `uFrostiness` - Frost amount (default: 0.5)
  - `uRefractiveIndex` - IOR (default: 1.31)
  - `uCrystalSize` - Crystal grain size (default: 0.1)

#### Fabric Shader
- **Files:** `shaders/fabric.frag`, `materials/fabric.lua`
- **Description:** Cloth with subsurface scattering
- **Parameters:**
  - `uRoughness` - Surface roughness (default: 0.8)
  - `uSheenColor`, `uSheenAmount` - Sheen controls
  - `uFuzziness` - Fabric fuzz (default: 0.5)
  - `uSubsurfaceColor` - Subsurface color

#### Leather Shader
- **Files:** `shaders/leather.frag`, `materials/leather.lua`
- **Description:** Leather with pores and wrinkles
- **Parameters:**
  - `uRoughness` - Surface roughness (default: 0.6)
  - `uPoreSize` - Pore size (default: 0.02)
  - `uWrinkleAmount` - Wrinkle intensity (default: 0.3)
  - `uGlossiness` - Surface gloss (default: 0.2)

#### Wood Shader
- **Files:** `shaders/wood.frag`, `materials/wood.lua`
- **Description:** Procedural wood grain
- **Parameters:**
  - `uDarkWoodColor`, `uLightWoodColor` - Grain colors
  - `uGrainFrequency` - Ring frequency (default: 5.0)
  - `uGrainVariation` - Irregularity (default: 0.3)

#### Marble Shader
- **Files:** `shaders/marble.frag`, `materials/marble.lua`
- **Description:** Procedural marble/stone
- **Parameters:**
  - `uVeinColor`, `uBaseStoneColor` - Colors
  - `uVeinFrequency` - Pattern frequency (default: 3.0)
  - `uVeinComplexity` - Complexity (default: 0.5)
  - `uGlossiness` - Polished surface (default: 0.7)

#### Lava Shader (Animated)
- **Files:** `shaders/lava.frag`, `materials/lava.lua`
- **Description:** Animated molten lava with emissive glow
- **Parameters:**
  - `uHotColor`, `uCoolColor` - Temperature colors
  - `uFlowSpeed` - Flow speed (default: 0.5)
  - `uEmissive` - Emissive intensity (default: 2.0)

#### Crystal Shader
- **Files:** `shaders/crystal.frag`, `materials/crystal.lua`
- **Description:** Crystalline gem with chromatic dispersion
- **Parameters:**
  - `uRefractiveIndex` - IOR (default: 2.4)
  - `uDispersion` - Chromatic dispersion (default: 0.3)
  - `uFacets` - Facet sharpness (default: 8.0)

#### Skin Shader
- **Files:** `shaders/skin.frag`, `materials/skin.lua`
- **Description:** Subsurface scattering for skin
- **Parameters:**
  - `uSubsurfaceColor` - SSS color
  - `uScatterWidth` - Scatter width (default: 0.5)
  - `uOiliness` - Skin shine (default: 0.3)
  - `uPoreSize` - Pore size (default: 0.01)

---

### 3. Effect Shaders (11 shaders)

#### Glow Shader
- **Files:** `shaders/glow.frag`, `materials/glow.lua`
- **Description:** Emissive glow with pulsing animation
- **Parameters:**
  - `uGlowColor`, `uGlowIntensity` - Glow controls
  - `uPulseSpeed`, `uPulseAmount` - Pulse animation
  - `uFresnelPower` - Edge glow (default: 2.0)

#### Dissolve Shader
- **Files:** `shaders/dissolve.frag`, `materials/dissolve.lua`
- **Description:** Burn/dissolve away effect
- **Parameters:**
  - `uDissolveAmount` - Dissolve progress 0-1
  - `uEdgeColor`, `uEdgeColor2` - Burn edge colors
  - `uEdgeWidth` - Edge glow width (default: 0.1)

#### Hologram Shader
- **Files:** `shaders/hologram.frag`, `materials/hologram.lua`
- **Description:** Sci-fi holographic effect with scanlines
- **Parameters:**
  - `uScanlineSpeed`, `uScanlineDensity` - Scanline controls
  - `uFlickerSpeed` - Flicker frequency (default: 10.0)
  - `uGlitchAmount` - Glitch intensity (default: 0.1)

#### Force Field Shader
- **Files:** `shaders/force_field.frag`, `materials/force_field.lua`
- **Description:** Energy shield with hexagon grid
- **Parameters:**
  - `uHexSize` - Hexagon size (default: 0.3)
  - `uPulseSpeed` - Energy pulse speed (default: 1.5)
  - `uImpactX`, `uImpactY`, `uImpactStrength` - Impact ripple

#### Ghost Shader
- **Files:** `shaders/ghost.frag`, `materials/ghost.lua`
- **Description:** Ethereal ghostly transparency
- **Parameters:**
  - `uWispiness` - Wispy trail amount (default: 0.5)
  - `uFloatSpeed` - Floating animation (default: 1.0)
  - `uGlowIntensity` - Ethereal glow (default: 1.5)

#### Glitch Shader
- **Files:** `shaders/glitch.frag`, `materials/glitch.lua`
- **Description:** Digital glitch/corruption effect
- **Parameters:**
  - `uGlitchIntensity` - Glitch frequency (default: 0.5)
  - `uGlitchSpeed` - Glitch speed (default: 5.0)
  - `uBlockSize` - Glitch block size (default: 0.1)

#### Fire Shader
- **Files:** `shaders/fire.frag`, `materials/fire.lua`
- **Description:** Procedural fire effect
- **Parameters:**
  - `uFireColorHot`, `uFireColorMid`, `uFireColorCool` - Temperature colors
  - `uFlameSpeed` - Animation speed (default: 2.0)
  - `uTurbulence` - Flame turbulence (default: 0.5)

#### Smoke Shader
- **Files:** `shaders/smoke.frag`, `materials/smoke.lua`
- **Description:** Volumetric smoke effect
- **Parameters:**
  - `uDensity` - Smoke density (default: 0.5)
  - `uRiseSpeed` - Rising speed (default: 0.5)
  - `uDissipation` - Dissipation rate (default: 0.5)

#### Electricity Shader
- **Files:** `shaders/electricity.frag`, `materials/electricity.lua`
- **Description:** Electric arc/lightning effect
- **Parameters:**
  - `uBoltSpeed` - Arc animation speed (default: 10.0)
  - `uBranchiness` - Arc branching (default: 0.5)
  - `uThickness` - Arc thickness (default: 0.05)

#### Portal Shader
- **Files:** `shaders/portal.frag`, `materials/portal.lua`
- **Description:** Swirling portal effect
- **Parameters:**
  - `uPortalColor1`, `uPortalColor2` - Portal colors
  - `uRotationSpeed` - Rotation speed (default: 1.0)
  - `uWarpAmount` - Space warping (default: 0.5)

#### Teleport Shader
- **Files:** `shaders/teleport.frag`, `materials/teleport.lua`
- **Description:** Teleportation sparkle/particle effect
- **Parameters:**
  - `uTeleportProgress` - Animation progress 0-1
  - `uParticleColor` - Particle color
  - `uParticleDensity` - Particle density (default: 50.0)

---

### 4. Debug/Utility Shaders (8 shaders)

#### Wireframe Shader
- **Files:** `shaders/wireframe.frag`, `materials/wireframe.lua`
- **Description:** Wireframe overlay visualization
- **Parameters:**
  - `uWireColor` - Wireframe color (default: green)
  - `uWireThickness` - Wire thickness (default: 0.02)
  - `uShowSolid` - Show solid fill (default: false)

#### Normals Shader
- **Files:** `shaders/normals.frag`, `materials/normals.lua`
- **Description:** Visualize surface normals as RGB
- **Parameters:**
  - `uWorldSpace` - Show world-space normals (default: true)

#### Depth Shader
- **Files:** `shaders/depth.frag`, `materials/depth.lua`
- **Description:** Visualize depth buffer
- **Parameters:**
  - `uNearPlane`, `uFarPlane` - Depth range
  - `uLinearize` - Linearize depth (default: true)

#### UV Shader
- **Files:** `shaders/uv.frag`, `materials/uv.lua`
- **Description:** Visualize UV coordinates with grid
- **Parameters:**
  - `uShowGrid` - Show UV grid (default: true)
  - `uGridDensity` - Grid density (default: 10.0)

#### AO Debug Shader
- **Files:** `shaders/ao_debug.frag`, `materials/ao_debug.lua`
- **Description:** Visualize ambient occlusion
- **Parameters:**
  - `uAOStrength` - AO strength (default: 1.0)
  - `uAORadius` - AO radius (default: 1.0)

#### Checker Shader
- **Files:** `shaders/checker.frag`, `materials/checker.lua`
- **Description:** Checkerboard pattern for UV testing
- **Parameters:**
  - `uColor1`, `uColor2` - Checker colors
  - `uDensity` - Checker density (default: 8.0)

#### Vertex Colors Shader
- **Files:** `shaders/vertex_colors.frag`, `materials/vertex_colors.lua`
- **Description:** Display vertex colors
- **Parameters:**
  - `uShowAlpha` - Visualize alpha channel (default: false)

#### Error Shader
- **Files:** `shaders/error.frag`, `materials/error.lua`
- **Description:** Magenta error shader (missing material fallback)
- **Parameters:** None (animated magenta/black checkerboard)

---

### 5. Post-Processing Shaders (5 shaders)

**Note:** Post-processing shaders use `fullscreen.vert` instead of `basic.vert`

#### Bloom Shader
- **Files:** `shaders/bloom.frag`
- **Description:** Glow/bloom post-process
- **Parameters:**
  - `uSceneTexture` - Scene color texture
  - `uThreshold` - Brightness threshold (default: 1.0)
  - `uIntensity` - Bloom intensity (default: 1.0)
  - `uBlurSize` - Blur size (default: 2.0)

#### Vignette Shader
- **Files:** `shaders/vignette.frag`
- **Description:** Darkened edges
- **Parameters:**
  - `uSceneTexture` - Scene color texture
  - `uIntensity` - Vignette intensity (default: 0.5)
  - `uPower` - Falloff power (default: 2.0)
  - `uColor` - Vignette color (default: black)

#### Color Grading Shader
- **Files:** `shaders/color_grade.frag`
- **Description:** Color correction and grading
- **Parameters:**
  - `uSceneTexture` - Scene color texture
  - `uExposure`, `uContrast`, `uSaturation`, `uBrightness`
  - `uColorTint` - Color tint
  - `uGamma` - Gamma correction (default: 2.2)

#### Fog Shader
- **Files:** `shaders/fog.frag`
- **Description:** Distance fog post-process
- **Parameters:**
  - `uSceneTexture`, `uDepthTexture` - Scene and depth
  - `uFogColor` - Fog color
  - `uFogDensity`, `uFogStart`, `uFogEnd` - Fog controls
  - `uExponential` - Use exponential fog (default: false)

#### Depth of Field Shader
- **Files:** `shaders/dof.frag`
- **Description:** Bokeh blur based on depth
- **Parameters:**
  - `uSceneTexture`, `uDepthTexture` - Scene and depth
  - `uFocusDistance` - Focus distance (default: 10.0)
  - `uFocusRange` - Focus range (default: 5.0)
  - `uBokehSize` - Blur size (default: 3.0)

---

### 6. Environment Shaders (4 shaders)

#### Skybox Shader
- **Files:** `shaders/skybox.frag`, `materials/skybox.lua`
- **Description:** Procedural sky gradient with sun and stars
- **Parameters:**
  - `uSkyColorTop`, `uSkyColorHorizon`, `uGroundColor` - Sky colors
  - `uSunDirection`, `uSunColor`, `uSunSize` - Sun controls
  - `uShowStars` - Show stars (default: true)

#### Terrain Shader
- **Files:** `shaders/terrain.frag`, `materials/terrain.lua`
- **Description:** Multi-texture terrain blending
- **Parameters:**
  - `uGrassColor`, `uDirtColor`, `uRockColor`, `uSandColor` - Terrain types
  - `uSlopeThreshold` - Slope for rock (default: 0.7)
  - `uHeightScale` - Height scale (default: 10.0)

#### Foliage Shader
- **Files:** `shaders/foliage.frag`, `materials/foliage.lua`
- **Description:** Vegetation with wind animation (use with `animated.vert`)
- **Parameters:**
  - `uSubsurfaceColor`, `uSubsurfaceAmount` - SSS controls
  - `uAlphaCutoff` - Alpha cutoff for leaves (default: 0.5)
  - `uTwoSided` - Two-sided rendering (default: true)
  - **From animated.vert:** `uWindStrength`, `uWindFrequency`

#### Clouds Shader
- **Files:** `shaders/clouds.frag`, `materials/clouds.lua`
- **Description:** Procedural volumetric clouds
- **Parameters:**
  - `uCloudColor`, `uSkyColor` - Colors
  - `uCloudSpeed` - Movement speed (default: 0.1)
  - `uCloudDensity` - Density (default: 0.5)
  - `uCloudScale` - Size scale (default: 3.0)

---

## Common Vertex Shaders

### basic.vert
Standard vertex shader for most materials. Provides:
- World position (`vWorldPos`)
- Transformed normal (`vNormal`)
- Texture coordinates (`vTexCoord`)
- Vertex color (`vColor`)

### animated.vert
Vertex shader with animation support. Adds:
- Wind animation (for foliage)
- Wave animation (for water, cloth)

**Additional Parameters:**
- `uWindStrength`, `uWindFrequency` - Wind animation
- `uWaveAmplitude`, `uWaveFrequency` - Wave animation

### fullscreen.vert
Vertex shader for post-processing (fullscreen quad). Provides:
- Texture coordinates (`vTexCoord`)

---

## Standard Uniforms

Most shaders support these standard uniforms:

### Lighting
- `uLightDir` - Light direction vector
- `uLightColor` - Light color (RGB)
- `uAmbientColor` - Ambient color (RGB)

### Transformation
- `uModel` - Model matrix
- `uView` - View matrix
- `uProjection` - Projection matrix
- `uNormalMatrix` - Normal transformation matrix (mat3)

### View
- `uCameraPos` - Camera world position

### Animation
- `uTime` - Time in seconds (for animated shaders)

### Material
- `uBaseColor` - Base material color (RGBA)

---

## Blend Modes

Materials use these blend modes:

- **opaque** - No transparency, writes to depth buffer
- **blend** - Alpha blending with transparency
- **additive** - Additive blending (for glows, fire)
- **alpha_test** - Hard alpha cutoff (for leaves, grass)

---

## Usage Examples

### Loading a Material

```cpp
// Example: Load toon material
auto material = materialSystem->loadMaterial("materials/toon.lua");

// Override uniform
material->setUniform("uBands", 4);
material->setUniform("uBaseColor", glm::vec4(0.8f, 0.2f, 0.3f, 1.0f));
```

### Animating a Shader

```cpp
// Dissolve effect
float dissolveProgress = 0.0f;
material->setUniform("uDissolveAmount", dissolveProgress);

// In update loop:
dissolveProgress += deltaTime * 0.5f; // Dissolve over 2 seconds
material->setUniform("uDissolveAmount", dissolveProgress);
```

### Force Field Impact

```cpp
// When shield is hit at world position (x, y)
material->setUniform("uImpactX", impactPos.x);
material->setUniform("uImpactY", impactPos.y);
material->setUniform("uImpactStrength", 1.0f);

// Fade out impact over time
impactStrength -= deltaTime * 2.0f;
material->setUniform("uImpactStrength", std::max(0.0f, impactStrength));
```

---

## Shader Features

### Noise Functions
Many shaders include procedural noise functions for:
- Texture generation (wood, marble, clouds)
- Animation (fire, water, smoke)
- Variation (painterly, watercolor)

### Fresnel Effects
Used for edge lighting and rim effects:
- Toon, anime (rim lighting)
- Hologram, ghost (edge glow)
- Glass, crystal (reflectivity)
- Force field (visibility)

### Subsurface Scattering
Approximated in:
- Skin shader (translucent skin)
- Fabric shader (cloth subsurface)
- Foliage shader (light through leaves)
- Ice shader (internal scattering)

### Physically Based Rendering
- PBR shader (full Cook-Torrance BRDF)
- Metallic shader (high F0 metals)
- Various materials use Fresnel-Schlick

---

## Performance Notes

### High Performance
- Toon, anime, comic (simple calculations)
- Debug shaders (wireframe, normals, UV)
- Metallic, glass (minimal samples)

### Medium Performance
- Painterly, watercolor (multi-octave noise)
- PBR (BRDF calculations)
- Water, lava (animated noise)

### Lower Performance
- Fire, smoke (multi-layer fbm)
- Terrain (multiple texture blending)
- Clouds (multi-octave fbm with transparency)
- Post-processing (multiple texture samples)

**Optimization Tips:**
- Reduce noise octaves for procedural shaders
- Lower particle density for effects
- Use simpler blend modes where possible
- Disable features not needed (e.g., `uShowStars` in skybox)

---

## Hot Reload

All materials have `hotReload = true`, allowing you to:
1. Edit shader or material files
2. Save changes
3. See updates immediately in-game (if hot reload system is active)

This enables rapid iteration and tweaking of shader parameters.

---

## GLSL Version

All shaders use **GLSL 410 core** for macOS compatibility with OpenGL 4.1.

---

## File Count Summary

- **Vertex Shaders:** 3 (basic.vert, animated.vert, fullscreen.vert)
- **Fragment Shaders:** 49
- **Material Templates:** 42
- **Total Files:** 94

---

## Contributing

When adding new shaders:
1. Use GLSL 410 core
2. Follow the naming conventions
3. Include default values in comments
4. Create a matching Lua material template
5. Document parameters in this file
6. Set `hotReload = true`
7. Add to appropriate category

---

## License

Part of the Bestow game engine. See project license.
