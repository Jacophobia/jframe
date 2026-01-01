// bestow-luabind/src/bindings/graphics3d_binding.cpp
// 3D Graphics system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

// Helper to convert Result<T, Graphics3DError> to Lua (value) or nil on error
template<typename T>
sol::object gfxResultToLua(sol::state& lua, const Result<T, Graphics3DError>& result) {
    if (result.has_value()) {
        return sol::make_object(lua, result.value());
    }
    return sol::nil;
}

// Helper for Result<void, Graphics3DError>
sol::object gfxVoidResultToLua(sol::state& lua, const Result<void, Graphics3DError>& result) {
    if (result.has_value()) {
        return sol::make_object(lua, true);
    }
    return sol::make_object(lua, false);
}

void bindGraphics3DSystem(sol::state& lua, IGraphics3DSystem& graphics) {
    //=========================================================================
    // Graphics3D-related types
    //=========================================================================

    // Graphics3DError enum
    lua.new_enum<Graphics3DError>("Graphics3DError",
        {
            {"Success", Graphics3DError::Success},
            {"InvalidMesh", Graphics3DError::InvalidMesh},
            {"InvalidMaterial", Graphics3DError::InvalidMaterial},
            {"InvalidTexture", Graphics3DError::InvalidTexture},
            {"InvalidShader", Graphics3DError::InvalidShader},
            {"ShaderCompilationFailed", Graphics3DError::ShaderCompilationFailed},
            {"OutOfMemory", Graphics3DError::OutOfMemory},
            {"ContextLost", Graphics3DError::ContextLost},
            {"InternalError", Graphics3DError::InternalError}
        }
    );

    // BlendMode enum (from types)
    lua.new_enum<BlendMode>("BlendMode",
        {
            {"Opaque", BlendMode::Opaque},
            {"AlphaBlend", BlendMode::AlphaBlend},
            {"Additive", BlendMode::Additive},
            {"Multiply", BlendMode::Multiply}
        }
    );

    // CullMode enum (from types)
    lua.new_enum<CullMode>("CullMode",
        {
            {"None", CullMode::None},
            {"Back", CullMode::Back},
            {"Front", CullMode::Front}
        }
    );

    // TextAlignment3D enum
    lua.new_enum<TextAlignment3D>("TextAlignment3D",
        {
            {"Left", TextAlignment3D::Left},
            {"Center", TextAlignment3D::Center},
            {"Right", TextAlignment3D::Right}
        }
    );

    // TextVerticalAlign3D enum
    lua.new_enum<TextVerticalAlign3D>("TextVerticalAlign3D",
        {
            {"Top", TextVerticalAlign3D::Top},
            {"Middle", TextVerticalAlign3D::Middle},
            {"Bottom", TextVerticalAlign3D::Bottom}
        }
    );

    // PresentMode enum
    lua.new_enum<PresentMode>("PresentMode",
        {
            {"Immediate", PresentMode::Immediate},
            {"FIFO", PresentMode::FIFO},
            {"Mailbox", PresentMode::Mailbox}
        }
    );

    // ProjectionType enum
    lua.new_enum<ProjectionType>("ProjectionType",
        {
            {"Perspective", ProjectionType::Perspective},
            {"Orthographic", ProjectionType::Orthographic}
        }
    );

    // PBRMaterial struct
    lua.new_usertype<PBRMaterial>("PBRMaterial",
        sol::constructors<PBRMaterial()>(),
        "baseColorFactor", &PBRMaterial::baseColorFactor,
        "baseColorTexture", &PBRMaterial::baseColorTexture,
        "metallicFactor", &PBRMaterial::metallicFactor,
        "roughnessFactor", &PBRMaterial::roughnessFactor,
        "metallicRoughnessTexture", &PBRMaterial::metallicRoughnessTexture,
        "normalTexture", &PBRMaterial::normalTexture,
        "normalScale", &PBRMaterial::normalScale,
        "occlusionTexture", &PBRMaterial::occlusionTexture,
        "occlusionStrength", &PBRMaterial::occlusionStrength,
        "emissiveFactor", &PBRMaterial::emissiveFactor,
        "emissiveTexture", &PBRMaterial::emissiveTexture,
        "blendMode", &PBRMaterial::blendMode,
        "cullMode", &PBRMaterial::cullMode,
        "alphaCutoff", &PBRMaterial::alphaCutoff,
        "doubleSided", &PBRMaterial::doubleSided,
        "receiveShadows", &PBRMaterial::receiveShadows,
        "castShadows", &PBRMaterial::castShadows
    );

    // UnlitMaterial struct
    lua.new_usertype<UnlitMaterial>("UnlitMaterial",
        sol::constructors<UnlitMaterial()>(),
        "color", &UnlitMaterial::color,
        "texture", &UnlitMaterial::texture,
        "blendMode", &UnlitMaterial::blendMode,
        "cullMode", &UnlitMaterial::cullMode
    );

    // DirectionalLight struct
    lua.new_usertype<DirectionalLight>("DirectionalLight",
        sol::constructors<DirectionalLight()>(),
        "direction", &DirectionalLight::direction,
        "color", &DirectionalLight::color,
        "intensity", &DirectionalLight::intensity,
        "castShadows", &DirectionalLight::castShadows,
        "shadowMapResolution", &DirectionalLight::shadowMapResolution
    );

    // PointLight struct
    lua.new_usertype<PointLight>("PointLight",
        sol::constructors<PointLight()>(),
        "color", &PointLight::color,
        "intensity", &PointLight::intensity,
        "range", &PointLight::range,
        "castShadows", &PointLight::castShadows
    );

    // SpotLight struct
    lua.new_usertype<SpotLight>("SpotLight",
        sol::constructors<SpotLight()>(),
        "direction", &SpotLight::direction,
        "color", &SpotLight::color,
        "intensity", &SpotLight::intensity,
        "range", &SpotLight::range,
        "innerConeAngle", &SpotLight::innerConeAngle,
        "outerConeAngle", &SpotLight::outerConeAngle,
        "castShadows", &SpotLight::castShadows
    );

    // Skybox struct
    lua.new_usertype<Skybox>("Skybox",
        sol::constructors<Skybox()>(),
        "cubemapTexture", &Skybox::cubemapTexture,
        "rotation", &Skybox::rotation,
        "exposure", &Skybox::exposure
    );

    // Fog struct (from types)
    lua.new_usertype<Fog>("Fog",
        sol::constructors<Fog()>(),
        "enabled", &Fog::enabled,
        "color", &Fog::color,
        "density", &Fog::density,
        "startDistance", &Fog::startDistance,
        "endDistance", &Fog::endDistance
    );

    // Camera3D struct (from types)
    lua.new_usertype<Camera3D>("Camera3D",
        sol::constructors<Camera3D()>(),
        "transform", &Camera3D::transform,
        "projection", &Camera3D::projection,
        "fovY", &Camera3D::fovY,
        "aspectRatio", &Camera3D::aspectRatio,
        "orthoWidth", &Camera3D::orthoWidth,
        "orthoHeight", &Camera3D::orthoHeight,
        "nearPlane", &Camera3D::nearPlane,
        "farPlane", &Camera3D::farPlane
    );

    // RenderStats struct
    lua.new_usertype<RenderStats>("RenderStats",
        sol::constructors<RenderStats()>(),
        "drawCalls", &RenderStats::drawCalls,
        "triangles", &RenderStats::triangles,
        "vertices", &RenderStats::vertices,
        "meshes", &RenderStats::meshes,
        "materials", &RenderStats::materials,
        "textures", &RenderStats::textures,
        "lights", &RenderStats::lights,
        "visibleObjects", &RenderStats::visibleObjects,
        "culledObjects", &RenderStats::culledObjects,
        "frameTimeMs", &RenderStats::frameTimeMs,
        "gpuTimeMs", &RenderStats::gpuTimeMs
    );

    // AnimationClip struct
    lua.new_usertype<AnimationClip>("AnimationClip",
        sol::constructors<AnimationClip()>(),
        "name", &AnimationClip::name,
        "duration", &AnimationClip::duration,
        "looping", &AnimationClip::looping,
        "ticksPerSecond", &AnimationClip::ticksPerSecond
    );

    // AnimationState struct
    lua.new_usertype<AnimationState>("AnimationState",
        sol::constructors<AnimationState()>(),
        "clip", &AnimationState::clip,
        "time", &AnimationState::time,
        "speed", &AnimationState::speed,
        "weight", &AnimationState::weight,
        "playing", &AnimationState::playing,
        "looping", &AnimationState::looping
    );

    // Text3DStyle struct
    lua.new_usertype<Text3DStyle>("Text3DStyle",
        sol::constructors<Text3DStyle()>(),
        "font", &Text3DStyle::font,
        "fontSize", &Text3DStyle::fontSize,
        "color", &Text3DStyle::color,
        "alignment", &Text3DStyle::alignment,
        "verticalAlign", &Text3DStyle::verticalAlign,
        "lineSpacing", &Text3DStyle::lineSpacing,
        "letterSpacing", &Text3DStyle::letterSpacing,
        "billboard", &Text3DStyle::billboard,
        "outlineWidth", &Text3DStyle::outlineWidth,
        "outlineColor", &Text3DStyle::outlineColor
    );

    // Text3DItem struct
    lua.new_usertype<Text3DItem>("Text3DItem",
        sol::constructors<Text3DItem()>(),
        "text", &Text3DItem::text,
        "style", &Text3DItem::style,
        "transform", &Text3DItem::transform,
        "layer", &Text3DItem::layer
    );

    //=========================================================================
    // bestow.graphics3d table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table gfxTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Lifecycle
    //-------------------------------------------------------------------------

    gfxTable["isInitialized"] = [&graphics]() {
        return graphics.isInitialized();
    };

    //-------------------------------------------------------------------------
    // Frame Lifecycle
    //-------------------------------------------------------------------------

    gfxTable["beginFrame"] = [&graphics]() {
        graphics.beginFrame();
    };

    gfxTable["endFrame"] = [&graphics]() {
        graphics.endFrame();
    };

    //-------------------------------------------------------------------------
    // Primitive Mesh Generation
    //-------------------------------------------------------------------------

    gfxTable["createCubeMesh"] = [&graphics, &lua](sol::optional<float> size) -> sol::object {
        auto result = graphics.createCubeMesh(size.value_or(1.0f));
        return gfxResultToLua(lua, result);
    };

    gfxTable["createSphereMesh"] = [&graphics, &lua](sol::optional<float> radius,
                                                       sol::optional<std::uint32_t> segments,
                                                       sol::optional<std::uint32_t> rings) -> sol::object {
        auto result = graphics.createSphereMesh(
            radius.value_or(0.5f),
            segments.value_or(32),
            rings.value_or(16)
        );
        return gfxResultToLua(lua, result);
    };

    gfxTable["createCylinderMesh"] = [&graphics, &lua](sol::optional<float> radius,
                                                         sol::optional<float> height,
                                                         sol::optional<std::uint32_t> segments) -> sol::object {
        auto result = graphics.createCylinderMesh(
            radius.value_or(0.5f),
            height.value_or(1.0f),
            segments.value_or(32)
        );
        return gfxResultToLua(lua, result);
    };

    gfxTable["createCapsuleMesh"] = [&graphics, &lua](sol::optional<float> radius,
                                                        sol::optional<float> height,
                                                        sol::optional<std::uint32_t> segments,
                                                        sol::optional<std::uint32_t> rings) -> sol::object {
        auto result = graphics.createCapsuleMesh(
            radius.value_or(0.5f),
            height.value_or(1.0f),
            segments.value_or(32),
            rings.value_or(8)
        );
        return gfxResultToLua(lua, result);
    };

    gfxTable["createPlaneMesh"] = [&graphics, &lua](sol::optional<float> width,
                                                      sol::optional<float> height,
                                                      sol::optional<std::uint32_t> widthSegments,
                                                      sol::optional<std::uint32_t> heightSegments) -> sol::object {
        auto result = graphics.createPlaneMesh(
            width.value_or(1.0f),
            height.value_or(1.0f),
            widthSegments.value_or(1),
            heightSegments.value_or(1)
        );
        return gfxResultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // Mesh Management
    //-------------------------------------------------------------------------

    gfxTable["destroyMesh"] = [&graphics](MeshHandle handle) {
        graphics.destroyMesh(handle);
    };

    gfxTable["hasMesh"] = [&graphics](MeshHandle handle) {
        return graphics.hasMesh(handle);
    };

    gfxTable["getMeshBounds"] = [&graphics](MeshHandle handle) {
        return graphics.getMeshBounds(handle);
    };

    //-------------------------------------------------------------------------
    // Material Management
    //-------------------------------------------------------------------------

    gfxTable["createMaterial"] = [&graphics, &lua](const PBRMaterial& mat) -> sol::object {
        auto result = graphics.createMaterial(mat);
        return gfxResultToLua(lua, result);
    };

    gfxTable["createUnlitMaterial"] = [&graphics, &lua](const UnlitMaterial& mat) -> sol::object {
        auto result = graphics.createUnlitMaterial(mat);
        return gfxResultToLua(lua, result);
    };

    gfxTable["destroyMaterial"] = [&graphics](MaterialHandle handle) {
        graphics.destroyMaterial(handle);
    };

    gfxTable["hasMaterial"] = [&graphics](MaterialHandle handle) {
        return graphics.hasMaterial(handle);
    };

    gfxTable["getDefaultPBRMaterial"] = [&graphics]() {
        return graphics.getDefaultPBRMaterial();
    };

    gfxTable["getDefaultUnlitMaterial"] = [&graphics]() {
        return graphics.getDefaultUnlitMaterial();
    };

    gfxTable["getErrorMaterial"] = [&graphics]() {
        return graphics.getErrorMaterial();
    };

    //-------------------------------------------------------------------------
    // Material Property Updates
    //-------------------------------------------------------------------------

    gfxTable["setMaterialBaseColor"] = [&graphics, &lua](MaterialHandle handle, const Vec4& color) {
        auto result = graphics.setMaterialBaseColor(handle, color);
        return gfxVoidResultToLua(lua, result);
    };

    gfxTable["setMaterialMetallicRoughness"] = [&graphics, &lua](MaterialHandle handle, float metallic, float roughness) {
        auto result = graphics.setMaterialMetallicRoughness(handle, metallic, roughness);
        return gfxVoidResultToLua(lua, result);
    };

    gfxTable["setMaterialEmissive"] = [&graphics, &lua](MaterialHandle handle, const Vec3& emissive) {
        auto result = graphics.setMaterialEmissive(handle, emissive);
        return gfxVoidResultToLua(lua, result);
    };

    gfxTable["getMaterialProperties"] = [&graphics, &lua](MaterialHandle handle) -> sol::object {
        auto result = graphics.getMaterialProperties(handle);
        if (result) {
            return sol::make_object(lua, *result);
        }
        return sol::nil;
    };

    //-------------------------------------------------------------------------
    // Immediate Mode Rendering
    //-------------------------------------------------------------------------

    gfxTable["drawMesh"] = sol::overload(
        [&graphics](MeshHandle mesh, MaterialHandle material, const Mat4& worldMatrix,
                    sol::optional<bool> castShadow, sol::optional<bool> receiveShadow) {
            graphics.drawMesh(mesh, material, worldMatrix,
                             castShadow.value_or(true), receiveShadow.value_or(true));
        },
        [&graphics](MeshHandle mesh, MaterialHandle material, const Transform3D& transform,
                    sol::optional<bool> castShadow, sol::optional<bool> receiveShadow) {
            graphics.drawMesh(mesh, material, transform,
                             castShadow.value_or(true), receiveShadow.value_or(true));
        }
    );

    //-------------------------------------------------------------------------
    // Camera
    //-------------------------------------------------------------------------

    gfxTable["setCamera"] = [&graphics](const Camera3D& camera) {
        graphics.setCamera(camera);
    };

    gfxTable["getCamera"] = [&graphics]() {
        return graphics.getCamera();
    };

    gfxTable["screenToWorldRay"] = [&graphics](const Vec2& screenPos) {
        return graphics.screenToWorldRay(screenPos);
    };

    gfxTable["worldToScreen"] = [&graphics, &lua](const Vec3& worldPos) -> sol::object {
        auto result = graphics.worldToScreen(worldPos);
        if (result) {
            return sol::make_object(lua, *result);
        }
        return sol::nil;
    };

    //-------------------------------------------------------------------------
    // Lighting
    //-------------------------------------------------------------------------

    gfxTable["setDirectionalLight"] = [&graphics](const DirectionalLight& light) {
        graphics.setDirectionalLight(light);
    };

    gfxTable["clearDirectionalLight"] = [&graphics]() {
        graphics.clearDirectionalLight();
    };

    gfxTable["addPointLight"] = [&graphics](const PointLight& light, const Vec3& position) {
        return graphics.addPointLight(light, position);
    };

    gfxTable["addSpotLight"] = [&graphics](const SpotLight& light, const Vec3& position) {
        return graphics.addSpotLight(light, position);
    };

    gfxTable["setLightPosition"] = [&graphics](std::uint32_t lightId, const Vec3& position) {
        graphics.setLightPosition(lightId, position);
    };

    gfxTable["removeLight"] = [&graphics](std::uint32_t lightId) {
        graphics.removeLight(lightId);
    };

    gfxTable["clearLights"] = [&graphics]() {
        graphics.clearLights();
    };

    gfxTable["setAmbientLight"] = [&graphics](const Vec3& color, sol::optional<float> intensity) {
        graphics.setAmbientLight(color, intensity.value_or(1.0f));
    };

    //-------------------------------------------------------------------------
    // Environment
    //-------------------------------------------------------------------------

    gfxTable["setSkybox"] = [&graphics](const Skybox& skybox) {
        graphics.setSkybox(skybox);
    };

    gfxTable["clearSkybox"] = [&graphics]() {
        graphics.clearSkybox();
    };

    gfxTable["setFog"] = [&graphics](const Fog& fog) {
        graphics.setFog(fog);
    };

    //-------------------------------------------------------------------------
    // Shadows
    //-------------------------------------------------------------------------

    gfxTable["setShadowsEnabled"] = [&graphics](bool enabled) {
        graphics.setShadowsEnabled(enabled);
    };

    gfxTable["areShadowsEnabled"] = [&graphics]() {
        return graphics.areShadowsEnabled();
    };

    gfxTable["setDirectionalShadowResolution"] = [&graphics](int resolution) {
        graphics.setDirectionalShadowResolution(resolution);
    };

    gfxTable["setShadowDistance"] = [&graphics](float distance) {
        graphics.setShadowDistance(distance);
    };

    //-------------------------------------------------------------------------
    // Debug Rendering
    //-------------------------------------------------------------------------

    gfxTable["debugDrawLine"] = [&graphics](const Vec3& start, const Vec3& end,
                                             sol::optional<Color> color,
                                             sol::optional<float> duration,
                                             sol::optional<bool> depthTest) {
        graphics.debugDrawLine(start, end,
                               color.value_or(Color::white()),
                               duration.value_or(0.0f),
                               depthTest.value_or(true));
    };

    gfxTable["debugDrawBox"] = [&graphics](const Vec3& center, const Vec3& halfExtents,
                                            sol::optional<Quat> rotation,
                                            sol::optional<Color> color,
                                            sol::optional<float> duration,
                                            sol::optional<bool> depthTest) {
        graphics.debugDrawBox(center, halfExtents,
                              rotation.value_or(Quat{1.0f, 0.0f, 0.0f, 0.0f}),
                              color.value_or(Color::white()),
                              duration.value_or(0.0f),
                              depthTest.value_or(true));
    };

    gfxTable["debugDrawSphere"] = [&graphics](const Vec3& center, float radius,
                                               sol::optional<Color> color,
                                               sol::optional<float> duration,
                                               sol::optional<bool> depthTest) {
        graphics.debugDrawSphere(center, radius,
                                 color.value_or(Color::white()),
                                 duration.value_or(0.0f),
                                 depthTest.value_or(true));
    };

    gfxTable["debugDrawRay"] = [&graphics](const Vec3& origin, const Vec3& direction, float length,
                                            sol::optional<Color> color,
                                            sol::optional<float> duration,
                                            sol::optional<bool> depthTest) {
        graphics.debugDrawRay(origin, direction, length,
                              color.value_or(Color::white()),
                              duration.value_or(0.0f),
                              depthTest.value_or(true));
    };

    gfxTable["debugDrawAxes"] = [&graphics](const Transform3D& transform,
                                             sol::optional<float> size,
                                             sol::optional<float> duration,
                                             sol::optional<bool> depthTest) {
        graphics.debugDrawAxes(transform,
                               size.value_or(1.0f),
                               duration.value_or(0.0f),
                               depthTest.value_or(true));
    };

    gfxTable["debugDrawAABB"] = [&graphics](const AABB3D& aabb,
                                             sol::optional<Color> color,
                                             sol::optional<float> duration,
                                             sol::optional<bool> depthTest) {
        graphics.debugDrawAABB(aabb,
                               color.value_or(Color::white()),
                               duration.value_or(0.0f),
                               depthTest.value_or(true));
    };

    gfxTable["debugClear"] = [&graphics]() {
        graphics.debugClear();
    };

    gfxTable["setDebugRenderingEnabled"] = [&graphics](bool enabled) {
        graphics.setDebugRenderingEnabled(enabled);
    };

    gfxTable["isDebugRenderingEnabled"] = [&graphics]() {
        return graphics.isDebugRenderingEnabled();
    };

    //-------------------------------------------------------------------------
    // Window Management
    //-------------------------------------------------------------------------

    gfxTable["getWindowSize"] = [&graphics]() {
        return graphics.getWindowSize();
    };

    gfxTable["setWindowSize"] = [&graphics](Size size) {
        graphics.setWindowSize(size);
    };

    gfxTable["isFullscreen"] = [&graphics]() {
        return graphics.isFullscreen();
    };

    gfxTable["setFullscreen"] = [&graphics](bool fullscreen) {
        graphics.setFullscreen(fullscreen);
    };

    gfxTable["shouldClose"] = [&graphics]() {
        return graphics.shouldClose();
    };

    //-------------------------------------------------------------------------
    // Render State
    //-------------------------------------------------------------------------

    gfxTable["setClearColor"] = [&graphics](const Color& color) {
        graphics.setClearColor(color);
    };

    gfxTable["setVSync"] = [&graphics](bool enabled) {
        graphics.setVSync(enabled);
    };

    gfxTable["setRenderScale"] = [&graphics](float scale) {
        graphics.setRenderScale(scale);
    };

    gfxTable["getRenderScale"] = [&graphics]() {
        return graphics.getRenderScale();
    };

    //-------------------------------------------------------------------------
    // Culling
    //-------------------------------------------------------------------------

    gfxTable["setFrustumCulling"] = [&graphics](bool enabled) {
        graphics.setFrustumCulling(enabled);
    };

    gfxTable["isFrustumCullingEnabled"] = [&graphics]() {
        return graphics.isFrustumCullingEnabled();
    };

    //-------------------------------------------------------------------------
    // Post-Processing
    //-------------------------------------------------------------------------

    gfxTable["setToneMapping"] = [&graphics](bool enabled) {
        graphics.setToneMapping(enabled);
    };

    gfxTable["setExposure"] = [&graphics](float exposure) {
        graphics.setExposure(exposure);
    };

    gfxTable["setBloom"] = [&graphics](bool enabled, sol::optional<float> threshold, sol::optional<float> intensity) {
        graphics.setBloom(enabled, threshold.value_or(1.0f), intensity.value_or(1.0f));
    };

    gfxTable["setSSAO"] = [&graphics](bool enabled, sol::optional<float> radius, sol::optional<float> intensity) {
        graphics.setSSAO(enabled, radius.value_or(0.5f), intensity.value_or(1.0f));
    };

    //-------------------------------------------------------------------------
    // Statistics
    //-------------------------------------------------------------------------

    gfxTable["getStats"] = [&graphics]() {
        return graphics.getStats();
    };

    //-------------------------------------------------------------------------
    // Skeletal Animation (Graphics-side)
    //-------------------------------------------------------------------------

    gfxTable["getAnimationClipNames"] = [&graphics](SkeletonHandle skeleton) {
        return graphics.getAnimationClipNames(skeleton);
    };

    gfxTable["getAnimationClipInfo"] = [&graphics](AnimationClipHandle clip) {
        return graphics.getAnimationClipInfo(clip);
    };

    gfxTable["destroySkeleton"] = [&graphics](SkeletonHandle skeleton) {
        graphics.destroySkeleton(skeleton);
    };

    gfxTable["destroyAnimationClip"] = [&graphics](AnimationClipHandle clip) {
        graphics.destroyAnimationClip(clip);
    };

    //-------------------------------------------------------------------------
    // 3D Text Rendering
    //-------------------------------------------------------------------------

    gfxTable["destroyFont3D"] = [&graphics](Font3DHandle font) {
        graphics.destroyFont3D(font);
    };

    gfxTable["drawText3D"] = sol::overload(
        [&graphics](const Text3DItem& item) {
            graphics.drawText3D(item);
        },
        [&graphics](const std::string& text, const Vec3& position, Font3DHandle font,
                    sol::optional<float> fontSize, sol::optional<Color> color) {
            graphics.drawText3D(text, position, font,
                               fontSize.value_or(1.0f),
                               color.value_or(Color::white()));
        }
    );

    gfxTable["measureText3D"] = [&graphics](const std::string& text, Font3DHandle font, sol::optional<float> fontSize) {
        return graphics.measureText3D(text, font, fontSize.value_or(1.0f));
    };

    //-------------------------------------------------------------------------
    // LOD
    //-------------------------------------------------------------------------

    gfxTable["setLODBias"] = [&graphics](float bias) {
        graphics.setLODBias(bias);
    };

    bestow["graphics3d"] = gfxTable;
}

}  // namespace bestow
