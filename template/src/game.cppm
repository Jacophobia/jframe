// src/game.cppm
// Bestow Game Template - Application with Automatic Dependency Injection
//
// This template demonstrates the recommended Application<> base class pattern:
// 1. Inherit from Application<YourGame, Dep1, Dep2, ...>
// 2. Constructor receives dependencies directly
// 3. Engine auto-detects deps: engine.run<MyGame>()

module;

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module my.game;

import std;
import bestow.services;   // All contract interfaces + Application base
import bestow.types;
import bestow.graphics3d;

export namespace mygame {

//==========================================================================
// MyGame Application
//
// Inherit from Application<YourGame, Dependencies...> to enable automatic
// dependency injection. Just call engine.run<MyGame>() - no need to list
// dependencies in the run call!
//
// Benefits:
// - Clean run call: engine.run<MyGame>()
// - Dependencies declared once in base class
// - Constructor params naturally match
// - Easy to mock for testing
//==========================================================================

class MyGame : public bestow::Application<MyGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem,
    bestow::IEntitySystem,
    bestow::IEventSystem,
    bestow::IAudioSystem>
{
public:
    // Constructor receives dependencies - params match base class template args
    MyGame(bestow::IGraphics3DSystem& graphics,
           bestow::IInputSystem& input,
           bestow::IEntitySystem& entities,
           bestow::IEventSystem& events,
           bestow::IAudioSystem& audio)
        : graphics_(&graphics)
        , input_(&input)
        , entities_(&entities)
        , events_(&events)
        , audio_(&audio) {}

    ~MyGame() override = default;

    //======================================================================
    // IApplication Interface
    //======================================================================

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
    // Game Lifecycle
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
            .windowTitle = "My Bestow Game",
            .vsync = true,
            .fullscreen = false
        };

        if (!graphics_->initialize(gfxConfig)) {
            std::cerr << "Failed to initialize graphics\n";
            return false;
        }

        graphics_->setClearColor(bestow::Color{30, 30, 50, 255});

        // Initialize input (if available)
        if (input_) {
            input_->initialize(graphics_->getNativeWindowHandle());
        }

        // Create a simple cube mesh
        auto result = graphics_->createCubeMesh(1.0f);
        if (result) {
            cubeMesh_ = *result;
        }

        // Get default material
        material_ = graphics_->getDefaultPBRMaterial();

        // Setup camera (looking at origin from above-right)
        bestow::Camera3D cam;
        cam.fovY = 45.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane = 100.0f;
        cam.transform.position = {5.0f, 5.0f, 5.0f};

        // Calculate rotation to look at origin
        glm::vec3 lookDir = glm::normalize(glm::vec3(-5.0f, -5.0f, -5.0f));
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(up, -lookDir));
        up = glm::cross(-lookDir, right);
        glm::mat3 rotMatrix(right, up, -lookDir);
        glm::quat rotation = glm::quat_cast(rotMatrix);
        cam.transform.rotation = {rotation.w, rotation.x, rotation.y, rotation.z};

        graphics_->setCamera(cam);

        // Setup lighting
        bestow::DirectionalLight light{
            .direction = {0.5f, -1.0f, 0.3f},
            .color = {1.0f, 1.0f, 1.0f},
            .intensity = 1.0f
        };
        graphics_->setDirectionalLight(light);
        graphics_->setAmbientLight({0.2f, 0.2f, 0.3f}, 0.3f);

        return true;
    }

    void gameLoop() {
        constexpr bestow::DeltaTime fixedDt = 1.0f / 60.0f;
        auto previousTime = std::chrono::high_resolution_clock::now();
        bestow::DeltaTime accumulator = 0.0f;

        running_ = true;

        while (running_ && !graphics_->shouldClose()) {
            // Calculate delta time
            auto currentTime = std::chrono::high_resolution_clock::now();
            bestow::DeltaTime frameTime =
                std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;

            // Clamp to prevent spiral of death
            if (frameTime > 0.25f) {
                frameTime = 0.25f;
            }
            accumulator += frameTime;

            // Update input
            if (input_) {
                input_->update();

                // Handle quit - Dvorak friendly: ESC to quit
                if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
                    running_ = false;
                }
            }

            // Update audio
            if (audio_) {
                audio_->update(frameTime);
            }

            // Fixed timestep updates
            while (accumulator >= fixedDt) {
                update(fixedDt);
                accumulator -= fixedDt;
            }

            // Render
            graphics_->beginFrame();
            render();
            graphics_->endFrame();
        }
    }

    void update(bestow::DeltaTime dt) {
        // Rotate the cube
        rotation_ += dt;
    }

    void render() {
        // Draw the rotating cube
        bestow::Mat4 transform = glm::rotate(
            glm::identity<glm::mat4>(),
            rotation_,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        graphics_->drawMesh(cubeMesh_, material_, transform);
    }

    void cleanup() {
        if (input_) {
            input_->shutdown();
        }
        if (graphics_) {
            graphics_->shutdown();
        }
    }

    //======================================================================
    // Dependencies (injected via constructor)
    //======================================================================
    bestow::IGraphics3DSystem* graphics_ = nullptr;
    bestow::IInputSystem* input_ = nullptr;
    bestow::IEntitySystem* entities_ = nullptr;
    bestow::IEventSystem* events_ = nullptr;
    bestow::IAudioSystem* audio_ = nullptr;

    //======================================================================
    // Game State
    //======================================================================
    bestow::MeshHandle cubeMesh_ = 0;
    bestow::MaterialHandle material_ = 0;
    float rotation_ = 0.0f;
    bool running_ = false;

};

}  // namespace mygame
