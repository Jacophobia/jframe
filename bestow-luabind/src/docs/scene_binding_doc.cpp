// bestow-luabind/src/docs/scene_binding_doc.cpp
// API documentation for bestow.scene

module bestow.luabind;

import std;

namespace bestow {

void registerSceneDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "scene";
    sys.qualifiedName = "bestow.scene";
    sys.description = "Scene stack management system. Scenes are registered from asset files and managed via a stack -- push to enter a scene (pausing the previous), pop to return, or replace to swap the active scene.";

    // --- Methods ---

    sys.methods.push_back(MethodDoc{
        .name = "register",
        .qualifiedName = "bestow.scene.register",
        .description = "Register a scene by name from an asset path. The asset is loaded via the AssetSystem.",
        .params = {
            {.name = "name", .type = "string", .description = "Unique name for the scene (e.g., 'MainMenu', 'Gameplay')"},
            {.name = "assetPath", .type = "string", .description = "Path to the scene asset file"},
        },
        .returns = {{.type = "boolean", .description = "true if registered successfully"}},
        .example = "bestow.scene.register(\"MainMenu\", \"scenes/main_menu.lua\")\nbestow.scene.register(\"Gameplay\", \"scenes/gameplay.lua\")",
        .seeAlso = {"bestow.scene.push", "bestow.scene.registered"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "push",
        .qualifiedName = "bestow.scene.push",
        .description = "Push a scene onto the stack. The current scene is paused and the new scene becomes active.",
        .params = {
            {.name = "name", .type = "string", .description = "Name of the registered scene to push"},
            {.name = "params", .type = "table", .description = "Optional parameters to pass to the scene (string, number, or boolean values)", .optional = true},
        },
        .returns = {{.type = "boolean", .description = "true if pushed successfully"}},
        .example = "bestow.scene.push(\"PauseMenu\")\nbestow.scene.push(\"Gameplay\", { level = 3, difficulty = \"hard\" })",
        .seeAlso = {"bestow.scene.pop", "bestow.scene.replace"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "pop",
        .qualifiedName = "bestow.scene.pop",
        .description = "Pop the active scene from the stack. The previous scene resumes.",
        .returns = {{.type = "boolean", .description = "true if popped successfully (false if stack is empty)"}},
        .seeAlso = {"bestow.scene.push"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "replace",
        .qualifiedName = "bestow.scene.replace",
        .description = "Replace the active (top) scene with a different scene. The replaced scene is removed from the stack.",
        .params = {
            {.name = "name", .type = "string", .description = "Name of the registered scene to switch to"},
            {.name = "params", .type = "table", .description = "Optional parameters to pass to the scene", .optional = true},
        },
        .returns = {{.type = "boolean", .description = "true if replaced successfully"}},
        .example = "bestow.scene.replace(\"GameOver\", { score = 42000 })",
        .seeAlso = {"bestow.scene.push", "bestow.scene.pop"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "clear",
        .qualifiedName = "bestow.scene.clear",
        .description = "Clear the entire scene stack. All scenes are unloaded.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "active",
        .qualifiedName = "bestow.scene.active",
        .description = "Get the name of the currently active (top of stack) scene.",
        .returns = {{.type = "string|nil", .description = "Name of the active scene, or nil if the stack is empty"}},
        .example = "local current = bestow.scene.active()\nif current == \"Gameplay\" then\n    -- in gameplay\nend",
    });

    sys.methods.push_back(MethodDoc{
        .name = "stack",
        .qualifiedName = "bestow.scene.stack",
        .description = "Get the full scene stack as a table, ordered from bottom to top.",
        .returns = {{.type = "string[]", .description = "Array of scene names (index 1 = bottom, last = top/active)"}},
        .example = "local scenes = bestow.scene.stack()\nfor i, name in ipairs(scenes) do\n    print(i, name)\nend",
    });

    sys.methods.push_back(MethodDoc{
        .name = "state",
        .qualifiedName = "bestow.scene.state",
        .description = "Get the current state of a scene as a string.",
        .params = {
            {.name = "name", .type = "string", .description = "Name of the scene to query"},
        },
        .returns = {{.type = "string", .description = "Scene state: 'unloaded', 'loading', 'ready', 'active', 'paused', or 'unloading'"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "registered",
        .qualifiedName = "bestow.scene.registered",
        .description = "Get a list of all registered scene names.",
        .returns = {{.type = "string[]", .description = "Array of registered scene names"}},
        .seeAlso = {"bestow.scene.register"},
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
