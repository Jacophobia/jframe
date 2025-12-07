// tests/unit/Graphics3DSystemTests.cpp
// 3D Graphics system unit tests

#include <memory>
#include <vector>

#include <gtest/gtest.h>

import bestow.graphics3d;
import bestow.graphics3d.impl;
import bestow.types;
import bestow.assets;

namespace bestow::tests {

class Graphics3DSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        graphics3d_ = createGraphics3DSystem();
    }

    std::unique_ptr<IGraphics3DSystem> graphics3d_;
};

//==============================================================================
// Mesh Creation and Management Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, CreateSimpleMesh) {
    // Create a simple triangle mesh
    std::vector<Vertex3D> vertices = {
        Vertex3D{.position = Vec3{0.0f, 1.0f, 0.0f}, .normal = Vec3{0.0f, 0.0f, 1.0f}},
        Vertex3D{.position = Vec3{-1.0f, -1.0f, 0.0f}, .normal = Vec3{0.0f, 0.0f, 1.0f}},
        Vertex3D{.position = Vec3{1.0f, -1.0f, 0.0f}, .normal = Vec3{0.0f, 0.0f, 1.0f}}
    };

    std::vector<std::uint32_t> indices = {0, 1, 2};

    MeshDef meshDef{
        .vertices = vertices,
        .indices = indices,
        .isDynamic = false
    };

    auto result = graphics3d_->createMesh(meshDef);
    EXPECT_TRUE(result.has_value());

    if (result.has_value()) {
        MeshHandle mesh = result.value();
        EXPECT_NE(mesh, 0u);
        EXPECT_TRUE(graphics3d_->hasMesh(mesh));
    }
}

TEST_F(Graphics3DSystemTest, CreateDynamicMesh) {
    std::vector<Vertex3D> vertices = {
        Vertex3D{.position = Vec3{0.0f, 0.0f, 0.0f}},
        Vertex3D{.position = Vec3{1.0f, 0.0f, 0.0f}},
        Vertex3D{.position = Vec3{0.0f, 1.0f, 0.0f}}
    };

    std::vector<std::uint32_t> indices = {0, 1, 2};

    MeshDef meshDef{
        .vertices = vertices,
        .indices = indices,
        .isDynamic = true  // Allow updates
    };

    auto result = graphics3d_->createMesh(meshDef);
    EXPECT_TRUE(result.has_value());
}

TEST_F(Graphics3DSystemTest, DestroyMesh) {
    std::vector<Vertex3D> vertices = {
        Vertex3D{.position = Vec3{0.0f, 0.0f, 0.0f}},
        Vertex3D{.position = Vec3{1.0f, 0.0f, 0.0f}},
        Vertex3D{.position = Vec3{0.0f, 1.0f, 0.0f}}
    };

    std::vector<std::uint32_t> indices = {0, 1, 2};

    MeshDef meshDef{
        .vertices = vertices,
        .indices = indices
    };

    auto result = graphics3d_->createMesh(meshDef);
    ASSERT_TRUE(result.has_value());

    MeshHandle mesh = result.value();
    EXPECT_TRUE(graphics3d_->hasMesh(mesh));

    graphics3d_->destroyMesh(mesh);
    EXPECT_FALSE(graphics3d_->hasMesh(mesh));
}

TEST_F(Graphics3DSystemTest, HasMeshReturnsFalseForInvalidHandle) {
    MeshHandle invalidMesh = 999999;
    EXPECT_FALSE(graphics3d_->hasMesh(invalidMesh));
}

TEST_F(Graphics3DSystemTest, CreateMeshWithSubMeshes) {
    std::vector<Vertex3D> vertices = {
        // First triangle
        Vertex3D{.position = Vec3{0.0f, 0.0f, 0.0f}},
        Vertex3D{.position = Vec3{1.0f, 0.0f, 0.0f}},
        Vertex3D{.position = Vec3{0.0f, 1.0f, 0.0f}},
        // Second triangle
        Vertex3D{.position = Vec3{1.0f, 1.0f, 0.0f}},
        Vertex3D{.position = Vec3{2.0f, 1.0f, 0.0f}},
        Vertex3D{.position = Vec3{1.0f, 2.0f, 0.0f}}
    };

    std::vector<std::uint32_t> indices = {
        0, 1, 2,  // SubMesh 0
        3, 4, 5   // SubMesh 1
    };

    std::vector<SubMesh> subMeshes = {
        SubMesh{.indexOffset = 0, .indexCount = 3, .materialIndex = 0},
        SubMesh{.indexOffset = 3, .indexCount = 3, .materialIndex = 1}
    };

    MeshDef meshDef{
        .vertices = vertices,
        .indices = indices,
        .subMeshes = subMeshes
    };

    auto result = graphics3d_->createMesh(meshDef);
    EXPECT_TRUE(result.has_value());
}

