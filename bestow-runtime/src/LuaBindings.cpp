// bestow-runtime/src/LuaBindings.cpp
// Lua bindings implementation (PROTOTYPE)

#include "LuaBindings.hpp"
#include "ComponentRegistry.hpp"

#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

namespace bestow::runtime {

// ============================================================================
// LuaEntity Implementation
// ============================================================================

LuaEntity::LuaEntity(entt::entity entity, entt::registry* registry, sol::state* lua)
    : entity_(entity)
    , registry_(registry)
    , lua_(lua)
{
}

bool LuaEntity::isValid() const {
    return registry_ && registry_->valid(entity_);
}

void LuaEntity::destroy() {
    if (isValid()) {
        registry_->destroy(entity_);
    }
}

LuaEntity& LuaEntity::at(float x, float y) {
    if (!isValid()) return *this;

    if (registry_->all_of<Position2D>(entity_)) {
        auto& pos = registry_->get<Position2D>(entity_);
        pos.x = x;
        pos.y = y;
    } else {
        registry_->emplace<Position2D>(entity_, x, y);
    }
    return *this;
}

LuaEntity& LuaEntity::at3D(float x, float y, float z) {
    if (!isValid()) return *this;

    if (registry_->all_of<Position3D>(entity_)) {
        auto& pos = registry_->get<Position3D>(entity_);
        pos.x = x;
        pos.y = y;
        pos.z = z;
    } else {
        registry_->emplace<Position3D>(entity_, x, y, z);
    }
    return *this;
}

LuaEntity& LuaEntity::withTag(const std::string& tag) {
    if (!isValid()) return *this;

    if (registry_->all_of<TagComponent>(entity_)) {
        registry_->get<TagComponent>(entity_).addTag(tag);
    } else {
        TagComponent tc;
        tc.addTag(tag);
        registry_->emplace<TagComponent>(entity_, std::move(tc));
    }
    return *this;
}

LuaEntity& LuaEntity::withScale(float s) {
    if (!isValid()) return *this;

    if (registry_->all_of<Scale2D>(entity_)) {
        auto& scale = registry_->get<Scale2D>(entity_);
        scale.x = s;
        scale.y = s;
    } else {
        registry_->emplace<Scale2D>(entity_, s, s);
    }
    return *this;
}

LuaEntity& LuaEntity::withRotation(float degrees) {
    if (!isValid()) return *this;

    if (registry_->all_of<Rotation2D>(entity_)) {
        registry_->get<Rotation2D>(entity_).degrees = degrees;
    } else {
        registry_->emplace<Rotation2D>(entity_, degrees);
    }
    return *this;
}

LuaEntity& LuaEntity::addComponent(const std::string& name, sol::table data) {
    if (!isValid()) return *this;

    auto& compReg = ComponentRegistry::instance();
    compReg.createComponent(*registry_, entity_, name, data);
    return *this;
}

sol::table LuaEntity::get(const std::string& name) {
    if (!isValid() || !lua_) return sol::table{};

    auto& compReg = ComponentRegistry::instance();
    return compReg.getComponent(*lua_, *registry_, entity_, name);
}

bool LuaEntity::has(const std::string& name) {
    if (!isValid()) return false;

    auto& compReg = ComponentRegistry::instance();
    return compReg.hasComponent(*registry_, entity_, name);
}

void LuaEntity::remove(const std::string& name) {
    if (!isValid()) return;

    auto& compReg = ComponentRegistry::instance();
    compReg.removeComponent(*registry_, entity_, name);
}

bool LuaEntity::hasTag(const std::string& tag) {
    if (!isValid()) return false;

    if (registry_->all_of<TagComponent>(entity_)) {
        return registry_->get<TagComponent>(entity_).hasTag(tag);
    }
    return false;
}

std::vector<std::string> LuaEntity::getTags() {
    if (!isValid()) return {};

    if (registry_->all_of<TagComponent>(entity_)) {
        return registry_->get<TagComponent>(entity_).tags;
    }
    return {};
}

// ============================================================================
// LuaBindings Implementation
// ============================================================================

LuaBindings::LuaBindings(sol::state& lua)
    : lua_(lua)
    , ownedRegistry_(std::make_unique<entt::registry>())
{
    registry_ = ownedRegistry_.get();
}

void LuaBindings::registerAll() {
    // Create bestow namespace
    lua_["bestow"] = lua_.create_table();

    // Register math types first (Vec2, Vec3, etc.)
    registerMathTypes();

    // Register component types
    registerComponents();

    // Register entity bindings
    registerEntityBindings(registry_);

    spdlog::info("[LuaBindings] All bindings registered (prototype mode)");
}

void LuaBindings::registerMathTypes() {
    // Vec2
    lua_.new_usertype<glm::vec2>("Vec2",
        sol::constructors<glm::vec2(), glm::vec2(float, float)>(),
        "x", &glm::vec2::x,
        "y", &glm::vec2::y,
        sol::meta_function::addition, [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
        sol::meta_function::multiplication, sol::overload(
            [](const glm::vec2& a, float s) { return a * s; },
            [](float s, const glm::vec2& a) { return s * a; }
        ),
        "length", [](const glm::vec2& v) { return glm::length(v); },
        "normalize", [](const glm::vec2& v) {
            float len = glm::length(v);
            return len > 0.0f ? v / len : v;
        },
        "dot", [](const glm::vec2& a, const glm::vec2& b) { return glm::dot(a, b); }
    );

    // Vec3
    lua_.new_usertype<glm::vec3>("Vec3",
        sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z,
        sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
        sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
        sol::meta_function::multiplication, sol::overload(
            [](const glm::vec3& a, float s) { return a * s; },
            [](float s, const glm::vec3& a) { return s * a; }
        ),
        "length", [](const glm::vec3& v) { return glm::length(v); },
        "normalize", [](const glm::vec3& v) {
            float len = glm::length(v);
            return len > 0.0f ? v / len : v;
        },
        "dot", [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
        "cross", [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); }
    );

    spdlog::debug("[LuaBindings] Math types registered");
}

void LuaBindings::registerComponents() {
    auto& reg = ComponentRegistry::instance();

    // Register Position2D
    reg.registerComponent<Position2D>(
        "Position2D",
        [](Position2D& p, sol::table data) {
            if (data["x"].valid()) p.x = data["x"].get<float>();
            if (data["y"].valid()) p.y = data["y"].get<float>();
        },
        [](sol::state& lua, const Position2D& p) {
            sol::table t = lua.create_table();
            t["x"] = p.x;
            t["y"] = p.y;
            return t;
        }
    );

    // Register Position3D
    reg.registerComponent<Position3D>(
        "Position3D",
        [](Position3D& p, sol::table data) {
            if (data["x"].valid()) p.x = data["x"].get<float>();
            if (data["y"].valid()) p.y = data["y"].get<float>();
            if (data["z"].valid()) p.z = data["z"].get<float>();
        },
        [](sol::state& lua, const Position3D& p) {
            sol::table t = lua.create_table();
            t["x"] = p.x;
            t["y"] = p.y;
            t["z"] = p.z;
            return t;
        }
    );

    // Register Scale2D
    reg.registerComponent<Scale2D>(
        "Scale2D",
        [](Scale2D& s, sol::table data) {
            if (data["x"].valid()) s.x = data["x"].get<float>();
            if (data["y"].valid()) s.y = data["y"].get<float>();
        },
        [](sol::state& lua, const Scale2D& s) {
            sol::table t = lua.create_table();
            t["x"] = s.x;
            t["y"] = s.y;
            return t;
        }
    );

    // Register Rotation2D
    reg.registerComponent<Rotation2D>(
        "Rotation2D",
        [](Rotation2D& r, sol::table data) {
            if (data["degrees"].valid()) r.degrees = data["degrees"].get<float>();
        },
        [](sol::state& lua, const Rotation2D& r) {
            sol::table t = lua.create_table();
            t["degrees"] = r.degrees;
            return t;
        }
    );

    // Register TagComponent
    reg.registerComponent<TagComponent>(
        "Tags",
        [](TagComponent& tc, sol::table data) {
            if (data["tags"].valid()) {
                sol::table tags = data["tags"];
                tc.tags.clear();
                for (auto& [k, v] : tags) {
                    if (v.is<std::string>()) {
                        tc.tags.push_back(v.as<std::string>());
                    }
                }
            }
        },
        [](sol::state& lua, const TagComponent& tc) {
            sol::table t = lua.create_table();
            sol::table tags = lua.create_table();
            for (size_t i = 0; i < tc.tags.size(); ++i) {
                tags[i + 1] = tc.tags[i];
            }
            t["tags"] = tags;
            return t;
        }
    );

    spdlog::debug("[LuaBindings] Components registered");
}

void LuaBindings::registerEntityBindings(entt::registry* registry) {
    registry_ = registry;

    // Register LuaEntity usertype
    lua_.new_usertype<LuaEntity>("Entity",
        sol::no_constructor,

        // Core methods
        "isValid", &LuaEntity::isValid,
        "destroy", &LuaEntity::destroy,

        // Fluent builders
        "at", &LuaEntity::at,
        "at3D", &LuaEntity::at3D,
        "withTag", &LuaEntity::withTag,
        "withScale", &LuaEntity::withScale,
        "withRotation", &LuaEntity::withRotation,

        // Component access
        "addComponent", &LuaEntity::addComponent,
        "get", &LuaEntity::get,
        "has", &LuaEntity::has,
        "remove", &LuaEntity::remove,

        // Tag access
        "hasTag", &LuaEntity::hasTag,
        "getTags", &LuaEntity::getTags
    );

    spdlog::debug("[LuaBindings] Entity bindings registered");
}

}  // namespace bestow::runtime
