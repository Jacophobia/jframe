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
};

// The demo auto-play sequence with full animation flow
// Sequence loops back to idle after landing recovery
inline const std::vector<SequenceStep> kDemoSequence = {
    {AnimState::Idle,           2.0f, 0.0f, true},
    {AnimState::Walk,           2.0f, 0.3f, true},
    {AnimState::Jog,            2.0f, 0.6f, true},
    {AnimState::Run,            2.0f, 1.0f, true},
    {AnimState::Jump,           0.0f, 1.0f, false},  // Wait for animation end, not grounded during jump
    {AnimState::Falling,        0.06f, 0.0f, false}, // Very brief fall
    {AnimState::Landing,        0.0f, 0.0f, true},   // Land (wait for animation end), now grounded
    {AnimState::LandingRecovery, 0.0f, 0.0f, true},  // Recovery (wait for animation end)
    // Sequence will loop back to Idle
};

//==========================================================================
// Animation Showcase Application
//==========================================================================

class AnimationShowcase : public bestow::Application<AnimationShowcase,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem,
    bestow::IAssetSystem,
    bestow::IAnimationSystem>
{
public:
    AnimationShowcase(bestow::IGraphics3DSystem& graphics,
                      bestow::IInputSystem& input,
                      bestow::IAssetSystem& assets,
                      bestow::IAnimationSystem& animation)
        : graphics_(&graphics)
        , input_(&input)
        , assets_(&assets)
        , animation_(&animation) {}

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

    // Auto-play sequence state
    std::size_t sequenceIndex_ = 0;
    float sequenceTimer_ = 0.0f;
    bool autoPlayEnabled_ = true;    // Toggle with P key
    bool waitingForAnimEnd_ = false;

    // Root motion - character world position driven by animation
    bestow::Vec3 characterPosition_{0.0f, 0.0f, 0.0f};
    bool useRootMotion_ = true;      // Toggle with R key

    //======================================================================
    // Initialization
    //======================================================================

    bool initialize() {
        if (!graphics_) {
            std::cerr << "Graphics system not available\n";
            return false;
        }

        // Initialize graphics
        bestow::Graphics3DConfig gfxConfig{
            .windowWidth = 1280,
            .windowHeight = 720,
            .windowTitle = "Animation Showcase - Bestow Demo",
            .vsync = true,
            .fullscreen = false
        };

        if (!graphics_->initialize(gfxConfig)) {
            std::cerr << "Failed to initialize graphics\n";
            return false;
        }

        graphics_->setClearColor(bestow::Color{30, 35, 45, 255});

        // Initialize input
        if (input_) {
            input_->initialize(graphics_->getNativeWindowHandle());
        }

        // Initialize animation system
        if (animation_) {
            animation_->initialize();
        }

        // Create ground mesh (large for Mixamo character scale)
        auto planeResult = graphics_->createPlaneMesh(500.0f, 500.0f, 10, 10);
        if (planeResult) {
            groundMesh_ = *planeResult;
        }

        // Create ground material
        groundMaterial_ = graphics_->getDefaultUnlitMaterial();
        // Note: characterMaterial_ will be created from model data in loadCharacterModel()

        // Load the character model (creates mesh and material from FBX data)
        loadCharacterModel();

        // Setup camera
        setupCamera();

        // Setup natural lighting (sun-like directional light)
        bestow::DirectionalLight light{
            .direction = {0.5f, -0.8f, 0.3f},  // Sun angle from upper-left
            .color = {1.0f, 0.95f, 0.85f},     // Warm sunlight
            .intensity = 5.0f,
            .castShadows = true
        };
        graphics_->setDirectionalLight(light);
        graphics_->setAmbientLight({0.6f, 0.7f, 0.8f}, 0.4f);  // Brighter ambient fill

        return true;
    }

    void loadCharacterModel() {
        // Load the test character FBX using :library:/ path resolution
        std::string characterPath = ":library:/characters/test-character.fbx";

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

    void loadAnimationClips() {
        // Animation files to load (mapped to state names)
        struct AnimFile {
            std::string name;
            std::string path;
        };

        std::vector<AnimFile> animFiles = {
            {"idle",             ":library:/animations/idle.fbx"},
            {"walk",             ":library:/animations/walk.fbx"},
            {"jog",              ":library:/animations/jog.fbx"},
            {"run",              ":library:/animations/run.fbx"},
            {"jump",             ":library:/animations/jumping-up.fbx"},
            {"falling",          ":library:/animations/falling.fbx"},
            {"landing",          ":library:/animations/landing.fbx"},
            {"landing-recovery", ":library:/animations/landing-recovery.fbx"},
        };

        std::cout << "\nLoading animation clips:\n";

        for (const auto& animFile : animFiles) {
            auto animHandle = assets_->loadModel(animFile.path);
            if (!assets_->isLoaded(animHandle)) {
                std::cerr << "  Failed to load: " << animFile.path << "\n";
                continue;
            }

            const bestow::ModelData* animData = assets_->getModelData(animHandle);
            if (!animData || animData->animations.empty()) {
                std::cerr << "  No animations in: " << animFile.path << "\n";
                continue;
            }

            // Create clips from this animation file
            auto clips = animation_->createAnimationClips(skeletonHandle_, *animData);
            if (!clips.empty()) {
                clipsByName_[animFile.name] = clips[0];  // Use first clip
                clipHandles_.push_back(clips[0]);
                std::cout << "  Loaded: " << animFile.name << " (clip handle: " << clips[0] << ")\n";
            }
        }

        std::cout << "Loaded " << clipsByName_.size() << " animation clips\n\n";
    }

    void createStateMachine() {
        using namespace bestow;

        // Build the state machine definition
        // Note: Blend times increased to 0.5s for more visible transitions during testing
        auto builder = AnimationStateMachineBuilder("CharacterLocomotion")
            // Add states (last param is default blend time INTO this state)
            // Locomotion states - longer blend times for smooth transitions
            .addState(AnimState::Idle, "idle", "idle",
                      AnimationWrapMode::Loop, 0.8f)
            .addState(AnimState::Walk, "walk", "walk",
                      AnimationWrapMode::Loop, 0.8f)
            .addState(AnimState::Jog, "jog", "jog",
                      AnimationWrapMode::Loop, 0.7f)
            .addState(AnimState::Run, "run", "run",
                      AnimationWrapMode::Loop, 0.6f)
            // Action states
            .addState(AnimState::Jump, "jump", "jump",
                      AnimationWrapMode::Once, 0.35f)
            .addState(AnimState::Falling, "falling", "falling",
                      AnimationWrapMode::Loop, 0.4f)
            .addState(AnimState::Landing, "landing", "landing",
                      AnimationWrapMode::Once, 0.25f)
            .addState(AnimState::LandingRecovery, "landing-recovery", "landing-recovery",
                      AnimationWrapMode::Once, 0.5f)

            // Set default state
            .setDefaultState(AnimState::Idle)

            // Add parameters
            .addParameter("Speed", AnimParamType::Float, 0.0f)
            .addParameter("IsGrounded", AnimParamType::Bool, true)
            .addParameter("Jump", AnimParamType::Trigger)

            // Locomotion transitions (grounded, speed-based)
            // idle -> walk when Speed > 0.1
            .addTransition(AnimState::Idle, AnimState::Walk,
                {paramGreater("Speed", 0.1f), paramEquals("IsGrounded", true)})
            // walk -> idle when Speed < 0.1
            .addTransition(AnimState::Walk, AnimState::Idle,
                {paramLess("Speed", 0.1f), paramEquals("IsGrounded", true)})
            // walk -> jog when Speed > 0.4
            .addTransition(AnimState::Walk, AnimState::Jog,
                {paramGreater("Speed", 0.4f), paramEquals("IsGrounded", true)})
            // jog -> walk when Speed < 0.4
            .addTransition(AnimState::Jog, AnimState::Walk,
                {paramLess("Speed", 0.4f), paramEquals("IsGrounded", true)})
            // jog -> run when Speed > 0.8
            .addTransition(AnimState::Jog, AnimState::Run,
                {paramGreater("Speed", 0.8f), paramEquals("IsGrounded", true)})
            // run -> jog when Speed < 0.8
            .addTransition(AnimState::Run, AnimState::Jog,
                {paramLess("Speed", 0.8f), paramEquals("IsGrounded", true)})

            // Jump transition (from any grounded state)
            .addTransition(AnimState::Idle, AnimState::Jump,
                {onTrigger("Jump")}, 0.35f, 10)  // High priority
            .addTransition(AnimState::Walk, AnimState::Jump,
                {onTrigger("Jump")}, 0.35f, 10)
            .addTransition(AnimState::Jog, AnimState::Jump,
                {onTrigger("Jump")}, 0.35f, 10)
            .addTransition(AnimState::Run, AnimState::Jump,
                {onTrigger("Jump")}, 0.35f, 10)

            // Jump -> Landing directly when grounded (skip falling for smoother flow)
            .addTransition(AnimState::Jump, AnimState::Landing,
                {onAnimationEnd(), paramEquals("IsGrounded", true)}, 0.25f)

            // Jump -> Falling when jump ends and still in air
            .addTransition(AnimState::Jump, AnimState::Falling,
                {onAnimationEnd(), paramEquals("IsGrounded", false)}, 0.15f)

            // Falling -> Landing when grounded
            .addTransition(AnimState::Falling, AnimState::Landing,
                {paramEquals("IsGrounded", true)}, 0.1f)

            // Landing -> LandingRecovery when landing animation ends
            .addTransition(AnimState::Landing, AnimState::LandingRecovery,
                {onAnimationEnd()})

            // LandingRecovery -> appropriate locomotion state when animation ends
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
        if (index >= kDemoSequence.size() || !stateMachine_) return;

        sequenceIndex_ = index;
        sequenceTimer_ = 0.0f;

        const auto& step = kDemoSequence[index];

        // Set parameters for this step
        stateMachine_->setFloat("Speed", step.targetSpeed);
        stateMachine_->setBool("IsGrounded", step.isGrounded);

        // If this is the jump state, trigger the jump
        if (step.state == AnimState::Jump) {
            stateMachine_->setTrigger("Jump");
        }

        // Determine if we should wait for animation end
        waitingForAnimEnd_ = (step.duration <= 0.0f);

        std::cout << "[AutoPlay] Step " << index << ": "
                  << "State=" << step.state
                  << ", Speed=" << step.targetSpeed
                  << ", Grounded=" << step.isGrounded
                  << (waitingForAnimEnd_ ? " (wait for anim end)" : "")
                  << "\n";
    }

    void updateAutoPlay(float dt) {
        if (sequenceIndex_ >= kDemoSequence.size()) return;

        const auto& step = kDemoSequence[sequenceIndex_];

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

        // Loop back to beginning when we reach the end
        if (nextIndex >= kDemoSequence.size()) {
            std::cout << "[AutoPlay] Sequence complete! Looping back to start.\n";
            nextIndex = 0;
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
        cam.fovY = 45.0f;
        cam.nearPlane = 1.0f;
        cam.farPlane = 1000.0f;  // Far enough for zoomed out view

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
