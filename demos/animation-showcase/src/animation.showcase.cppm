// demos/animation-showcase/src/animation.showcase.cppm
// Animation System Showcase Application
//
// Demonstrates:
// - Loading FBX models with skeletal animation data
// - Creating skeletons and animation clips from model data
// - Animation State Machine with smooth blending transitions
// - Auto-play sequence: idle → walk → jog → run → jump → falling → landing → recovery → walk
// - Rendering animated meshes with Vulkan

module;

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

export module animation.showcase;

import std;
import bestow.services;
import bestow.types;
import bestow.lua;
import bestow.graphics3d;
import bestow.animation;
import bestow.animation.statemachine;

export namespace showcase {

//==========================================================================
// Animation State IDs
//==========================================================================

namespace AnimState {
    constexpr bestow::AnimStateId Idle          = 1;
    constexpr bestow::AnimStateId Walk          = 2;
    constexpr bestow::AnimStateId Jog           = 3;
    constexpr bestow::AnimStateId Run           = 4;
    constexpr bestow::AnimStateId Jump          = 5;
    constexpr bestow::AnimStateId Falling       = 6;
    constexpr bestow::AnimStateId Landing       = 7;
    constexpr bestow::AnimStateId LandingRecovery = 8;
}

//==========================================================================
// Auto-Play Sequence Configuration
//==========================================================================

struct SequenceStep {
    bestow::AnimStateId state;
    float duration;        // Time to stay in this state (0 = wait for animation end)
    float targetSpeed;     // Speed parameter to set
    bool isGrounded;       // IsGrounded parameter
    bool triggerJump = false;  // Whether to trigger a jump
};

//==========================================================================
// Animation Showcase Application
//==========================================================================

class AnimationShowcase : public bestow::Application<AnimationShowcase,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem,
    bestow::IAssetSystem,
    bestow::IAnimationSystem,
    bestow::ILuaRuntime>
{
public:
    AnimationShowcase(bestow::IGraphics3DSystem& graphics,
                      bestow::IInputSystem& input,
                      bestow::IAssetSystem& assets,
                      bestow::IAnimationSystem& animation,
                      bestow::ILuaRuntime& lua)
        : graphics_(&graphics)
        , input_(&input)
        , assets_(&assets)
        , animation_(&animation)
        , lua_(&lua) {}

    ~AnimationShowcase() override = default;

    void run() override {
        if (!initialize()) {
            return;
        }

        gameLoop();
        cleanup();
    }

    void shutdown() override {
        running_ = false;
    }

private:
    //======================================================================
    // Dependencies
    //======================================================================
    bestow::IGraphics3DSystem* graphics_ = nullptr;
    bestow::IInputSystem* input_ = nullptr;
    bestow::IAssetSystem* assets_ = nullptr;
    bestow::IAnimationSystem* animation_ = nullptr;
    bestow::ILuaRuntime* lua_ = nullptr;

    //======================================================================
    // Animation Data
    //======================================================================
    bestow::AssetHandle modelHandle_{};
    bestow::SkeletonHandle skeletonHandle_ = 0;
    bestow::AnimatorHandle animatorHandle_ = 0;
    std::vector<bestow::AnimationClipHandle> clipHandles_;
    std::size_t currentClipIndex_ = 0;

    // State Machine
    std::unique_ptr<bestow::IAnimationStateMachine> stateMachine_;
    std::unordered_map<std::string, bestow::AnimationClipHandle> clipsByName_;

    //======================================================================
    // Rendering
    //======================================================================
    bestow::MeshHandle characterMesh_ = 0;
    bestow::MaterialHandle characterMaterial_ = 0;
    bestow::MeshHandle groundMesh_ = 0;
    bestow::MaterialHandle groundMaterial_ = 0;

    //======================================================================
    // State
    //======================================================================
    bool running_ = false;
    bool modelLoaded_ = false;
    bool showMesh_ = true;           // Toggle with M key
    float cameraAngle_ = 0.0f;
    float cameraDistance_ = 300.0f;  // Very zoomed out for Mixamo models (which are in cm scale)
    float cameraHeight_ = 100.0f;    // Higher to see full character
    float cameraTargetY_ = 100.0f;   // Look-at target height (adjustable with Up/Down)

    // Auto-play sequence state (loaded from Lua)
    std::vector<SequenceStep> demoSequence_;
    std::size_t sequenceIndex_ = 0;
    float sequenceTimer_ = 0.0f;
    bool autoPlayEnabled_ = true;    // Toggle with P key
    bool waitingForAnimEnd_ = false;
    bool sequenceLoops_ = true;      // Whether sequence loops back to start

    // Hot reload file watching
    struct WatchedFile {
        std::string path;
        std::filesystem::file_time_type lastModified;
    };
    std::vector<WatchedFile> watchedConfigs_;
    float hotReloadCheckTimer_ = 0.0f;
    static constexpr float kHotReloadCheckInterval = 0.25f;  // Check 4x per second

    // Root motion - character world position driven by animation
    bestow::Vec3 characterPosition_{0.0f, 0.0f, 0.0f};
    bool useRootMotion_ = true;      // Toggle with R key

    //======================================================================
    // Initialization
    //======================================================================

    bool initialize() {
        if (!graphics_ || !lua_) {
            std::cerr << "Graphics or Lua system not available\n";
            return false;
        }

        // Load app configuration from Lua
        auto appResult = lua_->loadApp("data/app.lua");
        if (!appResult) {
            std::cerr << "Failed to load app.lua: " << appResult.error().message << "\n";
            return false;
        }

        // Load animation config files as globals (for hot-reload access)
        loadAnimationConfigs();

        // Initialize graphics with Lua config
        bestow::Graphics3DConfig gfxConfig{
            .windowWidth = static_cast<uint32_t>(lua_->getIntOr("window.width", 1280)),
            .windowHeight = static_cast<uint32_t>(lua_->getIntOr("window.height", 720)),
            .windowTitle = lua_->getStringOr("window.title", "Animation Showcase"),
            .vsync = lua_->getBoolOr("window.vsync", true),
            .fullscreen = lua_->getBoolOr("window.fullscreen", false)
        };

        if (!graphics_->initialize(gfxConfig)) {
            std::cerr << "Failed to initialize graphics\n";
            return false;
        }

        // Set clear color from Lua config
        auto clearColor = lua_->getTable("graphics.clearColor");
        if (clearColor.size() >= 4) {
            graphics_->setClearColor(bestow::Color{
                static_cast<uint8_t>(std::get<int64_t>(clearColor[0])),
                static_cast<uint8_t>(std::get<int64_t>(clearColor[1])),
                static_cast<uint8_t>(std::get<int64_t>(clearColor[2])),
                static_cast<uint8_t>(std::get<int64_t>(clearColor[3]))
            });
        } else {
            graphics_->setClearColor(bestow::Color{30, 35, 45, 255});
        }

        // Initialize input
        if (input_) {
            input_->initialize(graphics_->getNativeWindowHandle());
        }

        // Initialize animation system
        if (animation_) {
            animation_->initialize();
        }

        // Create ground mesh from Lua config
        float groundSize = lua_->getFloatOr("ground.size", 500.0f);
        auto planeResult = graphics_->createPlaneMesh(groundSize, groundSize, 10, 10);
        if (planeResult) {
            groundMesh_ = *planeResult;
        }

        // Create ground material
        groundMaterial_ = graphics_->getDefaultUnlitMaterial();

        // Load camera settings from Lua
        cameraDistance_ = lua_->getFloatOr("camera.distance", 300.0f);
        cameraHeight_ = lua_->getFloatOr("camera.height", 100.0f);
        cameraTargetY_ = lua_->getFloatOr("camera.targetY", 100.0f);
        cameraAngle_ = lua_->getFloatOr("camera.angle", 0.0f);

        // Load debug settings from Lua
        showMesh_ = !lua_->getBoolOr("debug.showBones", true);  // Inverse: if showing bones, less focus on mesh

        // Load the character model (creates mesh and material from FBX data)
        loadCharacterModel();

        // Setup camera
        setupCamera();

        // Setup lighting from Lua config
        auto lightDir = lua_->getTable("graphics.directionalLight.direction");
        auto lightColor = lua_->getTable("graphics.directionalLight.color");
        float lightIntensity = lua_->getFloatOr("graphics.directionalLight.intensity", 8.0f);

        bestow::DirectionalLight light{
            .direction = {
                lightDir.size() >= 3 ? static_cast<float>(std::get<double>(lightDir[0])) : 0.5f,
                lightDir.size() >= 3 ? static_cast<float>(std::get<double>(lightDir[1])) : -0.8f,
                lightDir.size() >= 3 ? static_cast<float>(std::get<double>(lightDir[2])) : 0.3f
            },
            .color = {
                lightColor.size() >= 3 ? static_cast<float>(std::get<double>(lightColor[0])) : 1.0f,
                lightColor.size() >= 3 ? static_cast<float>(std::get<double>(lightColor[1])) : 0.98f,
                lightColor.size() >= 3 ? static_cast<float>(std::get<double>(lightColor[2])) : 0.9f
            },
            .intensity = lightIntensity,
            .castShadows = true
        };
        graphics_->setDirectionalLight(light);

        auto ambientColor = lua_->getTable("graphics.ambientLight.color");
        float ambientIntensity = lua_->getFloatOr("graphics.ambientLight.intensity", 0.6f);
        graphics_->setAmbientLight(
            {
                ambientColor.size() >= 3 ? static_cast<float>(std::get<double>(ambientColor[0])) : 0.8f,
                ambientColor.size() >= 3 ? static_cast<float>(std::get<double>(ambientColor[1])) : 0.85f,
                ambientColor.size() >= 3 ? static_cast<float>(std::get<double>(ambientColor[2])) : 0.9f
            },
            ambientIntensity
        );

        return true;
    }

    void loadCharacterModel() {
        // Load the test character FBX using path from Lua config
        std::string characterPath = lua_->getStringOr("character.model", ":library:/characters/test-character.fbx");

        std::cout << "Loading character from: " << characterPath << std::endl;

        // Load the model using the asset system
        modelHandle_ = assets_->loadModel(characterPath);

        if (!assets_->isLoaded(modelHandle_)) {
            std::cerr << "Failed to load character model\n";
            return;
        }

        const bestow::ModelData* modelData = assets_->getModelData(modelHandle_);
        if (!modelData) {
            std::cerr << "Failed to get model data\n";
            return;
        }

        std::cout << "Model loaded: " << modelData->name << std::endl;
        std::cout << "  Meshes: " << modelData->meshes.size() << std::endl;
        std::cout << "  Bones: " << modelData->bones.size() << std::endl;

        // Create skeleton from model
        auto skeletonResult = animation_->createSkeleton(*modelData);
        if (!skeletonResult) {
            std::cerr << "Failed to create skeleton\n";
            return;
        }
        skeletonHandle_ = *skeletonResult;

        std::cout << "Skeleton created with " << animation_->getBoneCount(skeletonHandle_) << " bones\n";

        // Load all animation clips for the state machine demo
        loadAnimationClips();

        // Create animator
        auto animatorResult = animation_->createAnimator(skeletonHandle_);
        if (!animatorResult) {
            std::cerr << "Failed to create animator\n";
            return;
        }
        animatorHandle_ = *animatorResult;

        // Enable root motion - extract XZ translation for locomotion
        // NOTE: Y extraction is disabled because these animations aren't designed
        // for Y root motion. The falling/landing animations have the hips move low
        // as part of the crouching pose, not as actual vertical displacement.
        // For Y motion to work, you'd need animations authored with root motion in mind.
        bestow::RootMotionConfig rootConfig;
        rootConfig.enabled = true;
        rootConfig.extractTranslationX = true;
        rootConfig.extractTranslationY = false;  // Disabled - animations not authored for Y root motion
        rootConfig.extractTranslationZ = true;
        rootConfig.extractRotationY = false;
        animation_->setRootMotionConfig(animatorHandle_, rootConfig);
        std::cout << "Root motion enabled (XZ only)\n";

        // Create the animation state machine
        createStateMachine();

        // Create materials from model data
        if (!modelData->materials.empty()) {
            auto materialsResult = graphics_->createMaterialsFromModel(*modelData);
            if (materialsResult && !materialsResult->empty()) {
                characterMaterial_ = (*materialsResult)[0];
                std::cout << "Created material from model\n";
            } else {
                characterMaterial_ = graphics_->getDefaultPBRMaterial();
            }
        } else {
            characterMaterial_ = graphics_->getDefaultPBRMaterial();
        }

        // Create mesh from model data
        if (!modelData->meshes.empty()) {
            auto meshResult = graphics_->createMeshFromData(modelData->meshes[0]);
            if (meshResult) {
                characterMesh_ = *meshResult;
                std::cout << "Created character mesh with " << modelData->meshes[0].vertices.size()
                          << " vertices\n";
            }
        }

        modelLoaded_ = true;

        // Start the auto-play sequence
        if (autoPlayEnabled_ && stateMachine_) {
            startSequenceStep(0);
        }
    }

    void loadAnimationConfigs() {
        // Load config files and store as Lua globals for hot-reload access
        auto& lua = lua_->getLuaState();

        // Load animations config
        std::ifstream animFile("data/config/animations.lua");
        if (animFile) {
            std::stringstream buffer;
            buffer << animFile.rdbuf();
            auto result = lua.safe_script(buffer.str(), sol::script_pass_on_error);
            if (result.valid()) {
                lua["animations"] = result.get<sol::table>();
                std::cout << "[Config] Loaded animations.lua\n";
            } else {
                sol::error err = result;
                std::cerr << "[Config] Failed to load animations.lua: " << err.what() << "\n";
            }
        }

        // Load state machine config
        std::ifstream smFile("data/config/state-machine.lua");
        if (smFile) {
            std::stringstream buffer;
            buffer << smFile.rdbuf();
            auto result = lua.safe_script(buffer.str(), sol::script_pass_on_error);
            if (result.valid()) {
                lua["stateMachine"] = result.get<sol::table>();
                std::cout << "[Config] Loaded state-machine.lua\n";
            } else {
                sol::error err = result;
                std::cerr << "[Config] Failed to load state-machine.lua: " << err.what() << "\n";
            }
        }

        // Load demo sequence config
        std::ifstream seqFile("data/config/demo-sequence.lua");
        if (seqFile) {
            std::stringstream buffer;
            buffer << seqFile.rdbuf();
            auto result = lua.safe_script(buffer.str(), sol::script_pass_on_error);
            if (result.valid()) {
                lua["demoSequence"] = result.get<sol::table>();
                std::cout << "[Config] Loaded demo-sequence.lua\n";

                // Parse sequence into steps
                loadDemoSequence();
            } else {
                sol::error err = result;
                std::cerr << "[Config] Failed to load demo-sequence.lua: " << err.what() << "\n";
            }
        }

        // Register files for hot reload watching
        registerWatchedFiles();
    }

    void registerWatchedFiles() {
        watchedConfigs_.clear();

        const std::vector<std::string> configPaths = {
            "data/config/animations.lua",
            "data/config/state-machine.lua",
            "data/config/demo-sequence.lua"
        };

        for (const auto& path : configPaths) {
            if (std::filesystem::exists(path)) {
                watchedConfigs_.push_back({
                    .path = path,
                    .lastModified = std::filesystem::last_write_time(path)
                });
            }
        }

        std::cout << "[HotReload] Watching " << watchedConfigs_.size() << " config files\n";
    }

    bool checkForConfigChanges() {
        bool anyChanged = false;

        for (auto& watched : watchedConfigs_) {
            if (!std::filesystem::exists(watched.path)) continue;

            auto currentTime = std::filesystem::last_write_time(watched.path);
            if (currentTime != watched.lastModified) {
                std::cout << "[HotReload] Detected change: " << watched.path << "\n";
                watched.lastModified = currentTime;
                anyChanged = true;
            }
        }

        return anyChanged;
    }

    void hotReloadConfigs() {
        std::cout << "\n=== HOT RELOAD ===\n";
        loadAnimationConfigs();
        // Rebuild state machine with new blend times (preserves current state)
        if (stateMachine_) {
            auto currentState = stateMachine_->getState();
            createStateMachine();
            // Restore current animation state if possible
            if (stateMachine_) {
                stateMachine_->setFloat("Speed", currentState.parameters.contains("Speed") ?
                    std::get<float>(currentState.parameters.at("Speed")) : 0.0f);
            }
        }
        std::cout << "=== RELOAD COMPLETE ===\n\n";
    }

    void loadDemoSequence() {
        // Load demo sequence from Lua config
        auto& lua = lua_->getLuaState();
        sol::table seqConfig = lua["demoSequence"];
        if (!seqConfig.valid()) {
            std::cerr << "No 'demoSequence' config found, using empty sequence\n";
            return;
        }

        // Get sequence settings
        sequenceLoops_ = seqConfig.get_or("loop", true);
        autoPlayEnabled_ = seqConfig.get_or("enabled", true);

        // Parse steps
        demoSequence_.clear();
        sol::table steps = seqConfig["steps"];
        if (!steps.valid()) {
            std::cerr << "No 'steps' in demoSequence config\n";
            return;
        }

        for (auto& pair : steps) {
            if (pair.second.get_type() != sol::type::table) continue;

            sol::table step = pair.second.as<sol::table>();
            SequenceStep s;
            s.state = static_cast<bestow::AnimStateId>(step.get_or("state", 0));
            s.duration = step.get_or("duration", 1.0f);
            s.targetSpeed = step.get_or("speed", 0.0f);
            s.isGrounded = step.get_or("grounded", true);
            s.triggerJump = step.get_or("jump", false);
            demoSequence_.push_back(s);
        }

        std::cout << "[Config] Loaded " << demoSequence_.size() << " demo sequence steps\n";
    }

    void loadAnimationClips() {
        // Load animation clip paths from Lua config
        auto& lua = lua_->getLuaState();
        sol::table animConfig = lua["animations"];
        if (!animConfig.valid()) {
            std::cerr << "No 'animations' config found, using defaults\n";
            return;
        }

        sol::table clips = animConfig["clips"];
        if (!clips.valid()) {
            std::cerr << "No 'clips' table in animations config\n";
            return;
        }

        std::cout << "\nLoading animation clips from Lua config:\n";

        // Iterate through clip definitions from Lua
        for (auto& pair : clips) {
            std::string clipName = pair.first.as<std::string>();
            std::string clipPath = pair.second.as<std::string>();

            auto animHandle = assets_->loadModel(clipPath);
            if (!assets_->isLoaded(animHandle)) {
                std::cerr << "  Failed to load: " << clipPath << "\n";
                continue;
            }

            const bestow::ModelData* animData = assets_->getModelData(animHandle);
            if (!animData || animData->animations.empty()) {
                std::cerr << "  No animations in: " << clipPath << "\n";
                continue;
            }

            // Create clips from this animation file
            auto animClips = animation_->createAnimationClips(skeletonHandle_, *animData);
            if (!animClips.empty()) {
                clipsByName_[clipName] = animClips[0];  // Use first clip
                clipHandles_.push_back(animClips[0]);
                std::cout << "  Loaded: " << clipName << " from " << clipPath << " (handle: " << animClips[0] << ")\n";
            }
        }

        std::cout << "Loaded " << clipsByName_.size() << " animation clips\n\n";
    }

    void createStateMachine() {
        using namespace bestow;

        // Load state config from Lua for blend times (hot-reloadable)
        auto& lua = lua_->getLuaState();
        sol::table smConfig = lua["stateMachine"];
        sol::table states = smConfig.valid() ? smConfig["states"] : sol::table{};

        // Helper to get blend time from Lua config or use default
        auto getBlendTime = [&states](int stateId, float defaultVal) -> float {
            if (!states.valid()) return defaultVal;
            sol::optional<sol::table> state = states[stateId];
            if (state) {
                return state->get_or("blendTime", defaultVal);
            }
            return defaultVal;
        };

        // Build the state machine definition using Lua-configured blend times
        auto builder = AnimationStateMachineBuilder("CharacterLocomotion")
            // Add states with blend times from Lua config
            .addState(AnimState::Idle, "idle", "idle",
                      AnimationWrapMode::Loop, getBlendTime(0, 0.8f))
            .addState(AnimState::Walk, "walk", "walk",
                      AnimationWrapMode::Loop, getBlendTime(1, 0.8f))
            .addState(AnimState::Jog, "jog", "jog",
                      AnimationWrapMode::Loop, getBlendTime(2, 0.7f))
            .addState(AnimState::Run, "run", "run",
                      AnimationWrapMode::Loop, getBlendTime(3, 0.6f))
            .addState(AnimState::Jump, "jump", "jump",
                      AnimationWrapMode::Once, getBlendTime(4, 0.35f))
            .addState(AnimState::Falling, "falling", "falling",
                      AnimationWrapMode::Loop, getBlendTime(5, 0.4f))
            .addState(AnimState::Landing, "landing", "landing",
                      AnimationWrapMode::Once, getBlendTime(6, 0.25f))
            .addState(AnimState::LandingRecovery, "landing-recovery", "landing-recovery",
                      AnimationWrapMode::Once, getBlendTime(7, 0.5f))

            // Set default state
            .setDefaultState(AnimState::Idle)

            // Add parameters
            .addParameter("Speed", AnimParamType::Float, 0.0f)
            .addParameter("IsGrounded", AnimParamType::Bool, true)
            .addParameter("Jump", AnimParamType::Trigger)

            // Locomotion transitions (grounded, speed-based)
            .addTransition(AnimState::Idle, AnimState::Walk,
                {paramGreater("Speed", 0.1f), paramEquals("IsGrounded", true)})
            .addTransition(AnimState::Walk, AnimState::Idle,
                {paramLess("Speed", 0.1f), paramEquals("IsGrounded", true)})
            .addTransition(AnimState::Walk, AnimState::Jog,
                {paramGreater("Speed", 0.4f), paramEquals("IsGrounded", true)})
            .addTransition(AnimState::Jog, AnimState::Walk,
                {paramLess("Speed", 0.4f), paramEquals("IsGrounded", true)})
            .addTransition(AnimState::Jog, AnimState::Run,
                {paramGreater("Speed", 0.8f), paramEquals("IsGrounded", true)})
            .addTransition(AnimState::Run, AnimState::Jog,
                {paramLess("Speed", 0.8f), paramEquals("IsGrounded", true)})

            // Jump transitions
            .addTransition(AnimState::Idle, AnimState::Jump,
                {onTrigger("Jump")}, 0.35f, 10)
            .addTransition(AnimState::Walk, AnimState::Jump,
                {onTrigger("Jump")}, 0.35f, 10)
            .addTransition(AnimState::Jog, AnimState::Jump,
                {onTrigger("Jump")}, 0.35f, 10)
            .addTransition(AnimState::Run, AnimState::Jump,
                {onTrigger("Jump")}, 0.35f, 10)

            // Jump -> Landing/Falling
            .addTransition(AnimState::Jump, AnimState::Landing,
                {onAnimationEnd(), paramEquals("IsGrounded", true)}, 0.25f)
            .addTransition(AnimState::Jump, AnimState::Falling,
                {onAnimationEnd(), paramEquals("IsGrounded", false)}, 0.15f)

            // Falling -> Landing
            .addTransition(AnimState::Falling, AnimState::Landing,
                {paramEquals("IsGrounded", true)}, 0.1f)

            // Landing -> Recovery -> Locomotion
            .addTransition(AnimState::Landing, AnimState::LandingRecovery,
                {onAnimationEnd()})
            .addTransition(AnimState::LandingRecovery, AnimState::Walk,
                {onAnimationEnd(), paramGreater("Speed", 0.1f)})
            .addTransition(AnimState::LandingRecovery, AnimState::Idle,
                {onAnimationEnd(), paramLess("Speed", 0.1f)});

        auto definition = builder.build();

        // Resolve clip names to clip handles
        for (auto& state : definition.states) {
            auto it = clipsByName_.find(state.clipName);
            if (it != clipsByName_.end()) {
                state.clip = it->second;
            } else {
                std::cerr << "Warning: No clip found for state '" << state.name << "'\n";
            }
        }

        // Create the state machine
        stateMachine_ = createAnimationStateMachine(animation_, animatorHandle_, std::move(definition));

        if (stateMachine_) {
            std::cout << "Animation state machine created!\n";

            // Set up callbacks for debugging
            stateMachine_->setOnStateEnter([](AnimStateId state, AnimStateId from) {
                std::cout << "[StateMachine] Entered state " << state << " (from " << from << ")\n";
            });

            stateMachine_->setOnTransition([](AnimStateId from, AnimStateId to, float blend) {
                std::cout << "[StateMachine] Transitioning " << from << " -> " << to
                          << " (blend: " << blend << "s)\n";
            });
        }
    }

    //======================================================================
    // Game Loop
    //======================================================================

    void gameLoop() {
        constexpr bestow::DeltaTime fixedDt = 1.0f / 60.0f;
        auto previousTime = std::chrono::high_resolution_clock::now();
        bestow::DeltaTime accumulator = 0.0f;

        running_ = true;

        while (running_ && !graphics_->shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            bestow::DeltaTime frameTime =
                std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;

            if (frameTime > 0.25f) {
                frameTime = 0.25f;
            }
            accumulator += frameTime;

            // Update input
            if (input_) {
                input_->update();
            }

            // Hot reload check (throttled to avoid excessive file system access)
            hotReloadCheckTimer_ += frameTime;
            if (hotReloadCheckTimer_ >= kHotReloadCheckInterval) {
                hotReloadCheckTimer_ = 0.0f;
                if (checkForConfigChanges()) {
                    hotReloadConfigs();
                }
            }

            // Fixed timestep updates
            while (accumulator >= fixedDt) {
                handleInput();

                // Update auto-play sequence
                if (autoPlayEnabled_ && stateMachine_) {
                    updateAutoPlay(fixedDt);
                }

                // Update state machine (handles transitions, parameter evaluation)
                if (stateMachine_) {
                    stateMachine_->update(fixedDt);
                }

                // Update animation system
                if (animation_) {
                    animation_->update(fixedDt);

                    // Apply root motion to character position
                    if (useRootMotion_ && animatorHandle_) {
                        auto rootMotion = animation_->getRootMotion(animatorHandle_);
                        if (rootMotion.hasTranslation) {
                            characterPosition_ += rootMotion.deltaPosition;

                            // Debug: Log character position periodically
                            static int logCounter = 0;
                            if (logCounter++ % 60 == 0) {
                                std::cout << std::format("[Demo] CharPos: ({:.3f}, {:.3f}, {:.3f}), Delta: ({:.4f}, {:.4f}, {:.4f})\n",
                                    characterPosition_.x, characterPosition_.y, characterPosition_.z,
                                    rootMotion.deltaPosition.x, rootMotion.deltaPosition.y, rootMotion.deltaPosition.z);
                            }
                        }
                        animation_->consumeRootMotion(animatorHandle_);
                    }
                }

                updateCamera(fixedDt);
                accumulator -= fixedDt;
            }

            // Render
            render();
        }
    }

    void cleanup() {
        // Clean up animation resources
        if (animation_ && animatorHandle_) {
            animation_->destroyAnimator(animatorHandle_);
        }
        for (auto clip : clipHandles_) {
            if (animation_) {
                animation_->destroyAnimationClip(clip);
            }
        }
        if (animation_ && skeletonHandle_) {
            animation_->destroySkeleton(skeletonHandle_);
        }

        if (animation_) {
            animation_->shutdown();
        }

        if (input_) {
            input_->shutdown();
        }
        if (graphics_) {
            graphics_->shutdown();
        }
    }

    //======================================================================
    // Auto-Play Sequence
    //======================================================================

    void startSequenceStep(std::size_t index) {
        if (index >= demoSequence_.size() || !stateMachine_) return;

        sequenceIndex_ = index;
        sequenceTimer_ = 0.0f;

        const auto& step = demoSequence_[index];

        // Set parameters for this step
        stateMachine_->setFloat("Speed", step.targetSpeed);
        stateMachine_->setBool("IsGrounded", step.isGrounded);

        // Trigger jump if requested
        if (step.triggerJump) {
            stateMachine_->setTrigger("Jump");
        }

        // Determine if we should wait for animation end (duration == 0 means wait)
        waitingForAnimEnd_ = (step.duration <= 0.0f);

        std::cout << "[AutoPlay] Step " << index << ": "
                  << "State=" << step.state
                  << ", Speed=" << step.targetSpeed
                  << ", Grounded=" << step.isGrounded
                  << (step.triggerJump ? " (jumping)" : "")
                  << (waitingForAnimEnd_ ? " (wait for anim end)" : "")
                  << "\n";
    }

    void updateAutoPlay(float dt) {
        if (demoSequence_.empty() || sequenceIndex_ >= demoSequence_.size()) return;

        const auto& step = demoSequence_[sequenceIndex_];

        if (waitingForAnimEnd_) {
            // Check if we've entered the target state and animation completed
            auto machineState = stateMachine_->getState();

            // Wait until we're in the target state and animation is complete
            if (machineState.currentState == step.state &&
                machineState.animationComplete &&
                !machineState.inTransition) {
                advanceSequence();
            }
        } else {
            // Timer-based progression
            sequenceTimer_ += dt;
            if (sequenceTimer_ >= step.duration) {
                advanceSequence();
            }
        }
    }

    void advanceSequence() {
        std::size_t nextIndex = sequenceIndex_ + 1;

        // Loop back to beginning when we reach the end (if looping is enabled)
        if (nextIndex >= demoSequence_.size()) {
            if (sequenceLoops_) {
                std::cout << "[AutoPlay] Sequence complete! Looping back to start.\n";
                nextIndex = 0;
            } else {
                std::cout << "[AutoPlay] Sequence complete!\n";
                autoPlayEnabled_ = false;
                return;
            }
        }

        startSequenceStep(nextIndex);
    }

    //======================================================================
    // Input Handling
    //======================================================================

    void handleInput() {
        if (!input_) return;

        // ESC to quit
        if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            running_ = false;
        }

        // P to toggle auto-play
        if (input_->wasKeyJustPressed(GLFW_KEY_P)) {
            autoPlayEnabled_ = !autoPlayEnabled_;
            std::cout << "Auto-play " << (autoPlayEnabled_ ? "enabled" : "disabled") << "\n";
            if (autoPlayEnabled_ && stateMachine_) {
                // Restart the sequence
                startSequenceStep(0);
            }
        }

        // Camera rotation with A/E (Dvorak) or Left/Right
        if (input_->isKeyDown(GLFW_KEY_A) || input_->isKeyDown(GLFW_KEY_LEFT)) {
            cameraAngle_ += 2.0f;
        }
        if (input_->isKeyDown(GLFW_KEY_E) || input_->isKeyDown(GLFW_KEY_RIGHT)) {
            cameraAngle_ -= 2.0f;
        }

        // Camera zoom with ,/O (Dvorak W/S) or +/-
        float zoomRate = cameraDistance_ * 0.02f;
        if (input_->isKeyDown(GLFW_KEY_COMMA) || input_->isKeyDown(GLFW_KEY_EQUAL)) {
            cameraDistance_ = std::max(50.0f, cameraDistance_ - zoomRate);
        }
        if (input_->isKeyDown(GLFW_KEY_O) || input_->isKeyDown(GLFW_KEY_MINUS)) {
            cameraDistance_ = std::min(1000.0f, cameraDistance_ + zoomRate);
        }

        // Camera vertical pan with Up/Down arrows
        constexpr float panRate = 5.0f;
        if (input_->isKeyDown(GLFW_KEY_UP)) {
            cameraTargetY_ += panRate;
            cameraHeight_ += panRate;  // Move camera up too to keep relative angle
        }
        if (input_->isKeyDown(GLFW_KEY_DOWN)) {
            cameraTargetY_ -= panRate;
            cameraHeight_ -= panRate;
        }

        // Toggle mesh visibility with M
        if (input_->wasKeyJustPressed(GLFW_KEY_M)) {
            showMesh_ = !showMesh_;
            std::cout << (showMesh_ ? "Showing" : "Hiding") << " mesh\n";
        }

        // Manual force-reload with L (automatic reload also happens on file change)
        if (input_->wasKeyJustPressed(GLFW_KEY_L)) {
            hotReloadConfigs();
        }

        // Manual state control with number keys (when auto-play disabled)
        if (!autoPlayEnabled_ && stateMachine_) {
            if (input_->wasKeyJustPressed(GLFW_KEY_1)) {
                stateMachine_->setFloat("Speed", 0.0f);
                std::cout << "Speed: 0.0 (idle)\n";
            }
            if (input_->wasKeyJustPressed(GLFW_KEY_2)) {
                stateMachine_->setFloat("Speed", 0.3f);
                std::cout << "Speed: 0.3 (walk)\n";
            }
            if (input_->wasKeyJustPressed(GLFW_KEY_3)) {
                stateMachine_->setFloat("Speed", 0.6f);
                std::cout << "Speed: 0.6 (jog)\n";
            }
            if (input_->wasKeyJustPressed(GLFW_KEY_4)) {
                stateMachine_->setFloat("Speed", 1.0f);
                std::cout << "Speed: 1.0 (run)\n";
            }
            if (input_->wasKeyJustPressed(GLFW_KEY_SPACE)) {
                stateMachine_->setTrigger("Jump");
                stateMachine_->setBool("IsGrounded", false);
                std::cout << "Jump triggered!\n";
            }
            if (input_->wasKeyJustPressed(GLFW_KEY_G)) {
                bool grounded = !stateMachine_->getBool("IsGrounded");
                stateMachine_->setBool("IsGrounded", grounded);
                std::cout << "IsGrounded: " << (grounded ? "true" : "false") << "\n";
            }
        }
    }

