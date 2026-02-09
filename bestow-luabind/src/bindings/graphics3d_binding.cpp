// bestow-luabind/src/bindings/graphics3d_binding.cpp
// 3D Graphics system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <bestow/entt_compat.hpp>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

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

void bindGraphics3DSystem(sol::state& lua, IGraphics3DSystem& graphics,
                          IEntitySystem* entities, IAnimationSystem* animation) {
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

    // WindowMode enum
    lua.new_enum<WindowMode>("WindowMode",
        {
            {"Windowed", WindowMode::Windowed},
            {"Fullscreen", WindowMode::Fullscreen},
            {"BorderlessFullscreen", WindowMode::BorderlessFullscreen}
        }
    );

    // LockPointSource enum
    lua.new_enum<LockPointSource>("LockPointSource",
        {
            {"Socket", LockPointSource::Socket},
            {"Offset", LockPointSource::Offset}
        }
    );

    // LockPointDef struct
    lua.new_usertype<LockPointDef>("LockPointDef",
        sol::constructors<LockPointDef()>(),
        "name", &LockPointDef::name,
        "source", &LockPointDef::source,
        "socketName", &LockPointDef::socketName,
        "localOffset", &LockPointDef::localOffset,
        "priority", &LockPointDef::priority
    );
    lua["LockPointDef"]["new"] = [](sol::optional<sol::table> tbl) {
        LockPointDef def;
        if (tbl) {
            if (auto name = (*tbl)["name"]; name.valid()) def.name = name.get<std::string>();
            if (auto source = (*tbl)["source"]; source.valid()) {
                if (source.get_type() == sol::type::string) {
                    std::string s = source.get<std::string>();
                    def.source = (s == "socket") ? LockPointSource::Socket : LockPointSource::Offset;
                } else {
                    def.source = source.get<LockPointSource>();
                }
            }
            if (auto socketName = (*tbl)["socketName"]; socketName.valid()) def.socketName = socketName.get<std::string>();
            if (auto offset = (*tbl)["localOffset"]; offset.valid()) def.localOffset = offset.get<Vec3>();
            if (auto priority = (*tbl)["priority"]; priority.valid()) def.priority = priority.get<float>();
        }
        return def;
    };

    // LockableTarget struct
    lua.new_usertype<LockableTarget>("LockableTarget",
        sol::constructors<LockableTarget()>(),
        "lockPoints", &LockableTarget::lockPoints,
        "enabled", &LockableTarget::enabled
    );
    lua["LockableTarget"]["new"] = [](sol::optional<sol::table> tbl) {
        LockableTarget target;
        if (tbl) {
            if (auto enabled = (*tbl)["enabled"]; enabled.valid()) target.enabled = enabled.get<bool>();
            if (auto lockPoints = (*tbl)["lockPoints"]; lockPoints.valid() && lockPoints.get_type() == sol::type::table) {
                sol::table points = lockPoints.get<sol::table>();
                for (auto& pair : points) {
                    if (pair.second.is<LockPointDef>()) {
                        target.lockPoints.push_back(pair.second.as<LockPointDef>());
                    } else if (pair.second.is<sol::table>()) {
                        sol::table pt = pair.second.as<sol::table>();
                        LockPointDef def;
                        if (auto name = pt["name"]; name.valid()) def.name = name.get<std::string>();
                        if (auto source = pt["source"]; source.valid()) {
                            if (source.get_type() == sol::type::string) {
                                std::string s = source.get<std::string>();
                                def.source = (s == "socket") ? LockPointSource::Socket : LockPointSource::Offset;
                            }
                        }
                        if (auto socketName = pt["socketName"]; socketName.valid()) def.socketName = socketName.get<std::string>();
                        if (auto offset = pt["localOffset"]; offset.valid()) def.localOffset = offset.get<Vec3>();
                        if (auto priority = pt["priority"]; priority.valid()) def.priority = priority.get<float>();
                        target.lockPoints.push_back(def);
                    }
                }
            }
        }
        return target;
    };

    // LockOnConfig struct
    lua.new_usertype<LockOnConfig>("LockOnConfig",
        sol::constructors<LockOnConfig()>(),
        "maxRange", &LockOnConfig::maxRange,
        "fovMargin", &LockOnConfig::fovMargin,
        "centerBias", &LockOnConfig::centerBias,
        "priorityWeight", &LockOnConfig::priorityWeight,
        "preferCurrentTarget", &LockOnConfig::preferCurrentTarget,
        "hysteresis", &LockOnConfig::hysteresis
    );
    lua["LockOnConfig"]["new"] = [](sol::optional<sol::table> tbl) {
        LockOnConfig cfg;
        if (tbl) {
            if (auto v = (*tbl)["maxRange"]; v.valid()) cfg.maxRange = v.get<float>();
            if (auto v = (*tbl)["fovMargin"]; v.valid()) cfg.fovMargin = v.get<float>();
            if (auto v = (*tbl)["centerBias"]; v.valid()) cfg.centerBias = v.get<float>();
            if (auto v = (*tbl)["priorityWeight"]; v.valid()) cfg.priorityWeight = v.get<float>();
            if (auto v = (*tbl)["preferCurrentTarget"]; v.valid()) cfg.preferCurrentTarget = v.get<bool>();
            if (auto v = (*tbl)["hysteresis"]; v.valid()) cfg.hysteresis = v.get<float>();
        }
        return cfg;
    };

    // LockOnResult struct
    lua.new_usertype<LockOnResult>("LockOnResult",
        sol::constructors<LockOnResult()>(),
        "entity", &LockOnResult::entity,
        "lockPointIndex", &LockOnResult::lockPointIndex,
        "worldPosition", &LockOnResult::worldPosition,
        "screenPosition", &LockOnResult::screenPosition,
        "distance", &LockOnResult::distance,
        "score", &LockOnResult::score,
        "isValid", &LockOnResult::isValid
    );

    // PBRMaterial struct
    lua.new_usertype<PBRMaterial>("PBRMaterial",
        sol::constructors<PBRMaterial()>(),
        // Accept both Vec4 and Color for baseColorFactor
        "baseColorFactor", sol::property(
            [](const PBRMaterial& m) { return m.baseColorFactor; },
            [](PBRMaterial& m, sol::object value) {
                if (value.is<Vec4>()) {
                    m.baseColorFactor = value.as<Vec4>();
                } else if (value.is<Color>()) {
                    Color c = value.as<Color>();
                    m.baseColorFactor = Vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
                }
            }
        ),
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
    lua["PBRMaterial"]["new"] = []() { return PBRMaterial{}; };

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
    lua["Camera3D"]["new"] = []() { return Camera3D{}; };

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

    // Graphics3DConfig struct
    lua.new_usertype<Graphics3DConfig>("Graphics3DConfig",
        sol::constructors<Graphics3DConfig()>(),
        "windowWidth", &Graphics3DConfig::windowWidth,
        "windowHeight", &Graphics3DConfig::windowHeight,
        "windowTitle", &Graphics3DConfig::windowTitle,
        "vsync", &Graphics3DConfig::vsync,
        "windowMode", &Graphics3DConfig::windowMode,
        "enableValidation", &Graphics3DConfig::enableValidation,
        "nativeWindowHandle", &Graphics3DConfig::nativeWindowHandle
    );

    //=========================================================================
    // Register LockableTarget component factory (for Lua entity access)
    //=========================================================================

    if (entities) {
        // Register component type info for documentation/inspection
        ComponentTypeInfo lockableInfo;
        lockableInfo.name = "LockableTarget";
        lockableInfo.fields = {
            {"enabled", ComponentFieldType::Bool},
            {"lockPoints", ComponentFieldType::Array}
        };
        lockableInfo.canConstruct = true;
        entities->registerComponentType("LockableTarget", lockableInfo);

        spdlog::debug("[graphics3d] Registered LockableTarget component type info");
    }

    //=========================================================================
    // bestow.graphics3d table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table gfxTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Lifecycle
    //-------------------------------------------------------------------------

    gfxTable["initialize"] = [&graphics](sol::table configTable) {
        Graphics3DConfig config{};
        config.windowWidth = configTable.get_or("windowWidth", 1280);
        config.windowHeight = configTable.get_or("windowHeight", 720);
        config.windowTitle = configTable.get_or("windowTitle", std::string("Bestow"));
        config.vsync = configTable.get_or("vsync", true);
        // Parse windowMode string or fall back to legacy fullscreen bool
        auto wmStr = configTable.get<sol::optional<std::string>>("windowMode");
        if (wmStr) {
            if (*wmStr == "borderless") config.windowMode = WindowMode::BorderlessFullscreen;
            else if (*wmStr == "fullscreen") config.windowMode = WindowMode::Fullscreen;
            else config.windowMode = WindowMode::Windowed;
        } else {
            bool fs = configTable.get_or("fullscreen", false);
            config.windowMode = fs ? WindowMode::BorderlessFullscreen : WindowMode::Windowed;
        }
        config.enableValidation = configTable.get_or("enableValidation", false);
        return graphics.initialize(config);
    };

    gfxTable["shutdown"] = [&graphics]() {
        graphics.shutdown();
    };

    gfxTable["getNativeWindowHandle"] = [&graphics]() {
        return graphics.getNativeWindowHandle();
    };

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

    // setCamera accepts either Camera3D userdata OR a table with look-at style parameters:
    // { position = Vec3, target = Vec3, up = Vec3, fov = number, near = number, far = number }
    gfxTable["setCamera"] = [&graphics](sol::object cameraArg) {
        if (cameraArg.is<Camera3D>()) {
            // Direct Camera3D userdata
            graphics.setCamera(cameraArg.as<Camera3D>());
        } else if (cameraArg.get_type() == sol::type::table) {
            // Look-at style table
            sol::table t = cameraArg.as<sol::table>();

            Camera3D camera;

            // Extract position (required)
            if (t["position"].valid()) {
                camera.transform.position = t["position"].get<Vec3>();
            }

            // Compute rotation from look-at target
            if (t["target"].valid()) {
                Vec3 target = t["target"].get<Vec3>();
                Vec3 up = Vec3{0.0f, 1.0f, 0.0f};
                if (t["up"].valid()) {
                    up = t["up"].get<Vec3>();
                }

                // Calculate look-at quaternion
                Vec3 forward = glm::normalize(target - camera.transform.position);

                // Handle degenerate case where forward is parallel to up
                float dot = glm::dot(forward, up);
                if (std::abs(dot) > 0.999f) {
                    // Use alternative up vector
                    up = std::abs(forward.y) < 0.999f ? Vec3{0, 1, 0} : Vec3{1, 0, 0};
                }

                // Create rotation from look direction
                // Note: cameras typically look down -Z, so we need to invert
                camera.transform.rotation = glm::quatLookAt(forward, up);
            }

            // FOV (default 60 degrees)
            if (t["fov"].valid()) {
                camera.fovY = t["fov"].get<float>();
            } else if (t["fovY"].valid()) {
                camera.fovY = t["fovY"].get<float>();
            }

            // Near/far planes
            if (t["near"].valid()) {
                camera.nearPlane = t["near"].get<float>();
            } else if (t["nearPlane"].valid()) {
                camera.nearPlane = t["nearPlane"].get<float>();
            }

            if (t["far"].valid()) {
                camera.farPlane = t["far"].get<float>();
            } else if (t["farPlane"].valid()) {
                camera.farPlane = t["farPlane"].get<float>();
            }

            // Aspect ratio (optional - usually set from window)
            if (t["aspectRatio"].valid()) {
                camera.aspectRatio = t["aspectRatio"].get<float>();
            }

            graphics.setCamera(camera);
        } else {
            spdlog::error("[graphics3d.setCamera] Invalid argument: expected Camera3D or table");
        }
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

    gfxTable["setDirectionalLight"] = [&graphics](sol::table lightTable) {
        DirectionalLight light;
        if (lightTable["direction"].valid()) {
            light.direction = lightTable["direction"].get<Vec3>();
        }
        if (lightTable["color"].valid()) {
            light.color = lightTable["color"].get<Vec3>();
        }
        if (lightTable["intensity"].valid()) {
            light.intensity = lightTable["intensity"].get<float>();
        }
        if (lightTable["castShadows"].valid()) {
            light.castShadows = lightTable["castShadows"].get<bool>();
        }
        if (lightTable["shadowMapResolution"].valid()) {
            light.shadowMapResolution = lightTable["shadowMapResolution"].get<int>();
        }
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

    gfxTable["getWindowMode"] = [&graphics]() {
        return graphics.getWindowMode();
    };

    gfxTable["setWindowMode"] = [&graphics](sol::object arg) {
        if (arg.is<WindowMode>()) {
            graphics.setWindowMode(arg.as<WindowMode>());
        } else if (arg.is<std::string>()) {
            auto s = arg.as<std::string>();
            if (s == "borderless") graphics.setWindowMode(WindowMode::BorderlessFullscreen);
            else if (s == "fullscreen") graphics.setWindowMode(WindowMode::Fullscreen);
            else graphics.setWindowMode(WindowMode::Windowed);
        }
    };

    // Compat wrappers
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

    //-------------------------------------------------------------------------
    // Lock-On Targeting System
    //-------------------------------------------------------------------------

    gfxTable["setLockOnConfig"] = [&graphics](sol::object configObj) {
        LockOnConfig cfg;
        if (configObj.is<LockOnConfig>()) {
            cfg = configObj.as<LockOnConfig>();
        } else if (configObj.is<sol::table>()) {
            sol::table tbl = configObj.as<sol::table>();
            if (auto v = tbl["maxRange"]; v.valid()) cfg.maxRange = v.get<float>();
            if (auto v = tbl["fovMargin"]; v.valid()) cfg.fovMargin = v.get<float>();
            if (auto v = tbl["centerBias"]; v.valid()) cfg.centerBias = v.get<float>();
            if (auto v = tbl["priorityWeight"]; v.valid()) cfg.priorityWeight = v.get<float>();
            if (auto v = tbl["preferCurrentTarget"]; v.valid()) cfg.preferCurrentTarget = v.get<bool>();
            if (auto v = tbl["hysteresis"]; v.valid()) cfg.hysteresis = v.get<float>();
        }
        graphics.setLockOnConfig(cfg);
    };

    gfxTable["getLockOnConfig"] = [&graphics]() {
        return graphics.getLockOnConfig();
    };

    gfxTable["lockOn"] = [&graphics, entities, animation](sol::this_state s) -> sol::object {
        if (!entities) {
            spdlog::warn("[graphics3d.lockOn] Entity system not available");
            return sol::nil;
        }
        LockOnResult result = graphics.lockOn(*entities, animation);
        if (result.isValid()) {
            return sol::make_object(s, result);
        }
        return sol::nil;
    };

    gfxTable["getLockTarget"] = [&graphics](sol::this_state s) -> sol::object {
        auto result = graphics.getLockTarget();
        if (result && result->isValid()) {
            return sol::make_object(s, *result);
        }
        return sol::nil;
    };

    gfxTable["pollLockPosition"] = [&graphics, entities, animation](sol::this_state s) -> sol::object {
        if (!entities) {
            spdlog::warn("[graphics3d.pollLockPosition] Entity system not available");
            return sol::nil;
        }
        auto pos = graphics.pollLockPosition(*entities, animation);
        if (pos) {
            return sol::make_object(s, *pos);
        }
        return sol::nil;
    };

    gfxTable["shiftLockTarget"] = [&graphics, entities, animation](const Vec2& direction, sol::this_state s) -> sol::object {
        if (!entities) {
            spdlog::warn("[graphics3d.shiftLockTarget] Entity system not available");
            return sol::nil;
        }
        LockOnResult result = graphics.shiftLockTarget(direction, *entities, animation);
        if (result.isValid()) {
            return sol::make_object(s, result);
        }
        return sol::nil;
    };

    gfxTable["unlock"] = [&graphics]() {
        graphics.unlock();
    };

    gfxTable["isLocked"] = [&graphics]() {
        return graphics.isLocked();
    };

    gfxTable["getPotentialTargets"] = [&graphics, entities, animation]() -> std::vector<LockOnResult> {
        if (!entities) {
            spdlog::warn("[graphics3d.getPotentialTargets] Entity system not available");
            return {};
        }
        return graphics.getPotentialTargets(*entities, animation);
    };

    //-------------------------------------------------------------------------
    // LockableTarget Component Helpers
    //-------------------------------------------------------------------------

    gfxTable["addLockableTarget"] = [entities](Entity entity, sol::optional<sol::object> targetObj) {
        if (!entities) {
            spdlog::warn("[graphics3d.addLockableTarget] Entity system not available");
            return false;
        }
        if (!entities->isValid(entity)) {
            spdlog::warn("[graphics3d.addLockableTarget] Invalid entity");
            return false;
        }

        LockableTarget target;
        if (targetObj && targetObj->is<LockableTarget>()) {
            target = targetObj->as<LockableTarget>();
        } else if (targetObj && targetObj->is<sol::table>()) {
            sol::table tbl = targetObj->as<sol::table>();
            if (auto v = tbl["enabled"]; v.valid()) target.enabled = v.get<bool>();
            if (auto pts = tbl["lockPoints"]; pts.valid() && pts.get_type() == sol::type::table) {
                sol::table points = pts.get<sol::table>();
                for (auto& pair : points) {
                    if (pair.second.is<LockPointDef>()) {
                        target.lockPoints.push_back(pair.second.as<LockPointDef>());
                    } else if (pair.second.is<sol::table>()) {
                        sol::table pt = pair.second.as<sol::table>();
                        LockPointDef def;
                        if (auto n = pt["name"]; n.valid()) def.name = n.get<std::string>();
                        if (auto s = pt["source"]; s.valid()) {
                            if (s.get_type() == sol::type::string) {
                                std::string src = s.get<std::string>();
                                def.source = (src == "socket") ? LockPointSource::Socket : LockPointSource::Offset;
                            } else if (s.is<LockPointSource>()) {
                                def.source = s.get<LockPointSource>();
                            }
                        }
                        if (auto sn = pt["socketName"]; sn.valid()) def.socketName = sn.get<std::string>();
                        if (auto lo = pt["localOffset"]; lo.valid()) def.localOffset = lo.get<Vec3>();
                        if (auto p = pt["priority"]; p.valid()) def.priority = p.get<float>();
                        target.lockPoints.push_back(def);
                    }
                }
            }
        }

        entities->emplace<LockableTarget>(entity, target);
        return true;
    };

    gfxTable["getLockableTarget"] = [entities](Entity entity, sol::this_state s) -> sol::object {
        if (!entities) {
            spdlog::warn("[graphics3d.getLockableTarget] Entity system not available");
            return sol::nil;
        }
        auto* target = entities->tryGet<LockableTarget>(entity);
        if (target) {
            return sol::make_object(s, *target);
        }
        return sol::nil;
    };

    gfxTable["hasLockableTarget"] = [entities](Entity entity) -> bool {
        if (!entities) return false;
        return entities->tryGet<LockableTarget>(entity) != nullptr;
    };

    gfxTable["removeLockableTarget"] = [entities](Entity entity) {
        if (!entities) return;
        entities->remove<LockableTarget>(entity);
    };

    bestow["graphics3d"] = gfxTable;
}

}  // namespace bestow