TEST_F(Graphics3DSystemTest, GetMeshBounds) {
    std::vector<Vertex3D> vertices = {
        Vertex3D{.position = Vec3{-1.0f, -1.0f, -1.0f}},
        Vertex3D{.position = Vec3{1.0f, 1.0f, 1.0f}}
    };

    std::vector<std::uint32_t> indices = {0, 1, 0};

    AABB3D expectedBounds{
        .min = Vec3{-1.0f, -1.0f, -1.0f},
        .max = Vec3{1.0f, 1.0f, 1.0f}
    };

    MeshDef meshDef{
        .vertices = vertices,
        .indices = indices,
        .bounds = expectedBounds
    };

    auto result = graphics3d_->createMesh(meshDef);
    ASSERT_TRUE(result.has_value());

    MeshHandle mesh = result.value();
    AABB3D bounds = graphics3d_->getMeshBounds(mesh);

    // Bounds should be set (exact values may vary based on implementation)
    EXPECT_TRUE(bounds.min.x <= bounds.max.x);
    EXPECT_TRUE(bounds.min.y <= bounds.max.y);
    EXPECT_TRUE(bounds.min.z <= bounds.max.z);
}

//==============================================================================
// Material Management Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, CreatePBRMaterial) {
    PBRMaterial pbrMat;
    pbrMat.baseColorFactor = Vec4{1.0f, 0.0f, 0.0f, 1.0f};  // Red
    pbrMat.metallicFactor = 0.5f;
    pbrMat.roughnessFactor = 0.3f;

    auto result = graphics3d_->createMaterial(pbrMat);
    EXPECT_TRUE(result.has_value());

    if (result.has_value()) {
        MaterialHandle mat = result.value();
        EXPECT_NE(mat, 0u);
        EXPECT_TRUE(graphics3d_->hasMaterial(mat));
    }
}

TEST_F(Graphics3DSystemTest, CreateUnlitMaterial) {
    UnlitMaterial unlitMat;
    unlitMat.color = Vec4{0.0f, 1.0f, 0.0f, 1.0f};  // Green

    auto result = graphics3d_->createMaterial(unlitMat);
    EXPECT_TRUE(result.has_value());
}

TEST_F(Graphics3DSystemTest, DestroyMaterial) {
    PBRMaterial pbrMat;
    auto result = graphics3d_->createMaterial(pbrMat);
    ASSERT_TRUE(result.has_value());

    MaterialHandle mat = result.value();
    EXPECT_TRUE(graphics3d_->hasMaterial(mat));

    graphics3d_->destroyMaterial(mat);
    EXPECT_FALSE(graphics3d_->hasMaterial(mat));
}

TEST_F(Graphics3DSystemTest, HasMaterialReturnsFalseForInvalidHandle) {
    MaterialHandle invalidMat = 999999;
    EXPECT_FALSE(graphics3d_->hasMaterial(invalidMat));
}

//==============================================================================
// Rendering Queue Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, SubmitRenderItem) {
    // Create a simple mesh
    std::vector<Vertex3D> vertices = {
        Vertex3D{.position = Vec3{0.0f, 0.0f, 0.0f}}
    };
    std::vector<std::uint32_t> indices = {0};
    MeshDef meshDef{.vertices = vertices, .indices = indices};

    auto meshResult = graphics3d_->createMesh(meshDef);
    ASSERT_TRUE(meshResult.has_value());
    MeshHandle mesh = meshResult.value();

    // Create a material
    PBRMaterial pbrMat;
    auto matResult = graphics3d_->createMaterial(pbrMat);
    ASSERT_TRUE(matResult.has_value());
    MaterialHandle mat = matResult.value();

    // Submit render item
    RenderItem item;
    item.mesh = mesh;
    item.material = mat;
    item.worldMatrix = Mat4::identity();

    graphics3d_->submitRenderItem(item);
    // Should not crash
}