    //======================================================================
    // Camera
    //======================================================================

    void setupCamera() {
        bestow::Camera3D cam;
        // Load camera projection from Lua config
        cam.fovY = lua_->getFloatOr("camera.fovY", 45.0f);
        cam.nearPlane = lua_->getFloatOr("camera.nearPlane", 1.0f);
        cam.farPlane = lua_->getFloatOr("camera.farPlane", 1000.0f);

        updateCameraTransform(cam);
        graphics_->setCamera(cam);
    }

    void updateCamera(float dt) {
        bestow::Camera3D cam = graphics_->getCamera();
        updateCameraTransform(cam);
        graphics_->setCamera(cam);
    }

    void updateCameraTransform(bestow::Camera3D& cam) {
        float angleRad = glm::radians(cameraAngle_);
        bestow::Vec3 cameraPos = {
            cameraDistance_ * std::sin(angleRad),
            cameraHeight_,
            cameraDistance_ * std::cos(angleRad)
        };

        cam.transform.position = cameraPos;

        // Look at character center (adjustable with Up/Down arrows)
        glm::vec3 target(0.0f, cameraTargetY_, 0.0f);
        glm::vec3 camPosGlm(cameraPos.x, cameraPos.y, cameraPos.z);
        glm::vec3 lookDir = glm::normalize(target - camPosGlm);
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::quat rotation = glm::quatLookAt(lookDir, up);
        cam.transform.rotation = {rotation.w, rotation.x, rotation.y, rotation.z};
    }

