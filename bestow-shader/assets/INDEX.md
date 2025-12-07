# Shader Library Quick Reference

## Complete Shader List (49 Fragment Shaders)

### Stylized/Artistic (9)
1. `toon.frag` - Cel-shading with bands
2. `hatching.frag` - Cross-hatching sketch
3. `painterly.frag` - Brushstroke painting
4. `comic.frag` - Comic book halftone
5. `watercolor.frag` - Watercolor paint
6. `pixelart.frag` - Retro pixel art
7. `anime.frag` - Anime-style rendering
8. `oilpaint.frag` - Oil painting
9. `posterize.frag` - Limited palette

### Materials (12)
10. `pbr.frag` - Physically Based Rendering
11. `metallic.frag` - Enhanced metals
12. `glass.frag` - Transparent glass
13. `water.frag` - Animated water
14. `ice.frag` - Frozen crystalline
15. `fabric.frag` - Cloth/fabric
16. `leather.frag` - Leather texture
17. `wood.frag` - Wood grain
18. `marble.frag` - Marble stone
19. `lava.frag` - Molten lava
20. `crystal.frag` - Crystalline gem
21. `skin.frag` - Subsurface skin

### Effects (11)
22. `glow.frag` - Emissive glow
23. `dissolve.frag` - Burn/dissolve
24. `hologram.frag` - Holographic scanlines
25. `force_field.frag` - Energy shield
26. `ghost.frag` - Ethereal ghost
27. `glitch.frag` - Digital glitch
28. `fire.frag` - Procedural fire
29. `smoke.frag` - Volumetric smoke
30. `electricity.frag` - Electric arcs
31. `portal.frag` - Swirling portal
32. `teleport.frag` - Teleport particles

### Debug/Utility (8)
33. `wireframe.frag` - Wireframe overlay
34. `normals.frag` - Normal visualization
35. `depth.frag` - Depth visualization
36. `uv.frag` - UV coordinate display
37. `ao_debug.frag` - Ambient occlusion
38. `checker.frag` - Checkerboard pattern
39. `vertex_colors.frag` - Vertex color display
40. `error.frag` - Error fallback

### Post-Processing (5)
41. `bloom.frag` - Bloom/glow
42. `vignette.frag` - Darkened edges
43. `color_grade.frag` - Color grading
44. `fog.frag` - Distance fog
45. `dof.frag` - Depth of field

### Environment (4)
46. `skybox.frag` - Procedural sky
47. `terrain.frag` - Multi-texture terrain
48. `foliage.frag` - Vegetation
49. `clouds.frag` - Volumetric clouds

---

## Vertex Shaders (3)

- `basic.vert` - Standard vertex shader (use with most shaders)
- `animated.vert` - Vertex animation support (wind, waves)
- `fullscreen.vert` - Post-processing quad

---

## Material Templates (44)

All 49 shaders have corresponding `.lua` material files in `materials/` directory (except post-processing shaders which are typically used programmatically).

---

## Quick Start

### 1. Simple Stylized Look
```lua
-- materials/toon.lua
shader = { vertex = "shaders/basic.vert", fragment = "shaders/toon.frag" }
```

### 2. Realistic Material
```lua
-- materials/pbr.lua
shader = { vertex = "shaders/basic.vert", fragment = "shaders/pbr.frag" }
```

### 3. Animated Effect
```lua
-- materials/water.lua (uses uTime uniform)
shader = { vertex = "shaders/basic.vert", fragment = "shaders/water.frag" }
```

### 4. Vegetation with Wind
```lua
-- materials/foliage.lua (uses animated vertex shader)
shader = { vertex = "shaders/animated.vert", fragment = "shaders/foliage.frag" }
```

### 5. Debug Visualization
```lua
-- materials/normals.lua
shader = { vertex = "shaders/basic.vert", fragment = "shaders/normals.frag" }
```

---

## File Locations

```
bestow-shader/assets/
├── SHADER_LIBRARY.md    # Full documentation
├── INDEX.md             # This file (quick reference)
├── shaders/
│   ├── basic.vert       # Standard vertex shader
│   ├── animated.vert    # Animated vertex shader
│   ├── fullscreen.vert  # Post-process vertex shader
│   └── [49 .frag files] # Fragment shaders
└── materials/
    └── [44 .lua files]  # Material templates
```

---

## Categories by Use Case

### For Stylized Games
- Toon, anime, comic (cel-shaded)
- Painterly, watercolor, oilpaint (artistic)
- Hatching, pixelart (retro)

### For Realistic Games
- PBR (standard realistic)
- Metallic, glass, crystal (reflective)
- Skin, fabric, leather (organic)
- Wood, marble (natural)

### For VFX/Effects
- Fire, smoke, electricity (particles)
- Glow, hologram, ghost (translucent)
- Dissolve, teleport, portal (transitions)
- Force field (shields)

### For Environment
- Skybox, clouds (sky)
- Terrain (ground)
- Foliage (vegetation)
- Water, lava (liquids)

### For Development
- Wireframe, normals, depth (geometry)
- UV, checker (texturing)
- Vertex colors, AO (data)
- Error (fallback)

---

## Compatibility

- **GLSL Version:** 410 core
- **Target Platform:** macOS OpenGL 4.1+
- **Hot Reload:** All materials support hot reload
- **Shader Model:** 4.0+

---

## Total File Count

- **Vertex Shaders:** 3
- **Fragment Shaders:** 49
- **Material Templates:** 44
- **Documentation:** 2
- **Total:** 98 files

---

For detailed documentation, see `SHADER_LIBRARY.md`.
