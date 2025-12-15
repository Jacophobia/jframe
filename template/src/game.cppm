// src/game.cppm
// Bestow Game Template
//
// Replace "MyGame" with your game name and implement your game logic!

module;

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module my.game;

import std;
import bestow.runtime;
import bestow.types;
import bestow.graphics3d;

export class MyGame : public bestow::Game {
public:
    void onStart() override {
        // Create a simple cube mesh
        auto result = graphics3d()->createCubeMesh(1.0f);
        if (result) {
            cubeMesh_ = *result;
        }

        // Get default material
        material_ = graphics3d()->getDefaultPBRMaterial();

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

        graphics3d()->setCamera(cam);

        // Setup lighting
        bestow::DirectionalLight light{
            .direction = {0.5f, -1.0f, 0.3f},
            .color = {1.0f, 1.0f, 1.0f},
            .intensity = 1.0f
        };
        graphics3d()->setDirectionalLight(light);
        graphics3d()->setAmbientLight({0.2f, 0.2f, 0.3f}, 0.3f);
    }

    void onUpdate(bestow::DeltaTime dt) override {
        // Handle input
        // Controls: ,AOE for Dvorak movement, arrow keys also work
        if (input()->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            quit();
        }

        // Rotate the cube
        rotation_ += dt;
    }

    void onRender() override {
        // Draw the rotating cube
        bestow::Mat4 transform = glm::rotate(
            glm::identity<glm::mat4>(),
            rotation_,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        graphics3d()->drawMesh(cubeMesh_, material_, transform);
    }

private:
    bestow::MeshHandle cubeMesh_ = 0;
    bestow::MaterialHandle material_ = 0;
    float rotation_ = 0.0f;
};
