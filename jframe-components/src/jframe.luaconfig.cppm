// jframe-components/src/jframe.luaconfig.cppm
// Lua-based configuration loading for input and physics
// Returns data structures that can be applied to systems

module;

#include <sol/sol.hpp>

export module jframe.luaconfig;

import std;
import jframe.types;

export namespace jframe {

//==========================================================================
// PhysicsBodyConfig - Data structure for physics body configuration
//==========================================================================
// Can be loaded from Lua or constructed programmatically

struct PhysicsBodyConfig {
    BodyType type = BodyType::Dynamic;
    float width = 32.0f;
    float height = 32.0f;
    bool fixedRotation = true;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    bool isSensor = false;
    CollisionLayer layer = 0xFFFF;
    CollisionMask mask = 0xFFFF;

    // Convert to PhysicsBodyDef
    PhysicsBodyDef toBodyDef(float x, float y) const {
        return PhysicsBodyDef{
            .type = type,
            .transform = {.x = x, .y = y},
            .size = {width, height},
            .fixedRotation = fixedRotation,
            .linearDamping = linearDamping,
            .angularDamping = angularDamping,
            .density = density,
            .friction = friction,
            .restitution = restitution,
            .isSensor = isSensor
        };
    }
};

//==========================================================================
// LuaInputLoader - Load input mappings from Lua
//==========================================================================
// Example Lua file:
//
// local keys = {
//     SPACE = 32, W = 87, A = 65, S = 83, D = 68,
//     LEFT = 263, RIGHT = 262, UP = 265, DOWN = 264,
// }
// local buttons = { A = 0, B = 1, X = 2, Y = 3 }
// local axes = { LEFT_X = 0, LEFT_Y = 1 }
//
// return {
//     actions = {
//         jump = {
//             { type = "key", code = keys.SPACE },
//             { type = "button", code = buttons.A },
//         },
//         move_horizontal = {
//             { type = "key", code = keys.D, scale = 1.0 },
//             { type = "key", code = keys.A, scale = -1.0 },
//             { type = "axis", code = axes.LEFT_X },
//         },
//     },
// }
//==========================================================================

class LuaInputLoader {
public:
    // Parse Lua code and return input mappings
    static std::vector<InputMapping> parse(const std::string& luaCode) {
        std::vector<InputMapping> result;

        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

        // Sandbox
        lua["os"] = sol::nil;
        lua["io"] = sol::nil;
        lua["loadfile"] = sol::nil;
        lua["dofile"] = sol::nil;
        lua["load"] = sol::nil;

        try {
            // Use sol::script_pass_on_error to avoid throwing exceptions on syntax errors
            sol::protected_function_result execResult = lua.safe_script(luaCode, sol::script_pass_on_error);
            if (!execResult.valid()) {
                return result;
            }

            sol::table config = execResult;

            if (config["actions"].valid()) {
                sol::table actions = config["actions"];
                for (const auto& pair : actions) {
                    std::string actionName = pair.first.as<std::string>();
                    sol::table bindings = pair.second;

                    for (size_t i = 1; i <= bindings.size(); ++i) {
                        sol::table binding = bindings[i];

                        InputMapping mapping;
                        mapping.action = actionName;

                        sol::optional<std::string> type = binding["type"];
                        if (!type) continue;

                        if (*type == "key") {
                            mapping.binding.deviceType = InputDeviceType::Keyboard;
                        } else if (*type == "mouse") {
                            mapping.binding.deviceType = InputDeviceType::Mouse;
                        } else if (*type == "button" || *type == "axis") {
                            mapping.binding.deviceType = InputDeviceType::Controller;
                        }

                        sol::optional<int> code = binding["code"];
                        if (code) {
                            mapping.binding.keyCode = *code;
                            if (*type == "axis") {
                                mapping.binding.keyCode |= 0x8000;  // Flag for axis
                            }
                        }

                        sol::optional<float> scale = binding["scale"];
                        mapping.binding.scale = scale.value_or(1.0f);

                        sol::optional<float> deadzone = binding["deadzone"];
                        mapping.binding.deadzone = deadzone.value_or(0.1f);

                        sol::optional<int> controller = binding["controller"];
                        mapping.binding.deviceIndex = controller.value_or(0);

                        result.push_back(mapping);
                    }
                }
            }
        } catch (...) {
            // Return empty on any error
        }

        return result;
    }
};

//==========================================================================
// LuaPhysicsLoader - Load physics body configs from Lua
//==========================================================================
// Example Lua file:
//
// return {
//     layers = {
//         Player = 0x0001, Enemy = 0x0002, Terrain = 0x0008,
//         Ground = 0x0040, Trigger = 0x0010,
//     },
//     bodies = {
//         player = {
//             type = "dynamic",
//             width = 32, height = 48,
//             fixedRotation = true,
//             friction = 0.0,
//             layer = "Player",
//             mask = { "Terrain", "Enemy" },
//         },
//         platform = {
//             type = "static",
//             width = 200, height = 32,
//             friction = 0.5,
//             layer = { "Terrain", "Ground" },
//         },
//     },
// }
//==========================================================================

class LuaPhysicsLoader {
public:
    // Parse Lua code and return named physics body configs
    static std::unordered_map<std::string, PhysicsBodyConfig> parse(const std::string& luaCode) {
        std::unordered_map<std::string, PhysicsBodyConfig> result;

        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

        // Sandbox
        lua["os"] = sol::nil;
        lua["io"] = sol::nil;
        lua["loadfile"] = sol::nil;
        lua["dofile"] = sol::nil;
        lua["load"] = sol::nil;

        try {
            // Use sol::script_pass_on_error to avoid throwing exceptions on syntax errors
            sol::protected_function_result execResult = lua.safe_script(luaCode, sol::script_pass_on_error);
            if (!execResult.valid()) {
                return result;
            }

            sol::table config = execResult;

            // Parse layer definitions
            std::unordered_map<std::string, CollisionLayer> layerMap;
            if (config["layers"].valid()) {
                sol::table layers = config["layers"];
                for (const auto& pair : layers) {
                    std::string name = pair.first.as<std::string>();
                    int value = pair.second.as<int>();
                    layerMap[name] = static_cast<CollisionLayer>(value);
                }
            } else {
                // Default layer map (matches CollisionLayers from jframe.physics)
                layerMap = {
                    {"Player", 0x0001},
                    {"Enemy", 0x0002},
                    {"Projectile", 0x0004},
                    {"Terrain", 0x0008},
                    {"Trigger", 0x0010},
                    {"Collectible", 0x0020},
                    {"Ground", 0x0040},
                };
            }

            // Helper to parse layer/mask
            auto parseLayerMask = [&layerMap](sol::object obj) -> CollisionLayer {
                CollisionLayer mask = 0;
                if (obj.get_type() == sol::type::string) {
                    std::string name = obj.as<std::string>();
                    if (layerMap.count(name)) {
                        mask = layerMap[name];
                    }
                } else if (obj.get_type() == sol::type::table) {
                    sol::table layers = obj;
                    for (size_t i = 1; i <= layers.size(); ++i) {
                        std::string name = layers[i].get<std::string>();
                        if (layerMap.count(name)) {
                            mask |= layerMap[name];
                        }
                    }
                } else if (obj.get_type() == sol::type::number) {
                    mask = static_cast<CollisionLayer>(obj.as<int>());
                }
                return mask;
            };

            // Parse body definitions
            if (config["bodies"].valid()) {
                sol::table bodies = config["bodies"];
                for (const auto& pair : bodies) {
                    std::string bodyName = pair.first.as<std::string>();
                    sol::table bodyDef = pair.second;

                    PhysicsBodyConfig cfg;

                    sol::optional<std::string> typeStr = bodyDef["type"];
                    if (typeStr) {
                        if (*typeStr == "static") cfg.type = BodyType::Static;
                        else if (*typeStr == "kinematic") cfg.type = BodyType::Kinematic;
                        else cfg.type = BodyType::Dynamic;
                    }

                    sol::optional<float> width = bodyDef["width"];
                    if (width) cfg.width = *width;

                    sol::optional<float> height = bodyDef["height"];
                    if (height) cfg.height = *height;

                    sol::optional<bool> fixedRotation = bodyDef["fixedRotation"];
                    if (fixedRotation) cfg.fixedRotation = *fixedRotation;

                    sol::optional<float> density = bodyDef["density"];
                    if (density) cfg.density = *density;

                    sol::optional<float> friction = bodyDef["friction"];
                    if (friction) cfg.friction = *friction;

                    sol::optional<float> restitution = bodyDef["restitution"];
                    if (restitution) cfg.restitution = *restitution;

                    sol::optional<float> linearDamping = bodyDef["linearDamping"];
                    if (linearDamping) cfg.linearDamping = *linearDamping;

                    sol::optional<float> angularDamping = bodyDef["angularDamping"];
                    if (angularDamping) cfg.angularDamping = *angularDamping;

                    sol::optional<bool> sensor = bodyDef["sensor"];
                    if (sensor) cfg.isSensor = *sensor;

                    if (bodyDef["layer"].valid()) {
                        cfg.layer = parseLayerMask(bodyDef["layer"]);
                    }

                    if (bodyDef["mask"].valid()) {
                        cfg.mask = parseLayerMask(bodyDef["mask"]);
                    }

                    result[bodyName] = cfg;
                }
            }
        } catch (...) {
            // Return empty on any error
        }

        return result;
    }
};

//==========================================================================
// Convenience namespace for preset key/button codes
//==========================================================================

namespace Keys {
    // Common GLFW key codes
    inline constexpr int Space = 32;
    inline constexpr int Apostrophe = 39;
    inline constexpr int Comma = 44;
    inline constexpr int Minus = 45;
    inline constexpr int Period = 46;
    inline constexpr int Slash = 47;
    inline constexpr int Key0 = 48;
    inline constexpr int Key1 = 49;
    inline constexpr int Key2 = 50;
    inline constexpr int Key3 = 51;
    inline constexpr int Key4 = 52;
    inline constexpr int Key5 = 53;
    inline constexpr int Key6 = 54;
    inline constexpr int Key7 = 55;
    inline constexpr int Key8 = 56;
    inline constexpr int Key9 = 57;
    inline constexpr int Semicolon = 59;
    inline constexpr int Equal = 61;
    inline constexpr int A = 65;
    inline constexpr int B = 66;
    inline constexpr int C = 67;
    inline constexpr int D = 68;
    inline constexpr int E = 69;
    inline constexpr int F = 70;
    inline constexpr int G = 71;
    inline constexpr int H = 72;
    inline constexpr int I = 73;
    inline constexpr int J = 74;
    inline constexpr int K = 75;
    inline constexpr int L = 76;
    inline constexpr int M = 77;
    inline constexpr int N = 78;
    inline constexpr int O = 79;
    inline constexpr int P = 80;
    inline constexpr int Q = 81;
    inline constexpr int R = 82;
    inline constexpr int S = 83;
    inline constexpr int T = 84;
    inline constexpr int U = 85;
    inline constexpr int V = 86;
    inline constexpr int W = 87;
    inline constexpr int X = 88;
    inline constexpr int Y = 89;
    inline constexpr int Z = 90;
    inline constexpr int Escape = 256;
    inline constexpr int Enter = 257;
    inline constexpr int Tab = 258;
    inline constexpr int Backspace = 259;
    inline constexpr int Insert = 260;
    inline constexpr int Delete = 261;
    inline constexpr int Right = 262;
    inline constexpr int Left = 263;
    inline constexpr int Down = 264;
    inline constexpr int Up = 265;
    inline constexpr int PageUp = 266;
    inline constexpr int PageDown = 267;
    inline constexpr int Home = 268;
    inline constexpr int End = 269;
    inline constexpr int LeftShift = 340;
    inline constexpr int LeftControl = 341;
    inline constexpr int LeftAlt = 342;
    inline constexpr int RightShift = 344;
    inline constexpr int RightControl = 345;
    inline constexpr int RightAlt = 346;
}

namespace ControllerButtons {
    // SDL controller buttons
    inline constexpr int A = 0;
    inline constexpr int B = 1;
    inline constexpr int X = 2;
    inline constexpr int Y = 3;
    inline constexpr int Back = 4;
    inline constexpr int Guide = 5;
    inline constexpr int Start = 6;
    inline constexpr int LeftStick = 7;
    inline constexpr int RightStick = 8;
    inline constexpr int LeftShoulder = 9;
    inline constexpr int RightShoulder = 10;
    inline constexpr int DPadUp = 11;
    inline constexpr int DPadDown = 12;
    inline constexpr int DPadLeft = 13;
    inline constexpr int DPadRight = 14;
}

namespace ControllerAxes {
    // SDL controller axes
    inline constexpr int LeftX = 0;
    inline constexpr int LeftY = 1;
    inline constexpr int RightX = 2;
    inline constexpr int RightY = 3;
    inline constexpr int TriggerLeft = 4;
    inline constexpr int TriggerRight = 5;
}

}  // namespace jframe
