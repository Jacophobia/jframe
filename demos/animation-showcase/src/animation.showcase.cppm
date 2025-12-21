// demos/animation-showcase/src/animation.showcase.cppm
// Animation System Showcase Application
//
// Demonstrates:
// - Loading FBX models with skeletal animation data
// - Creating skeletons and animation clips from model data
// - Playing animations with the animation system
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

export namespace showcase {

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

        // Setup lighting
        bestow::DirectionalLight light{
            .direction = {0.3f, -1.0f, 0.2f},
            .color = {1.0f, 0.98f, 0.95f},
            .intensity = 1.0f,
            .castShadows = true
        };
        graphics_->setDirectionalLight(light);
        graphics_->setAmbientLight({0.3f, 0.35f, 0.4f}, 0.5f);

        return true;
    }

    void loadCharacterModel() {
        // Load the test character FBX using :library:/ path resolution
        // (library path is set to asset-library in main.cpp)
        std::string characterPath = ":library:/characters/test-character.fbx";
        std::string animationPath = ":library:/animations/Idle-looking-around.fbx";

        std::cout << "Loading character from: " << characterPath << std::endl;

        // Load the model using the asset system (will resolve :library:/ path)
        modelHandle_ = assets_->loadModel(characterPath);

        // Check if the model loaded
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
        std::cout << "  Animations: " << modelData->animations.size() << std::endl;

        // Create skeleton from model
        auto skeletonResult = animation_->createSkeleton(*modelData);
        if (!skeletonResult) {
            std::cerr << "Failed to create skeleton\n";
            return;
        }
        skeletonHandle_ = *skeletonResult;

        std::cout << "Skeleton created with " << animation_->getBoneCount(skeletonHandle_) << " bones\n";

        // Create animation clips
        clipHandles_ = animation_->createAnimationClips(skeletonHandle_, *modelData);
        std::cout << "Created " << clipHandles_.size() << " animation clips\n";

        // Also try to load the separate idle animation
        auto animHandle = assets_->loadModel(animationPath);
        if (assets_->isLoaded(animHandle)) {
            const bestow::ModelData* animData = assets_->getModelData(animHandle);
            if (animData && !animData->animations.empty()) {
                auto additionalClips = animation_->createAnimationClips(skeletonHandle_, *animData);
                for (auto clip : additionalClips) {
                    clipHandles_.push_back(clip);
                }
                std::cout << "Loaded " << additionalClips.size() << " additional animation clips from idle animation\n";
            }
        }

        // Create animator
        auto animatorResult = animation_->createAnimator(skeletonHandle_);
        if (!animatorResult) {
            std::cerr << "Failed to create animator\n";
            return;
        }
        animatorHandle_ = *animatorResult;

        // Play the best animation (prefer additional loaded animations over embedded ones)
        if (!clipHandles_.empty()) {
            // Play the last clip (usually the additional loaded animation is better)
            std::size_t clipToPlay = clipHandles_.size() - 1;
            animation_->play(animatorHandle_, clipHandles_[clipToPlay]);
            currentClipIndex_ = clipToPlay;
            std::cout << "Playing animation clip " << clipToPlay << "\n";
        }

        // Create materials from model data (includes embedded textures)
        if (!modelData->materials.empty()) {
            auto materialsResult = graphics_->createMaterialsFromModel(*modelData);
            if (materialsResult && !materialsResult->empty()) {
                characterMaterial_ = (*materialsResult)[0];  // Use first material
                std::cout << "Created material from model with "
                          << modelData->materials.size() << " materials\n";
                if (!modelData->materials[0].baseColorTexture.embeddedData.empty()) {
                    std::cout << "  Material has embedded texture: "
                              << modelData->materials[0].baseColorTexture.path << "\n";
                }
            } else {
                characterMaterial_ = graphics_->getDefaultPBRMaterial();
                std::cout << "Using default PBR material (no model materials created)\n";
            }
        } else {
            characterMaterial_ = graphics_->getDefaultPBRMaterial();
            std::cout << "Using default PBR material (no materials in model)\n";
        }

        // Create mesh from the model data (use first mesh if available)
        if (!modelData->meshes.empty()) {
            // Check if mesh has bone data
            const auto& meshData = modelData->meshes[0];
            int weightedVertices = 0;
            for (const auto& v : meshData.vertices) {
                float totalWeight = v.boneWeights[0] + v.boneWeights[1] + v.boneWeights[2] + v.boneWeights[3];
                if (totalWeight > 0.0f) weightedVertices++;
            }
            std::cout << "Mesh bone data: " << weightedVertices << "/" << meshData.vertices.size()
                      << " vertices have bone weights\n";
            if (!meshData.vertices.empty()) {
                const auto& v0 = meshData.vertices[0];
                std::cout << "  First vertex: bones=[" << (int)v0.boneIndices[0] << "," << (int)v0.boneIndices[1]
                          << "," << (int)v0.boneIndices[2] << "," << (int)v0.boneIndices[3]
                          << "] weights=[" << v0.boneWeights[0] << "," << v0.boneWeights[1]
                          << "," << v0.boneWeights[2] << "," << v0.boneWeights[3] << "]\n";
            }

            auto meshResult = graphics_->createMeshFromData(modelData->meshes[0]);
            if (meshResult) {
                characterMesh_ = *meshResult;
                std::cout << "Created character mesh with " << modelData->meshes[0].vertices.size()
                          << " vertices\n";
            } else {
                std::cerr << "Failed to create mesh from model data, using cube fallback\n";
                auto cubeResult = graphics_->createCubeMesh(1.0f);
                if (cubeResult) {
                    characterMesh_ = *cubeResult;
                }
            }
        } else {
            std::cerr << "No meshes in model, using cube fallback\n";
            auto cubeResult = graphics_->createCubeMesh(1.0f);
            if (cubeResult) {
                characterMesh_ = *cubeResult;
            }
        }

        modelLoaded_ = true;
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

                // Update animation system
                if (animation_) {
                    animation_->update(fixedDt);
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
    // Input Handling
    //======================================================================

    void handleInput() {
        if (!input_) return;

        // ESC to quit
        if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            running_ = false;
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

        // Switch animations with number keys
        if (!clipHandles_.empty()) {
            for (int i = 0; i < std::min(9, static_cast<int>(clipHandles_.size())); ++i) {
                if (input_->wasKeyJustPressed(GLFW_KEY_1 + i)) {
                    currentClipIndex_ = i;
                    animation_->play(animatorHandle_, clipHandles_[i], 0.3f);
                    std::cout << "Switched to animation " << i << std::endl;
                }
            }
        }

        // Space to pause/unpause
        if (input_->wasKeyJustPressed(GLFW_KEY_SPACE)) {
            bool paused = animation_->isPaused(animatorHandle_);
            animation_->setPaused(animatorHandle_, !paused);
            std::cout << (paused ? "Resumed" : "Paused") << " animation\n";
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

        // Draw ground
        if (groundMesh_) {
            bestow::Mat4 groundMatrix = glm::identity<glm::mat4>();
            graphics_->drawMesh(groundMesh_, groundMaterial_, groundMatrix, false, true);
        }

        // Draw character
        if (modelLoaded_ && animatorHandle_) {
            // Get bone transforms from animation system
            auto boneTransforms = animation_->getBoneTransforms(animatorHandle_);
            bestow::Mat4 characterMatrix = glm::identity<glm::mat4>();

            // Draw the mesh if enabled (currently not animated - skinning not implemented)
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
        // Display animation info
        if (modelLoaded_) {
            std::string info = "Animation: " + std::to_string(currentClipIndex_ + 1) +
                              "/" + std::to_string(clipHandles_.size());

            // Note: Text rendering would need to be implemented
            // For now, just output to console occasionally
        }
    }
};

}  // namespace showcase