    //======================================================================
    // Rendering
    //======================================================================

    void render() {
        graphics_->beginFrame();

        // Draw character
        if (modelLoaded_ && animatorHandle_) {
            // Get bone transforms from animation system
            auto boneTransforms = animation_->getBoneTransforms(animatorHandle_);
            // Apply character position from root motion
            bestow::Mat4 characterMatrix = glm::translate(glm::identity<glm::mat4>(),
                glm::vec3(characterPosition_.x, characterPosition_.y, characterPosition_.z));

            // Draw the mesh if enabled
            if (showMesh_ && characterMesh_) {
                graphics_->drawSkinnedMesh(characterMesh_, characterMaterial_, characterMatrix, boneTransforms);
            }

            // Draw debug visualization of bones (shows animation working)
            drawBoneVisualization(characterMatrix);
        }

        // Draw UI text
        drawUI();

        graphics_->endFrame();
    }

    void drawBoneVisualization(const bestow::Mat4& worldMatrix) {
        if (!animation_ || !skeletonHandle_) {
            return;
        }

        // Use model-space poses for visualization (NOT skinning matrices)
        auto modelPoses = animation_->getModelSpaceBonePoses(animatorHandle_);

        static int frameCount = 0;
        if (++frameCount % 60 == 0 && !modelPoses.empty()) {
            const auto& m = modelPoses[0];
            std::cout << "Bone 0 model pos: " << m[3][0] << ", " << m[3][1] << ", " << m[3][2] << std::endl;
        }

        // Draw a sphere at each bone position (model-space transformed to world)
        for (std::size_t i = 0; i < modelPoses.size() && i < 100; ++i) {
            const bestow::Mat4& boneMatrix = modelPoses[i];
            bestow::Mat4 finalMatrix = worldMatrix * boneMatrix;

            // Extract position from matrix
            bestow::Vec3 bonePos{
                finalMatrix[3][0],
                finalMatrix[3][1],
                finalMatrix[3][2]
            };

            // Draw debug sphere at the bone position (BIG and RED for visibility)
            graphics_->debugDrawSphere(bonePos, 15.0f, bestow::Color{255, 0, 0, 255});
        }
    }

    void drawUI() {
        // Display state machine info periodically
        static int frameCount = 0;
        if (++frameCount % 120 == 0 && stateMachine_) {
            auto state = stateMachine_->getState();
            auto stateName = stateMachine_->getCurrentStateName();

            std::cout << "[State] " << stateName
                      << " | Speed=" << stateMachine_->getFloat("Speed")
                      << " | Grounded=" << (stateMachine_->getBool("IsGrounded") ? "Y" : "N")
                      << " | Time=" << state.stateTime
                      << (state.inTransition ? " [transitioning]" : "")
                      << "\n";
        }
    }
};

}  // namespace showcase
