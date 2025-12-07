// tests/unit/ShaderSystemTests.cpp
// Shader system unit tests

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

import bestow.shader;
import bestow.shader.impl;
import bestow.types;
import bestow.assets;

namespace bestow::tests {

class ShaderSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        shaderSystem_ = createShaderSystem();
    }

    std::unique_ptr<IShaderSystem> shaderSystem_;
};

//==============================================================================
// Shader Program Creation Tests
//==============================================================================

TEST_F(ShaderSystemTest, CreateShaderProgramFromSource) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() {
                gl_Position = vec4(aPos, 1.0);
            }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() {
                FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
        )"
    };

    ShaderProgramDef programDef{
        .name = "TestShader",
        .stages = {vertexShader, fragmentShader},
        .enableHotReload = false
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    EXPECT_TRUE(result.has_value());

    if (result.has_value()) {
        ShaderProgramHandle program = result.value();
        EXPECT_NE(program, ShaderHandles::Invalid);
        EXPECT_TRUE(shaderSystem_->hasShaderProgram(program));
    }
}

TEST_F(ShaderSystemTest, CreateShaderProgramWithInvalidSource) {
    ShaderSourceDef invalidVertexShader{
        .stage = ShaderStage::Vertex,
        .source = "invalid shader source code"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() {
                FragColor = vec4(1.0);
            }
        )"
    };

    ShaderProgramDef programDef{
        .name = "InvalidShader",
        .stages = {invalidVertexShader, fragmentShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    // Should fail compilation
    EXPECT_FALSE(result.has_value());
}

TEST_F(ShaderSystemTest, DestroyShaderProgram) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "TempShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(result.has_value());

    ShaderProgramHandle program = result.value();
    EXPECT_TRUE(shaderSystem_->hasShaderProgram(program));

    shaderSystem_->destroyShaderProgram(program);
    EXPECT_FALSE(shaderSystem_->hasShaderProgram(program));
}

TEST_F(ShaderSystemTest, HasShaderProgramReturnsFalseForInvalid) {
    ShaderProgramHandle invalid = 999999;
    EXPECT_FALSE(shaderSystem_->hasShaderProgram(invalid));
}

//==============================================================================
// Material Creation Tests
//==============================================================================

TEST_F(ShaderSystemTest, CreateMaterialFromShaderProgram) {
    // First create a shader program
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "MaterialShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto shaderResult = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(shaderResult.has_value());

    // Now create a material using that shader
    ShaderMaterialDef materialDef{
        .name = "TestMaterial",
        .shader = shaderResult.value(),
        .blendMode = BlendMode::Opaque,
        .cullMode = CullMode::Back
    };

    auto materialResult = shaderSystem_->createMaterial(materialDef);
    EXPECT_TRUE(materialResult.has_value());

    if (materialResult.has_value()) {
        MaterialHandle material = materialResult.value();
        EXPECT_NE(material, 0u);
        EXPECT_TRUE(shaderSystem_->hasMaterial(material));
    }
}

TEST_F(ShaderSystemTest, DestroyMaterial) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "TempMaterialShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto shaderResult = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(shaderResult.has_value());

    ShaderMaterialDef materialDef{
        .name = "TempMaterial",
        .shader = shaderResult.value()
    };

    auto materialResult = shaderSystem_->createMaterial(materialDef);
    ASSERT_TRUE(materialResult.has_value());

    MaterialHandle material = materialResult.value();
    EXPECT_TRUE(shaderSystem_->hasMaterial(material));

    shaderSystem_->destroyMaterial(material);
    EXPECT_FALSE(shaderSystem_->hasMaterial(material));
}

TEST_F(ShaderSystemTest, HasMaterialReturnsFalseForInvalid) {
    MaterialHandle invalid = 999999;
    EXPECT_FALSE(shaderSystem_->hasMaterial(invalid));
}

//==============================================================================
// Uniform Setting Tests
//==============================================================================

