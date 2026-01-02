# Verification Checklist

This document provides verification criteria and test plans for the UI overhaul.

## Architecture Verification

### Contract Compliance

| Check | Criteria | Status |
|-------|----------|--------|
| UI depends only on contracts | `RmlUISystem` has no includes of OpenGL, Vulkan, GLFW | [ ] |
| Graphics provides UI backend | `getUIRenderBackend()` works for all implementations | [ ] |
| Single UI implementation | Same `RmlUISystem` works with OpenGL and Vulkan | [ ] |
| No circular dependencies | Contracts can be compiled independently | [ ] |
| DI wiring works | Kangaru can resolve all dependencies | [ ] |

### Interface Compatibility

| Check | Criteria | Status |
|-------|----------|--------|
| IGraphicsContext complete | All methods implemented in 2D and 3D systems | [ ] |
| IUIRenderBackend complete | All methods implemented in OpenGL and Vulkan | [ ] |
| Handle types compatible | UIGeometryHandle/UITextureHandle castable to RmlUi handles | [ ] |
| Vertex format correct | UIVertex maps correctly to RmlUi Vertex | [ ] |

## Functionality Verification

### OpenGL 2D Backend

| Test | Expected Result | Status |
|------|-----------------|--------|
| Backend initialization | `initialize()` returns true | [ ] |
| Compile geometry | Returns valid handle > 0 | [ ] |
| Render geometry | No GL errors, triangles visible | [ ] |
| Texture creation | Returns valid handle, texture usable | [ ] |
| Scissor test | Content clipped to region | [ ] |
| State preservation | Previous GL state restored after `endUIPass()` | [ ] |

### OpenGL 3D Backend

| Test | Expected Result | Status |
|------|-----------------|--------|
| Same as 2D | (Backend is shared) | [ ] |
| Works with 3D content | UI renders on top of 3D scene | [ ] |
| Depth test disabled | UI always visible | [ ] |

### Vulkan Backend

| Test | Expected Result | Status |
|------|-----------------|--------|
| Backend initialization | Pipeline created, no validation errors | [ ] |
| Compile geometry | VkBuffer created with data | [ ] |
| Render geometry | Draw commands recorded | [ ] |
| Texture creation | VkImage/VkImageView created | [ ] |
| Scissor test | Dynamic scissor set correctly | [ ] |
| Command buffer sync | Commands recorded in correct order | [ ] |

### RmlUi Integration

| Test | Expected Result | Status |
|------|-----------------|--------|
| Document loading | `.rml` files parse correctly | [ ] |
| Element rendering | Text and shapes visible | [ ] |
| Font rendering | Text readable, no artifacts | [ ] |
| Data binding | Variables update display | [ ] |
| Event handling | Click events fire callbacks | [ ] |
| Stylesheet application | CSS styles apply correctly | [ ] |
| Hot reload | Document changes reflected live | [ ] |

## Performance Verification

| Metric | Target | Status |
|--------|--------|--------|
| Draw calls | < 50 for typical HUD | [ ] |
| Frame time overhead | < 0.5ms for UI rendering | [ ] |
| Memory usage | < 10MB for UI resources | [ ] |
| Geometry compile time | < 1ms per document | [ ] |

## Regression Tests

### Existing Functionality

| Test | Expected Result | Status |
|------|-----------------|--------|
| 2D game rendering | Sprites render correctly | [ ] |
| 3D game rendering | Meshes render correctly | [ ] |
| Debug lines | `debugDrawLine()` works | [ ] |
| Text rendering (2D) | `drawText()` works | [ ] |
| Text rendering (3D) | `drawText3D()` works (if implemented) | [ ] |

### Snake Game Specific

| Test | Expected Result | Status |
|------|-----------------|--------|
| Game renders | Snake, food, environment visible | [ ] |
| UI overlay | HUD visible on top of game | [ ] |
| Score display | Score updates when food eaten | [ ] |
| Menu rendering | Main menu, pause menu work | [ ] |
| Can remove pixel font hack | Game works without custom font code | [ ] |