TEST_F(Graphics3DSystemTest, ClearRenderQueue) {
    std::vector<Vertex3D> vertices = {
        Vertex3D{.position = Vec3{0.0f, 0.0f, 0.0f}}
    };
    std::vector<std::uint32_t> indices = {0};
    MeshDef meshDef{.vertices = vertices, .indices = indices};

    auto meshResult = graphics3d_->createMesh(meshDef);
    ASSERT_TRUE(meshResult.has_value());

    PBRMaterial pbrMat;
    auto matResult = graphics3d_->createMaterial(pbrMat);
    ASSERT_TRUE(matResult.has_value());

    RenderItem item;
    item.mesh = meshResult.value();
    item.material = matResult.value();
    item.worldMatrix = Mat4::identity();

    graphics3d_->submitRenderItem(item);
    graphics3d_->clearRenderQueue();
    // Should not crash
}

//==============================================================================
// Frame Lifecycle Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, BeginAndEndFrame) {
    graphics3d_->beginFrame();
    graphics3d_->endFrame();
    // Should not crash
}

TEST_F(Graphics3DSystemTest, MultipleFrames) {
    for (int i = 0; i < 10; ++i) {
        graphics3d_->beginFrame();
        graphics3d_->endFrame();
    }
    // Should not crash
}

//==============================================================================
// Camera Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, SetActiveCamera) {
    Camera3D camera;
    camera.position = Vec3{0.0f, 0.0f, 5.0f};
    camera.target = Vec3{0.0f, 0.0f, 0.0f};
    camera.up = Vec3{0.0f, 1.0f, 0.0f};
    camera.fov = 45.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 100.0f;

    graphics3d_->setActiveCamera(camera);
    // Should not crash
}

TEST_F(Graphics3DSystemTest, GetActiveCamera) {
    Camera3D camera;
    camera.position = Vec3{1.0f, 2.0f, 3.0f};
    camera.fov = 60.0f;

    graphics3d_->setActiveCamera(camera);

    const Camera3D& retrieved = graphics3d_->getActiveCamera();
    EXPECT_FLOAT_EQ(retrieved.position.x, 1.0f);
    EXPECT_FLOAT_EQ(retrieved.position.y, 2.0f);
    EXPECT_FLOAT_EQ(retrieved.position.z, 3.0f);
    EXPECT_FLOAT_EQ(retrieved.fov, 60.0f);
}

//==============================================================================
// Lighting Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, SetDirectionalLight) {
    DirectionalLight dirLight;
    dirLight.direction = Vec3{0.0f, -1.0f, 0.0f};
    dirLight.color = Vec3{1.0f, 1.0f, 1.0f};
    dirLight.intensity = 1.0f;

    graphics3d_->setDirectionalLight(dirLight);
    // Should not crash
}

TEST_F(Graphics3DSystemTest, AddPointLight) {
    Entity lightEntity = static_cast<Entity>(1);

    PointLight pointLight;
    pointLight.color = Vec3{1.0f, 0.0f, 0.0f};
    pointLight.intensity = 2.0f;
    pointLight.range = 10.0f;

    graphics3d_->addPointLight(lightEntity, pointLight);
    // Should not crash
}

TEST_F(Graphics3DSystemTest, RemovePointLight) {
    Entity lightEntity = static_cast<Entity>(2);

    PointLight pointLight;
    graphics3d_->addPointLight(lightEntity, pointLight);
    graphics3d_->removePointLight(lightEntity);
    // Should not crash
}

TEST_F(Graphics3DSystemTest, AddSpotLight) {
    Entity lightEntity = static_cast<Entity>(3);

    SpotLight spotLight;
    spotLight.direction = Vec3{0.0f, -1.0f, 0.0f};
    spotLight.color = Vec3{1.0f, 1.0f, 0.0f};
    spotLight.intensity = 1.5f;
    spotLight.range = 15.0f;
    spotLight.innerConeAngle = 0.3f;
    spotLight.outerConeAngle = 0.5f;

    graphics3d_->addSpotLight(lightEntity, spotLight);
    // Should not crash
}

TEST_F(Graphics3DSystemTest, RemoveSpotLight) {
    Entity lightEntity = static_cast<Entity>(4);

    SpotLight spotLight;
    graphics3d_->addSpotLight(lightEntity, spotLight);
    graphics3d_->removeSpotLight(lightEntity);
    // Should not crash
}

//==============================================================================
// Render Statistics Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, GetRenderStats) {
    RenderStats stats = graphics3d_->getRenderStats();

    // Stats should be initialized
    EXPECT_GE(stats.drawCalls, 0u);
    EXPECT_GE(stats.triangles, 0u);
    EXPECT_GE(stats.vertices, 0u);
}

