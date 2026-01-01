// bestow-luabind/src/bindings/types_binding.cpp
// Core type bindings - Vec2, Vec3, Color, Transform, etc.

module;

#include <bestow/sol2_compat.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

module bestow.luabind;

import std;

namespace bestow {

void bindTypes(sol::state& lua) {
    //=========================================================================
    // Vec2
    //=========================================================================
    lua.new_usertype<Vec2>("Vec2",
        sol::constructors<Vec2(), Vec2(float, float)>(),
        "x", &Vec2::x,
        "y", &Vec2::y,
        // Operators
        sol::meta_function::addition, [](const Vec2& a, const Vec2& b) { return a + b; },
        sol::meta_function::subtraction, [](const Vec2& a, const Vec2& b) { return a - b; },
        sol::meta_function::multiplication, sol::overload(
            [](const Vec2& v, float s) { return v * s; },
            [](float s, const Vec2& v) { return s * v; }
        ),
        sol::meta_function::division, [](const Vec2& v, float s) { return v / s; },
        sol::meta_function::unary_minus, [](const Vec2& v) { return -v; },
        sol::meta_function::equal_to, [](const Vec2& a, const Vec2& b) { return a == b; },
        sol::meta_function::to_string, [](const Vec2& v) {
            return std::format("Vec2({}, {})", v.x, v.y);
        },
        // Methods
        "length", [](const Vec2& v) { return glm::length(v); },
        "lengthSquared", [](const Vec2& v) { return glm::dot(v, v); },
        "normalize", [](const Vec2& v) { return glm::normalize(v); },
        "dot", [](const Vec2& a, const Vec2& b) { return glm::dot(a, b); }
    );

    // Static constructors
    lua["Vec2"]["zero"] = []() { return Vec2(0.0f, 0.0f); };
    lua["Vec2"]["one"] = []() { return Vec2(1.0f, 1.0f); };
    lua["Vec2"]["up"] = []() { return Vec2(0.0f, 1.0f); };
    lua["Vec2"]["down"] = []() { return Vec2(0.0f, -1.0f); };
    lua["Vec2"]["left"] = []() { return Vec2(-1.0f, 0.0f); };
    lua["Vec2"]["right"] = []() { return Vec2(1.0f, 0.0f); };

    //=========================================================================
    // Vec3
    //=========================================================================
    lua.new_usertype<Vec3>("Vec3",
        sol::constructors<Vec3(), Vec3(float, float, float)>(),
        "x", &Vec3::x,
        "y", &Vec3::y,
        "z", &Vec3::z,
        // Operators
        sol::meta_function::addition, [](const Vec3& a, const Vec3& b) { return a + b; },
        sol::meta_function::subtraction, [](const Vec3& a, const Vec3& b) { return a - b; },
        sol::meta_function::multiplication, sol::overload(
            [](const Vec3& v, float s) { return v * s; },
            [](float s, const Vec3& v) { return s * v; }
        ),
        sol::meta_function::division, [](const Vec3& v, float s) { return v / s; },
        sol::meta_function::unary_minus, [](const Vec3& v) { return -v; },
        sol::meta_function::equal_to, [](const Vec3& a, const Vec3& b) { return a == b; },
        sol::meta_function::to_string, [](const Vec3& v) {
            return std::format("Vec3({}, {}, {})", v.x, v.y, v.z);
        },
        // Methods
        "length", [](const Vec3& v) { return glm::length(v); },
        "lengthSquared", [](const Vec3& v) { return glm::dot(v, v); },
        "normalize", [](const Vec3& v) { return glm::normalize(v); },
        "dot", [](const Vec3& a, const Vec3& b) { return glm::dot(a, b); },
        "cross", [](const Vec3& a, const Vec3& b) { return glm::cross(a, b); }
    );

    // Static constructors
    lua["Vec3"]["zero"] = []() { return Vec3(0.0f, 0.0f, 0.0f); };
    lua["Vec3"]["one"] = []() { return Vec3(1.0f, 1.0f, 1.0f); };
    lua["Vec3"]["up"] = []() { return Vec3(0.0f, 1.0f, 0.0f); };
    lua["Vec3"]["down"] = []() { return Vec3(0.0f, -1.0f, 0.0f); };
    lua["Vec3"]["left"] = []() { return Vec3(-1.0f, 0.0f, 0.0f); };
    lua["Vec3"]["right"] = []() { return Vec3(1.0f, 0.0f, 0.0f); };
    lua["Vec3"]["forward"] = []() { return Vec3(0.0f, 0.0f, -1.0f); };
    lua["Vec3"]["back"] = []() { return Vec3(0.0f, 0.0f, 1.0f); };

    //=========================================================================
    // Vec4
    //=========================================================================
    lua.new_usertype<Vec4>("Vec4",
        sol::constructors<Vec4(), Vec4(float, float, float, float)>(),
        "x", &Vec4::x,
        "y", &Vec4::y,
        "z", &Vec4::z,
        "w", &Vec4::w,
        sol::meta_function::to_string, [](const Vec4& v) {
            return std::format("Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
        }
    );

    //=========================================================================
    // Quat (Quaternion)
    //=========================================================================
    lua.new_usertype<Quat>("Quat",
        sol::constructors<Quat(), Quat(float, float, float, float)>(),
        "w", &Quat::w,
        "x", &Quat::x,
        "y", &Quat::y,
        "z", &Quat::z,
        sol::meta_function::multiplication, [](const Quat& a, const Quat& b) { return a * b; },
        sol::meta_function::to_string, [](const Quat& q) {
            return std::format("Quat({}, {}, {}, {})", q.w, q.x, q.y, q.z);
        },
        "normalize", [](const Quat& q) { return glm::normalize(q); },
        "inverse", [](const Quat& q) { return glm::inverse(q); }
    );

    // Static constructors
    lua["Quat"]["identity"] = []() { return Quat(1.0f, 0.0f, 0.0f, 0.0f); };
    lua["Quat"]["fromAxisAngle"] = [](const Vec3& axis, float angle) {
        return glm::angleAxis(angle, glm::normalize(axis));
    };
    lua["Quat"]["fromEuler"] = [](float pitch, float yaw, float roll) {
        return glm::quat(Vec3(pitch, yaw, roll));
    };

    //=========================================================================
    // Color
    //=========================================================================
    lua.new_usertype<Color>("Color",
        sol::constructors<Color()>(),
        "r", &Color::r,
        "g", &Color::g,
        "b", &Color::b,
        "a", &Color::a,
        sol::meta_function::to_string, [](const Color& c) {
            return std::format("Color({}, {}, {}, {})", c.r, c.g, c.b, c.a);
        }
    );

    // Static constructors / factories
    lua["Color"]["new"] = [](int r, int g, int b, int a) {
        return Color{
            static_cast<std::uint8_t>(r),
            static_cast<std::uint8_t>(g),
            static_cast<std::uint8_t>(b),
            static_cast<std::uint8_t>(a)
        };
    };
    lua["Color"]["fromFloat"] = [](float r, float g, float b, float a) {
        return Color::fromFloat(r, g, b, a);
    };
    lua["Color"]["white"] = []() { return Color::white(); };
    lua["Color"]["black"] = []() { return Color::black(); };
    lua["Color"]["red"] = []() { return Color::red(); };
    lua["Color"]["green"] = []() { return Color::green(); };
    lua["Color"]["blue"] = []() { return Color::blue(); };
    lua["Color"]["transparent"] = []() { return Color::transparent(); };

    //=========================================================================
    // Transform2D
    //=========================================================================
    lua.new_usertype<Transform2D>("Transform2D",
        sol::constructors<Transform2D()>(),
        "x", &Transform2D::x,
        "y", &Transform2D::y,
        "rotation", &Transform2D::rotation,
        "scaleX", &Transform2D::scaleX,
        "scaleY", &Transform2D::scaleY,
        "position", &Transform2D::position,
        "scale", &Transform2D::scale,
        sol::meta_function::to_string, [](const Transform2D& t) {
            return std::format("Transform2D(x={}, y={}, rot={}, sx={}, sy={})",
                               t.x, t.y, t.rotation, t.scaleX, t.scaleY);
        }
    );

    lua["Transform2D"]["new"] = [](float x, float y, float rotation, float scaleX, float scaleY) {
        return Transform2D{x, y, rotation, scaleX, scaleY};
    };

    //=========================================================================
    // Transform3D
    //=========================================================================
    lua.new_usertype<Transform3D>("Transform3D",
        sol::constructors<Transform3D()>(),
        "position", &Transform3D::position,
        "rotation", &Transform3D::rotation,
        "scale", &Transform3D::scale,
        sol::meta_function::to_string, [](const Transform3D& t) {
            return std::format("Transform3D(pos=({},{},{}), scale=({},{},{}))",
                               t.position.x, t.position.y, t.position.z,
                               t.scale.x, t.scale.y, t.scale.z);
        }
    );

    lua["Transform3D"]["identity"] = []() { return Transform3D::identity(); };
    lua["Transform3D"]["new"] = [](const Vec3& pos, const Quat& rot, const Vec3& scale) {
        return Transform3D{pos, rot, scale};
    };

    //=========================================================================
    // AABB3D
    //=========================================================================
    lua.new_usertype<AABB3D>("AABB3D",
        sol::constructors<AABB3D()>(),
        "min", &AABB3D::min,
        "max", &AABB3D::max,
        "center", &AABB3D::center,
        "extents", &AABB3D::extents,
        "size", &AABB3D::size
    );

    lua["AABB3D"]["new"] = [](const Vec3& min, const Vec3& max) {
        return AABB3D{min, max};
    };

    //=========================================================================
    // Ray3D
    //=========================================================================
    lua.new_usertype<Ray3D>("Ray3D",
        sol::constructors<Ray3D()>(),
        "origin", &Ray3D::origin,
        "direction", &Ray3D::direction,
        "pointAt", &Ray3D::pointAt
    );

    lua["Ray3D"]["new"] = [](const Vec3& origin, const Vec3& direction) {
        return Ray3D{origin, direction};
    };

    //=========================================================================
    // Size and Coordinate
    //=========================================================================
    lua.new_usertype<Size>("Size",
        sol::constructors<Size()>(),
        "width", &Size::width,
        "height", &Size::height
    );

    lua["Size"]["new"] = [](int width, int height) {
        return Size{width, height};
    };

    lua.new_usertype<Coordinate>("Coordinate",
        sol::constructors<Coordinate()>(),
        "x", &Coordinate::x,
        "y", &Coordinate::y
    );

    lua["Coordinate"]["new"] = [](int x, int y) {
        return Coordinate{x, y};
    };

    //=========================================================================
    // Entity (opaque handle)
    //=========================================================================
    lua.new_usertype<Entity>("Entity",
        sol::no_constructor,
        sol::meta_function::equal_to, [](Entity a, Entity b) { return a == b; },
        sol::meta_function::to_string, [](Entity e) {
            return std::format("Entity({})", static_cast<std::uint32_t>(e));
        }
    );
}

}  // namespace bestow