## Integration Tests

### Full Pipeline Test

```cpp
// Test: Create graphics, UI, render, shutdown
TEST(UIIntegration, FullPipeline) {
    // Create container
    kgr::container container;
    container.service<IGraphics3DSystem, VulkanGraphics3DSystem>();
    container.service<IGraphicsContext, VulkanGraphics3DSystem>();
    container.service<IAssetSystem, AssetSystem>();
    container.service<IUISystem, RmlUISystem>();

    auto& graphics = container.service<IGraphics3DSystem>();
    auto& ui = container.service<IUISystem>();

    // Initialize
    ASSERT_TRUE(graphics.initialize({800, 600, "Test"}));
    ASSERT_TRUE(ui.initialize({}).has_value());

    // Load document
    auto doc = ui.loadDocumentFromString("<rml><body>Test</body></rml>");
    ASSERT_TRUE(doc.has_value());
    ui.showDocument(*doc);

    // Render one frame
    graphics.beginFrame();
    ui.render();
    graphics.endFrame();

    // Cleanup
    ui.shutdown();
    graphics.shutdown();
}
```

### Backend Isolation Test

```cpp
// Test: UI backend works without full graphics
TEST(UIRenderBackend, Isolation) {
    // Assuming OpenGL context is available
    OpenGLUIRenderBackend backend;
    ASSERT_TRUE(backend.initialize());

    // Create geometry
    std::vector<UIVertex> verts = {
        {{0, 0}, Color::white(), {0, 0}},
        {{100, 0}, Color::white(), {1, 0}},
        {{100, 100}, Color::white(), {1, 1}},
    };
    std::vector<uint32_t> indices = {0, 1, 2};

    auto handle = backend.compileGeometry(verts, indices);
    ASSERT_NE(handle, InvalidUIGeometry);

    // Create texture
    std::vector<uint8_t> texData(4 * 4 * 4, 255);  // 4x4 white
    auto tex = backend.createTexture(texData, 4, 4);
    ASSERT_NE(tex, InvalidUITexture);

    // Render
    backend.beginUIPass();
    backend.renderGeometry(handle, {10, 10}, tex);
    backend.endUIPass();

    // Cleanup
    backend.releaseGeometry(handle);
    backend.releaseTexture(tex);
    backend.shutdown();
}
```

## Manual Verification Steps

### Visual Inspection

1. **Run snake game with Vulkan backend**
   - [ ] Game renders without errors
   - [ ] UI HUD is visible
   - [ ] Text is readable
   - [ ] Colors are correct (not washed out)
   - [ ] Elements are positioned correctly

2. **Run 2D example with OpenGL backend**
   - [ ] Sprites render correctly
   - [ ] UI overlay visible
   - [ ] No visual artifacts

3. **Test scissor clipping**
   - [ ] Create nested scrollable element
   - [ ] Verify content clips at boundaries
   - [ ] No content bleeding outside

4. **Test input handling**
   - [ ] Click on UI buttons
   - [ ] Verify hover states work
   - [ ] Keyboard input in text fields

### Hot Reload Testing

1. Start game with UI loaded
2. Modify `.rml` file externally
3. Save file
4. Verify changes appear in game (within 1 second)
5. No crashes or visual glitches

## Acceptance Criteria

The overhaul is considered complete when:

1. **All contract compliance checks pass**
2. **All functionality tests pass for both OpenGL and Vulkan**
3. **Snake game works with proper UI (no pixel font hack)**
4. **Performance targets met**
5. **No regressions in existing functionality**
6. **Documentation complete and accurate**

## Known Limitations

Document any known limitations discovered during verification:

| Limitation | Workaround | Priority |
|------------|------------|----------|
| (To be filled during verification) | | |

## Sign-off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Developer | | | |
| Reviewer | | | |
| Tester | | | |