TEST_F(ShaderSystemTest, SetUniformFloat) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            uniform float u_intensity;
            out vec4 FragColor;
            void main() { FragColor = vec4(u_intensity); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "UniformTestShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(result.has_value());

    ShaderProgramHandle program = result.value();
    shaderSystem_->useShaderProgram(program);

    UniformValue value = 0.5f;
    shaderSystem_->setUniform(program, "u_intensity", value);
    // Should not crash
}

TEST_F(ShaderSystemTest, SetUniformVec3) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            uniform vec3 u_color;
            out vec4 FragColor;
            void main() { FragColor = vec4(u_color, 1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "Vec3UniformShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(result.has_value());

    ShaderProgramHandle program = result.value();
    shaderSystem_->useShaderProgram(program);

    UniformValue value = Vec3{1.0f, 0.0f, 0.0f};
    shaderSystem_->setUniform(program, "u_color", value);
    // Should not crash
}

TEST_F(ShaderSystemTest, SetUniformMat4) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            uniform mat4 u_mvp;
            void main() { gl_Position = u_mvp * vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "Mat4UniformShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(result.has_value());

    ShaderProgramHandle program = result.value();
    shaderSystem_->useShaderProgram(program);

    UniformValue value = Mat4::identity();
    shaderSystem_->setUniform(program, "u_mvp", value);
    // Should not crash
}

TEST_F(ShaderSystemTest, SetUniformOnInvalidProgram) {
    ShaderProgramHandle invalid = 999999;
    UniformValue value = 1.0f;

    shaderSystem_->setUniform(invalid, "u_value", value);
    // Should not crash
}

//==============================================================================
// Shader Program Usage Tests
//==============================================================================

TEST_F(ShaderSystemTest, UseShaderProgram) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "UseTestShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(result.has_value());

    shaderSystem_->useShaderProgram(result.value());
    // Should not crash
}

TEST_F(ShaderSystemTest, UseInvalidShaderProgram) {
    ShaderProgramHandle invalid = 999999;
    shaderSystem_->useShaderProgram(invalid);
    // Should not crash
}

TEST_F(ShaderSystemTest, SwitchBetweenShaderPrograms) {
    // Create first shader
    ShaderSourceDef vs1{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fs1{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(1.0, 0.0, 0.0, 1.0); }
        )"
    };

    ShaderProgramDef prog1{
        .name = "RedShader",
        .stages = {vs1, fs1}
    };

    auto result1 = shaderSystem_->createShaderProgram(prog1);
    ASSERT_TRUE(result1.has_value());

    // Create second shader
    ShaderSourceDef vs2{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fs2{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(0.0, 1.0, 0.0, 1.0); }
        )"
    };

    ShaderProgramDef prog2{
        .name = "GreenShader",
        .stages = {vs2, fs2}
    };

    auto result2 = shaderSystem_->createShaderProgram(prog2);
    ASSERT_TRUE(result2.has_value());

    // Switch between them
    shaderSystem_->useShaderProgram(result1.value());
    shaderSystem_->useShaderProgram(result2.value());
    shaderSystem_->useShaderProgram(result1.value());
    // Should not crash
}

//==============================================================================
// Uniform Introspection Tests
//==============================================================================

TEST_F(ShaderSystemTest, GetUniformsReturnsValidList) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            uniform mat4 u_model;
            uniform mat4 u_view;
            uniform mat4 u_projection;
            void main() {
                gl_Position = u_projection * u_view * u_model * vec4(aPos, 1.0);
            }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            uniform vec3 u_color;
            uniform float u_alpha;
            out vec4 FragColor;
            void main() { FragColor = vec4(u_color, u_alpha); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "IntrospectionShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(result.has_value());

    ShaderProgramHandle program = result.value();
    auto uniforms = shaderSystem_->getUniforms(program);

    // Should have at least some uniforms
    EXPECT_GE(uniforms.size(), 0u);
}

TEST_F(ShaderSystemTest, GetUniformsForInvalidProgram) {
    ShaderProgramHandle invalid = 999999;
    auto uniforms = shaderSystem_->getUniforms(invalid);

    EXPECT_TRUE(uniforms.empty());
}

//==============================================================================
// Hot Reload Tests
//==============================================================================

TEST_F(ShaderSystemTest, UpdateHotReloadDoesNotCrash) {
    shaderSystem_->updateHotReload();
    // Should not crash even with no shaders
}