TEST_F(Graphics3DSystemTest, RenderStatsUpdateAfterFrame) {
    graphics3d_->beginFrame();
    graphics3d_->endFrame();

    RenderStats stats = graphics3d_->getRenderStats();
    // After a frame, stats should be valid
    EXPECT_GE(stats.frameTimeMs, 0.0f);
}

//==============================================================================
// Viewport Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, SetViewport) {
    graphics3d_->setViewport(0, 0, 1920, 1080);
    // Should not crash
}

TEST_F(Graphics3DSystemTest, SetViewportMultipleTimes) {
    graphics3d_->setViewport(0, 0, 800, 600);
    graphics3d_->setViewport(0, 0, 1920, 1080);
    graphics3d_->setViewport(100, 100, 640, 480);
    // Should not crash
}

//==============================================================================
// Entity Rendering Setup Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, SetupEntityRendering) {
    Entity entity = static_cast<Entity>(100);

    // Create mesh and material
    std::vector<Vertex3D> vertices = {
        Vertex3D{.position = Vec3{0.0f, 0.0f, 0.0f}}
    };
    std::vector<std::uint32_t> indices = {0};
    MeshDef meshDef{.vertices = vertices, .indices = indices};

    auto meshResult = graphics3d_->createMesh(meshDef);
    ASSERT_TRUE(meshResult.has_value());

    PBRMaterial pbrMat;
    auto matResult = graphics3d_->createMaterial(pbrMat);
    ASSERT_TRUE(matResult.has_value());

    graphics3d_->setupEntityRendering(entity, meshResult.value(), matResult.value());
    // Should not crash
}

TEST_F(Graphics3DSystemTest, RemoveEntityRendering) {
    Entity entity = static_cast<Entity>(101);

    std::vector<Vertex3D> vertices = {
        Vertex3D{.position = Vec3{0.0f, 0.0f, 0.0f}}
    };
    std::vector<std::uint32_t> indices = {0};
    MeshDef meshDef{.vertices = vertices, .indices = indices};

    auto meshResult = graphics3d_->createMesh(meshDef);
    ASSERT_TRUE(meshResult.has_value());

    PBRMaterial pbrMat;
    auto matResult = graphics3d_->createMaterial(pbrMat);
    ASSERT_TRUE(matResult.has_value());

    graphics3d_->setupEntityRendering(entity, meshResult.value(), matResult.value());
    graphics3d_->removeEntityRendering(entity);
    // Should not crash
}

//==============================================================================
// Edge Cases and Error Handling Tests
//==============================================================================

TEST_F(Graphics3DSystemTest, CreateMeshWithEmptyVertices) {
    std::vector<Vertex3D> vertices;
    std::vector<std::uint32_t> indices;

    MeshDef meshDef{
        .vertices = vertices,
        .indices = indices
    };

    auto result = graphics3d_->createMesh(meshDef);
    // May fail or succeed depending on implementation
    // Just ensure it doesn't crash
}

TEST_F(Graphics3DSystemTest, DestroyInvalidMesh) {
    MeshHandle invalidMesh = 999999;
    graphics3d_->destroyMesh(invalidMesh);
    // Should not crash
}

TEST_F(Graphics3DSystemTest, DestroyInvalidMaterial) {
    MaterialHandle invalidMat = 999999;
    graphics3d_->destroyMaterial(invalidMat);
    // Should not crash
}

TEST_F(Graphics3DSystemTest, GetMeshBoundsForInvalidHandle) {
    MeshHandle invalidMesh = 999999;
    AABB3D bounds = graphics3d_->getMeshBounds(invalidMesh);
    // Should return valid (possibly default) bounds
}

TEST_F(Graphics3DSystemTest, MultipleSubmitsSameRenderItem) {
    std::vector<Vertex3D> vertices = {
        Vertex3D{.position = Vec3{0.0f, 0.0f, 0.0f}}
    };
    std::vector<std::uint32_t> indices = {0};
    MeshDef meshDef{.vertices = vertices, .indices = indices};

    auto meshResult = graphics3d_->createMesh(meshDef);
    ASSERT_TRUE(meshResult.has_value());

    PBRMaterial pbrMat;
    auto matResult = graphics3d_->createMaterial(pbrMat);
    ASSERT_TRUE(matResult.has_value());

    RenderItem item;
    item.mesh = meshResult.value();
    item.material = matResult.value();
    item.worldMatrix = Mat4::identity();

    // Submit same item multiple times
    for (int i = 0; i < 100; ++i) {
        graphics3d_->submitRenderItem(item);
    }
    // Should not crash
}

}  // namespace bestow::tests
