// bestow-luabind/src/docs/character_binding_doc.cpp
// API documentation for Character (high-level animated character API)

module bestow.luabind;

import std;

namespace bestow {

void registerCharacterDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "character";
    sys.qualifiedName = "bestow.character";
    sys.description = "High-level animated character API. Provides a user-friendly wrapper around animation, rendering, and event systems for managing game characters.";

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "Character",
        .qualifiedName = "Character",
        .description = "A high-level animated character that manages its own skeleton, animator, mesh, and animation events. Created via bestow.animation.loadCharacter(config).",
        .fields = {},
        .methods = {
            // Playback
            {.name = "play", .qualifiedName = "Character:play",
             .description = "Play a named animation with optional blend time.",
             .params = {
                 {.name = "animName", .type = "string", .description = "Animation name (e.g., 'idle', 'walk', 'attack')"},
                 {.name = "blendTime", .type = "number", .description = "Crossfade blend time in seconds", .optional = true, .defaultVal = "defaultBlendTime"},
             },
             .example = "character:play(\"walk\", 0.3)"},
            {.name = "playOnce", .qualifiedName = "Character:playOnce",
             .description = "Play an animation once, then perform an action: play another animation or call a method. Supports hot-reload-safe callbacks.",
             .params = {
                 {.name = "animName", .type = "string", .description = "Animation to play once"},
                 {.name = "thenArg1", .type = "string|table", .description = "Next animation name, or callback table (self)"},
                 {.name = "thenArg2", .type = "string", .description = "Method name on callback table (if table passed)", .optional = true},
                 {.name = "blendTime", .type = "number", .description = "Crossfade blend time", .optional = true},
             },
             .example = "-- Play attack, then return to idle\ncharacter:playOnce(\"attack\", \"idle\")\n\n-- Play attack, then call self:onAttackDone()\ncharacter:playOnce(\"attack\", self, \"onAttackDone\")"},
            {.name = "stop", .qualifiedName = "Character:stop",
             .description = "Stop all animation.",
             .params = {{.name = "fadeTime", .type = "number", .description = "Fade-out time", .optional = true, .defaultVal = "0"}}},
            {.name = "pause", .qualifiedName = "Character:pause",
             .description = "Pause animation playback."},
            {.name = "resume", .qualifiedName = "Character:resume",
             .description = "Resume paused animation playback."},
            {.name = "setSpeed", .qualifiedName = "Character:setSpeed",
             .description = "Set the animation playback speed.",
             .params = {{.name = "speed", .type = "number", .description = "Speed multiplier (1.0 = normal)"}}},
            {.name = "getSpeed", .qualifiedName = "Character:getSpeed",
             .description = "Get the animation playback speed.",
             .returns = {{.type = "number", .description = "Speed multiplier"}}},
            {.name = "setDefaultBlendTime", .qualifiedName = "Character:setDefaultBlendTime",
             .description = "Set the default crossfade blend time for animation transitions.",
             .params = {{.name = "time", .type = "number", .description = "Default blend time in seconds"}}},

            // Transform
            {.name = "setPosition", .qualifiedName = "Character:setPosition",
             .description = "Set the character position. Accepts (x, y, z) or a Vec3.",
             .params = {
                 {.name = "x", .type = "number|Vec3", .description = "X position or Vec3"},
                 {.name = "y", .type = "number", .description = "Y position (if using x,y,z)", .optional = true},
                 {.name = "z", .type = "number", .description = "Z position (if using x,y,z)", .optional = true},
             }},
            {.name = "getPosition", .qualifiedName = "Character:getPosition",
             .description = "Get the character position.",
             .returns = {{.type = "Vec3", .description = "Current position"}}},
            {.name = "setRotation", .qualifiedName = "Character:setRotation",
             .description = "Set the character Y-axis rotation.",
             .params = {{.name = "yaw", .type = "number", .description = "Rotation around Y axis in radians"}}},
            {.name = "getRotation", .qualifiedName = "Character:getRotation",
             .description = "Get the character Y-axis rotation.",
             .returns = {{.type = "number", .description = "Rotation in radians"}}},
            {.name = "setScale", .qualifiedName = "Character:setScale",
             .description = "Set the uniform character scale.",
             .params = {{.name = "scale", .type = "number", .description = "Uniform scale factor"}}},
            {.name = "getScale", .qualifiedName = "Character:getScale",
             .description = "Get the character scale.",
             .returns = {{.type = "number", .description = "Scale factor"}}},

            // Rendering
            {.name = "draw", .qualifiedName = "Character:draw",
             .description = "Draw the character at its current position, rotation, and scale. Can also be called as draw(x, y, z, rotation) to set position and rotation before drawing.",
             .params = {
                 {.name = "x", .type = "number", .description = "X position", .optional = true},
                 {.name = "y", .type = "number", .description = "Y position", .optional = true},
                 {.name = "z", .type = "number", .description = "Z position", .optional = true},
                 {.name = "rotation", .type = "number", .description = "Y rotation in radians", .optional = true},
             }},

            // Queries
            {.name = "isPlaying", .qualifiedName = "Character:isPlaying",
             .description = "Check if the character is currently playing (optionally a specific animation).",
             .params = {{.name = "animName", .type = "string", .description = "Specific animation to check", .optional = true}},
             .returns = {{.type = "boolean", .description = "true if playing"}}},
            {.name = "getCurrentAnimation", .qualifiedName = "Character:getCurrentAnimation",
             .description = "Get the name of the currently playing animation.",
             .returns = {{.type = "string", .description = "Current animation name (empty if none)"}}},
            {.name = "getAnimationProgress", .qualifiedName = "Character:getAnimationProgress",
             .description = "Get the normalized progress (0-1) of the current animation.",
             .returns = {{.type = "number", .description = "Progress (0-1)"}}},
            {.name = "getAnimationTime", .qualifiedName = "Character:getAnimationTime",
             .description = "Get the current playback time in seconds.",
             .returns = {{.type = "number", .description = "Time in seconds"}}},
            {.name = "getAnimationDuration", .qualifiedName = "Character:getAnimationDuration",
             .description = "Get the duration of the current animation.",
             .returns = {{.type = "number", .description = "Duration in seconds"}}},
            {.name = "getAvailableAnimations", .qualifiedName = "Character:getAvailableAnimations",
             .description = "Get a list of all loaded animation names.",
             .returns = {{.type = "string[]", .description = "Array of animation names"}}},
            {.name = "hasAnimation", .qualifiedName = "Character:hasAnimation",
             .description = "Check if the character has a specific animation loaded.",
             .params = {{.name = "name", .type = "string", .description = "Animation name to check"}},
             .returns = {{.type = "boolean", .description = "true if animation exists"}}},

            // Events
            {.name = "addEvent", .qualifiedName = "Character:addEvent",
             .description = "Add an event at a normalized time in an animation.",
             .params = {
                 {.name = "animName", .type = "string", .description = "Animation to add the event to"},
                 {.name = "time", .type = "number", .description = "Normalized time (0-1) when the event fires"},
                 {.name = "eventName", .type = "string", .description = "Event name"},
                 {.name = "data", .type = "table", .description = "Custom data table", .optional = true},
             }},
            {.name = "addEventWithDuration", .qualifiedName = "Character:addEventWithDuration",
             .description = "Add an event with a duration window at a normalized time.",
             .params = {
                 {.name = "animName", .type = "string", .description = "Animation to add the event to"},
                 {.name = "time", .type = "number", .description = "Normalized time (0-1) when the event fires"},
                 {.name = "eventName", .type = "string", .description = "Event name"},
                 {.name = "duration", .type = "number", .description = "Event active duration in seconds"},
                 {.name = "data", .type = "table", .description = "Custom data table", .optional = true},
             }},
            {.name = "onEvent", .qualifiedName = "Character:onEvent",
             .description = "Subscribe to all animation events (hot-reload safe). The method receives an event table with fields: name, animation, time, data.",
             .params = {
                 {.name = "table", .type = "table", .description = "Table containing the callback method (e.g., self)"},
                 {.name = "methodName", .type = "string", .description = "Method name to call on the table"},
             }},
            {.name = "on", .qualifiedName = "Character:on",
             .description = "Subscribe to a specific named event (hot-reload safe).",
             .params = {
                 {.name = "eventName", .type = "string", .description = "Event name to listen for"},
                 {.name = "table", .type = "table", .description = "Table containing the callback method"},
                 {.name = "methodName", .type = "string", .description = "Method name to call"},
             },
             .example = "character:on(\"footstep\", self, \"onFootstep\")"},
            {.name = "isEventActive", .qualifiedName = "Character:isEventActive",
             .description = "Check if a duration-based event is currently active.",
             .params = {{.name = "eventName", .type = "string", .description = "Event name to check"}},
             .returns = {{.type = "boolean", .description = "true if event is currently active"}}},
            {.name = "getActiveEvents", .qualifiedName = "Character:getActiveEvents",
             .description = "Get all currently active duration-based events.",
             .returns = {{.type = "table[]", .description = "Array of {name, remainingTime} tables"}}},

            // Lifecycle
            {.name = "update", .qualifiedName = "Character:update",
             .description = "Update the character. Call each frame to process animation events and duration tracking.",
             .params = {{.name = "dt", .type = "number", .description = "Delta time in seconds"}}},
            {.name = "destroy", .qualifiedName = "Character:destroy",
             .description = "Destroy the character and release all resources."},

            // Debug
            {.name = "showSkeleton", .qualifiedName = "Character:showSkeleton",
             .description = "Toggle skeleton debug visualization.",
             .params = {{.name = "show", .type = "boolean", .description = "true to show skeleton"}}},
            {.name = "showBounds", .qualifiedName = "Character:showBounds",
             .description = "Toggle bounding box debug visualization.",
             .params = {{.name = "show", .type = "boolean", .description = "true to show bounds"}}},
        },
        .example = "local character = bestow.animation.loadCharacter({\n    model = \":library:/characters/hero.fbx\",\n    animations = {\n        idle = \":library:/animations/idle.fbx\",\n        walk = \":library:/animations/walk.fbx\",\n        attack = {\n            path = \":library:/animations/attack.fbx\",\n            events = {\n                { time = 0.4, name = \"damage\", damage = 50 },\n                { time = 0.2, name = \"footstep\", foot = \"left\" },\n            }\n        }\n    }\n})\n\ncharacter:play(\"idle\")\ncharacter:setPosition(0, 0, 0)",
    });

    // --- Factory Method ---

    sys.methods.push_back(MethodDoc{
        .name = "loadCharacter",
        .qualifiedName = "bestow.animation.loadCharacter",
        .description = "Create a Character from a configuration table. The config should contain: model (string, required), animations (table mapping names to paths or {path, events} tables, optional), animationDir (string, auto-discover animations from directory, optional).",
        .params = {
            {.name = "config", .type = "table", .description = "Character configuration table"},
        },
        .returns = {{.type = "Character|nil", .description = "Character instance, or nil on failure"}},
        .example = "local hero = bestow.animation.loadCharacter({\n    model = \":library:/characters/hero.fbx\",\n    animationDir = \"animations/hero\"\n})",
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