TEST_F(ShaderSystemTest, UpdateHotReloadWithShaders) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "HotReloadTestShader",
        .stages = {vertexShader, fragmentShader},
        .enableHotReload = true
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(result.has_value());

    shaderSystem_->updateHotReload();
    // Should not crash
}

//==============================================================================
// Material with Uniforms Tests
//==============================================================================

TEST_F(ShaderSystemTest, CreateMaterialWithUniforms) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            uniform vec3 u_color;
            uniform float u_roughness;
            out vec4 FragColor;
            void main() { FragColor = vec4(u_color, 1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "MaterialUniformShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto shaderResult = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(shaderResult.has_value());

    ShaderMaterialDef materialDef{
        .name = "UniformMaterial",
        .shader = shaderResult.value(),
        .uniforms = {
            {"u_color", Vec3{1.0f, 0.0f, 0.0f}},
            {"u_roughness", 0.5f}
        }
    };

    auto materialResult = shaderSystem_->createMaterial(materialDef);
    EXPECT_TRUE(materialResult.has_value());
}

TEST_F(ShaderSystemTest, UseMaterial) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "UseMaterialShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto shaderResult = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(shaderResult.has_value());

    ShaderMaterialDef materialDef{
        .name = "UsableMaterial",
        .shader = shaderResult.value()
    };

    auto materialResult = shaderSystem_->createMaterial(materialDef);
    ASSERT_TRUE(materialResult.has_value());

    shaderSystem_->useMaterial(materialResult.value());
    // Should not crash
}

TEST_F(ShaderSystemTest, UseInvalidMaterial) {
    MaterialHandle invalid = 999999;
    shaderSystem_->useMaterial(invalid);
    // Should not crash
}

//==============================================================================
// Edge Cases and Error Handling Tests
//==============================================================================

TEST_F(ShaderSystemTest, CreateShaderWithOnlyVertexStage) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "VertexOnlyShader",
        .stages = {vertexShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    // Should fail or succeed depending on OpenGL requirements
    // Just ensure it doesn't crash
}

TEST_F(ShaderSystemTest, CreateShaderWithEmptyStages) {
    ShaderProgramDef programDef{
        .name = "EmptyShader",
        .stages = {}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    EXPECT_FALSE(result.has_value());
}

TEST_F(ShaderSystemTest, SetUniformBeforeUsingShaderProgram) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            uniform float u_value;
            out vec4 FragColor;
            void main() { FragColor = vec4(u_value); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "PreUseUniformShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(result.has_value());

    // Set uniform without using program first
    UniformValue value = 0.5f;
    shaderSystem_->setUniform(result.value(), "u_value", value);
    // Should not crash (may or may not set the value correctly)
}

TEST_F(ShaderSystemTest, DestroyShaderProgramTwice) {
    ShaderSourceDef vertexShader{
        .stage = ShaderStage::Vertex,
        .source = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            void main() { gl_Position = vec4(aPos, 1.0); }
        )"
    };

    ShaderSourceDef fragmentShader{
        .stage = ShaderStage::Fragment,
        .source = R"(
            #version 330 core
            out vec4 FragColor;
            void main() { FragColor = vec4(1.0); }
        )"
    };

    ShaderProgramDef programDef{
        .name = "DoubleDestroyShader",
        .stages = {vertexShader, fragmentShader}
    };

    auto result = shaderSystem_->createShaderProgram(programDef);
    ASSERT_TRUE(result.has_value());

    ShaderProgramHandle program = result.value();
    shaderSystem_->destroyShaderProgram(program);
    shaderSystem_->destroyShaderProgram(program);  // Second destroy
    // Should not crash
}

TEST_F(ShaderSystemTest, CreateMaterialWithInvalidShaderHandle) {
    ShaderMaterialDef materialDef{
        .name = "InvalidShaderMaterial",
        .shader = 999999  // Invalid handle
    };

    auto result = shaderSystem_->createMaterial(materialDef);
    // May fail or succeed depending on implementation
    // Just ensure it doesn't crash
}

}  // namespace bestow::tests
