// bestow-vulkan/src/VulkanGraphicsSystem.cpp
// Vulkan 2D graphics system implementation

module;

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

module bestow.vulkan.impl;

import std;
import bestow.graphics;
import bestow.types;
import bestow.assets;
import bestow.entity;
import bestow.utils;

namespace bestow::vulkan {

VulkanGraphicsSystem::VulkanGraphicsSystem() = default;

VulkanGraphicsSystem::~VulkanGraphicsSystem() {
    // Cleanup resources
    if (spriteVertexBuffer_ != 0) {
        context_.destroyBuffer(spriteVertexBuffer_);
    }
    if (primitiveVertexBuffer_ != 0) {
        context_.destroyBuffer(primitiveVertexBuffer_);
    }
    if (spritePipeline_ != 0) {
        context_.destroyPipeline(spritePipeline_);
    }
    if (primitivePipeline_ != 0) {
        context_.destroyPipeline(primitivePipeline_);
    }
}

Result<void, VulkanError> VulkanGraphicsSystem::initialize(const VulkanConfig& config) {
    auto result = context_.initialize(config);
    if (!result) {
        return result;
    }

    // Create vertex buffers
    VulkanBufferDef bufferDef{};
    bufferDef.size = sizeof(SpriteVertex) * 10000;  // Support many sprites
    bufferDef.usage = VulkanBufferUsage::Vertex | VulkanBufferUsage::TransferDst;
    bufferDef.hostVisible = true;
    bufferDef.persistentlyMapped = true;

    auto bufferResult = context_.createBuffer(bufferDef);
    if (!bufferResult) {
        return std::unexpected(bufferResult.error());
    }
    spriteVertexBuffer_ = *bufferResult;

    // Create primitive vertex buffer
    bufferDef.size = sizeof(glm::vec2) * 10000 + sizeof(glm::vec4) * 10000;
    bufferResult = context_.createBuffer(bufferDef);
    if (!bufferResult) {
        return std::unexpected(bufferResult.error());
    }
    primitiveVertexBuffer_ = *bufferResult;

    // Pipelines would be created here with actual SPIR-V shaders
    // For now, we skip pipeline creation as it requires compiled shaders

    return {};
}

void VulkanGraphicsSystem::beginFrame() {
    spriteVertices_.clear();
    primitiveVertices_.clear();
    primitiveColors_.clear();
    context_.beginFrame();
}

void VulkanGraphicsSystem::endFrame() {
    flushSpriteBatch();
    flushPrimitives();
    context_.endFrame();
}

void VulkanGraphicsSystem::draw(const Sprite& sprite) {
    // Convert sprite to vertices
    glm::vec2 pos{sprite.transform.x, sprite.transform.y};
    glm::vec2 size{static_cast<float>(sprite.sourceRect.size.width),
                   static_cast<float>(sprite.sourceRect.size.height)};

    glm::vec4 color{
        sprite.tint.r / 255.0f,
        sprite.tint.g / 255.0f,
        sprite.tint.b / 255.0f,
        sprite.tint.a / 255.0f
    };

    // Two triangles for quad
    spriteVertices_.push_back({pos, {0, 0}, color});
    spriteVertices_.push_back({pos + glm::vec2{size.x, 0}, {1, 0}, color});
    spriteVertices_.push_back({pos + glm::vec2{size.x, size.y}, {1, 1}, color});

    spriteVertices_.push_back({pos, {0, 0}, color});
    spriteVertices_.push_back({pos + glm::vec2{size.x, size.y}, {1, 1}, color});
    spriteVertices_.push_back({pos + glm::vec2{0, size.y}, {0, 1}, color});
}

void VulkanGraphicsSystem::drawBatch(std::span<const Sprite> sprites) {
    for (const auto& sprite : sprites) {
        draw(sprite);
    }
}

void VulkanGraphicsSystem::drawSprite(const SpriteSheet& sheet, int frameIndex,
                                       const Transform2D& transform, Color tint) {
    // Calculate frame UV coordinates
    int col = frameIndex % sheet.columns;
    int row = frameIndex / sheet.columns;
    float frameWidth = 1.0f / sheet.columns;
    float frameHeight = 1.0f / sheet.rows;

    float u0 = col * frameWidth;
    float v0 = row * frameHeight;
    float u1 = u0 + frameWidth;
    float v1 = v0 + frameHeight;

    glm::vec2 pos{transform.x, transform.y};
    glm::vec2 size{static_cast<float>(sheet.frameWidth), static_cast<float>(sheet.frameHeight)};

    glm::vec4 color{tint.r / 255.0f, tint.g / 255.0f, tint.b / 255.0f, tint.a / 255.0f};

    spriteVertices_.push_back({pos, {u0, v0}, color});
    spriteVertices_.push_back({pos + glm::vec2{size.x, 0}, {u1, v0}, color});
    spriteVertices_.push_back({pos + glm::vec2{size.x, size.y}, {u1, v1}, color});

    spriteVertices_.push_back({pos, {u0, v0}, color});
    spriteVertices_.push_back({pos + glm::vec2{size.x, size.y}, {u1, v1}, color});
    spriteVertices_.push_back({pos + glm::vec2{0, size.y}, {u0, v1}, color});
}

void VulkanGraphicsSystem::drawAnimatedSprite(AnimatedSprite& sprite,
                                               const Transform2D& transform, Color tint) {
    // Get current animation if one is active
    if (sprite.currentAnimation.empty() || !sprite.playing) {
        // Draw first frame of the sheet if no animation is playing
        drawSprite(sprite.sheet, 0, transform, tint);
        return;
    }

    auto it = sprite.animations.find(sprite.currentAnimation);
    if (it == sprite.animations.end()) {
        drawSprite(sprite.sheet, 0, transform, tint);
        return;
    }

    const Animation& anim = it->second;
    if (anim.frames.empty()) {
        drawSprite(sprite.sheet, 0, transform, tint);
        return;
    }

    // Update frame timer and advance frame
    // Each frame has its own duration
    sprite.frameTimer += 1.0f / 60.0f;  // Assuming 60fps
    const AnimationFrame& currentFrame = anim.frames[sprite.currentFrameIndex];

    while (sprite.frameTimer >= currentFrame.duration) {
        sprite.frameTimer -= currentFrame.duration;
        sprite.currentFrameIndex++;
        if (sprite.currentFrameIndex >= static_cast<int>(anim.frames.size())) {
            if (anim.looping) {
                sprite.currentFrameIndex = 0;
            } else {
                sprite.currentFrameIndex = static_cast<int>(anim.frames.size()) - 1;
                sprite.playing = false;
                break;
            }
        }
    }

    // Draw current frame
    int frameIndex = anim.frames[sprite.currentFrameIndex].frameIndex;
    drawSprite(sprite.sheet, frameIndex, transform, tint);
}

void VulkanGraphicsSystem::drawRect(const Canvas& rect, const Color& color, bool filled) {
    glm::vec2 pos{static_cast<float>(rect.origin.x), static_cast<float>(rect.origin.y)};
    glm::vec2 size{static_cast<float>(rect.size.width), static_cast<float>(rect.size.height)};
    glm::vec4 col{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

    if (filled) {
        primitiveVertices_.push_back(pos);
        primitiveVertices_.push_back(pos + glm::vec2{size.x, 0});
        primitiveVertices_.push_back(pos + size);

        primitiveVertices_.push_back(pos);
        primitiveVertices_.push_back(pos + size);
        primitiveVertices_.push_back(pos + glm::vec2{0, size.y});

        for (int i = 0; i < 6; ++i) {
            primitiveColors_.push_back(col);
        }
    } else {
        // Draw outline as lines
        glm::vec2 corners[4] = {
            pos, pos + glm::vec2{size.x, 0}, pos + size, pos + glm::vec2{0, size.y}
        };

        for (int i = 0; i < 4; ++i) {
            primitiveVertices_.push_back(corners[i]);
            primitiveVertices_.push_back(corners[(i + 1) % 4]);
            primitiveColors_.push_back(col);
            primitiveColors_.push_back(col);
        }
    }
}

void VulkanGraphicsSystem::drawLine(Vec2 from, Vec2 to, const Color& color, float thickness) {
    glm::vec4 col{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

    // For thick lines, create a quad
    if (thickness > 1.0f) {
        glm::vec2 dir = glm::normalize(glm::vec2{to.x - from.x, to.y - from.y});
        glm::vec2 perp{-dir.y, dir.x};
        float halfWidth = thickness / 2.0f;

        glm::vec2 p0{from.x, from.y};
        glm::vec2 p1{to.x, to.y};

        primitiveVertices_.push_back(p0 - perp * halfWidth);
        primitiveVertices_.push_back(p0 + perp * halfWidth);
        primitiveVertices_.push_back(p1 + perp * halfWidth);

        primitiveVertices_.push_back(p0 - perp * halfWidth);
        primitiveVertices_.push_back(p1 + perp * halfWidth);
        primitiveVertices_.push_back(p1 - perp * halfWidth);

        for (int i = 0; i < 6; ++i) {
            primitiveColors_.push_back(col);
        }
    } else {
        primitiveVertices_.push_back({from.x, from.y});
        primitiveVertices_.push_back({to.x, to.y});
        primitiveColors_.push_back(col);
        primitiveColors_.push_back(col);
    }
}

void VulkanGraphicsSystem::drawCircle(Vec2 center, float radius, const Color& color,
                                       bool filled, int segments) {
    glm::vec4 col{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
    using bestow::core::Math::TWO_PI;

    if (filled) {
        for (int i = 0; i < segments; ++i) {
            float angle1 = TWO_PI * i / segments;
            float angle2 = TWO_PI * (i + 1) / segments;

            primitiveVertices_.push_back({center.x, center.y});
            primitiveVertices_.push_back({center.x + radius * std::cos(angle1),
                                          center.y + radius * std::sin(angle1)});
            primitiveVertices_.push_back({center.x + radius * std::cos(angle2),
                                          center.y + radius * std::sin(angle2)});

            primitiveColors_.push_back(col);
            primitiveColors_.push_back(col);
            primitiveColors_.push_back(col);
        }
    } else {
        for (int i = 0; i < segments; ++i) {
            float angle1 = TWO_PI * i / segments;
            float angle2 = TWO_PI * (i + 1) / segments;

            primitiveVertices_.push_back({center.x + radius * std::cos(angle1),
                                          center.y + radius * std::sin(angle1)});
            primitiveVertices_.push_back({center.x + radius * std::cos(angle2),
                                          center.y + radius * std::sin(angle2)});
            primitiveColors_.push_back(col);
            primitiveColors_.push_back(col);
        }
    }
}

void VulkanGraphicsSystem::drawPolygon(std::span<const Vec2> vertices, const Color& color, bool filled) {
    if (vertices.size() < 3) return;

    glm::vec4 col{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};

    if (filled) {
        // Fan triangulation
        for (std::size_t i = 1; i < vertices.size() - 1; ++i) {
            primitiveVertices_.push_back({vertices[0].x, vertices[0].y});
            primitiveVertices_.push_back({vertices[i].x, vertices[i].y});
            primitiveVertices_.push_back({vertices[i + 1].x, vertices[i + 1].y});
            primitiveColors_.push_back(col);
            primitiveColors_.push_back(col);
            primitiveColors_.push_back(col);
        }
    } else {
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            primitiveVertices_.push_back({vertices[i].x, vertices[i].y});
            primitiveVertices_.push_back({vertices[(i + 1) % vertices.size()].x,
                                          vertices[(i + 1) % vertices.size()].y});
            primitiveColors_.push_back(col);
            primitiveColors_.push_back(col);
        }
    }
}

void VulkanGraphicsSystem::drawText(const std::string& text, Vec2 position,
                                     AssetHandle fontHandle, float size, const Color& color) {
    // Text rendering would require font atlas and glyph data
    // Simplified implementation - draw placeholder rectangles
    float x = position.x;
    for (char c : text) {
        if (c != ' ') {
            Canvas charRect{{static_cast<int>(x), static_cast<int>(position.y)},
                           {static_cast<int>(size * 0.6f), static_cast<int>(size)}};
            drawRect(charRect, color, true);
        }
        x += size * 0.6f;
    }
}

void VulkanGraphicsSystem::drawTextCentered(const std::string& text, Vec2 position,
                                             AssetHandle fontHandle, float size, const Color& color) {
    Vec2 textSize = measureText(text, fontHandle, size);
    Vec2 centeredPos{position.x - textSize.x / 2, position.y - textSize.y / 2};
    drawText(text, centeredPos, fontHandle, size, color);
}

Vec2 VulkanGraphicsSystem::measureText(const std::string& text, AssetHandle fontHandle, float size) const {
    return Vec2{static_cast<float>(text.length()) * size * 0.6f, size};
}

void VulkanGraphicsSystem::setCamera(const Camera& camera) {
    camera_ = camera;
}

Camera VulkanGraphicsSystem::getCamera() const {
    return camera_;
}

Vec2 VulkanGraphicsSystem::worldToScreen(Vec2 worldPos) const {
    Size windowSize = context_.getWindowSize();
    return Vec2{
        (worldPos.x - camera_.transform.x) * camera_.zoom + windowSize.width / 2.0f,
        (worldPos.y - camera_.transform.y) * camera_.zoom + windowSize.height / 2.0f
    };
}

Vec2 VulkanGraphicsSystem::screenToWorld(Vec2 screenPos) const {
    Size windowSize = context_.getWindowSize();
    return Vec2{
        (screenPos.x - windowSize.width / 2.0f) / camera_.zoom + camera_.transform.x,
        (screenPos.y - windowSize.height / 2.0f) / camera_.zoom + camera_.transform.y
    };
}

Size VulkanGraphicsSystem::getWindowSize() const {
    return context_.getWindowSize();
}

void VulkanGraphicsSystem::setWindowSize(Size size) {
    context_.setWindowSize(size);
}

bool VulkanGraphicsSystem::isFullscreen() const {
    return isFullscreen_;
}

void VulkanGraphicsSystem::setFullscreen(bool fullscreen) {
    if (isFullscreen_ == fullscreen) return;

    isFullscreen_ = fullscreen;

    GLFWwindow* window = context_.getWindow();
    if (!window) return;

    if (fullscreen) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(window, nullptr, 100, 100, 800, 600, 0);
    }
}

bool VulkanGraphicsSystem::shouldClose() const {
    GLFWwindow* window = context_.getWindow();
    return window ? glfwWindowShouldClose(window) : false;
}

void* VulkanGraphicsSystem::getNativeWindowHandle() const {
    return context_.getWindow();
}

void VulkanGraphicsSystem::setClearColor(const Color& color) {
    clearColor_ = color;
}

void VulkanGraphicsSystem::setVSync(bool enabled) {
    // VSync is set during swapchain creation
    // Would need to recreate swapchain to change
}

void VulkanGraphicsSystem::setAssetSystem(IAssetSystem* assets) {
    assetSystem_ = assets;
}

void VulkanGraphicsSystem::renderEntities(IEntitySystem& entities) {
    // Would iterate through entities with visual components and render them
}

void VulkanGraphicsSystem::renderEntities(IEntitySystem& entities,
                                           RenderLayer minLayer, RenderLayer maxLayer) {
    // Would iterate through entities in layer range
}

void VulkanGraphicsSystem::setViewportCulling(bool enabled) {
    viewportCullingEnabled_ = enabled;
}

bool VulkanGraphicsSystem::isViewportCullingEnabled() const {
    return viewportCullingEnabled_;
}

void VulkanGraphicsSystem::flushSpriteBatch() {
    if (spriteVertices_.empty()) return;

    VkCommandBuffer cmd = context_.getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE || spritePipeline_ == 0) {
        spriteVertices_.clear();
        return;
    }

    // Upload vertex data
    context_.uploadToBuffer(spriteVertexBuffer_,
                            spriteVertices_.data(),
                            spriteVertices_.size() * sizeof(SpriteVertex));

    // Compute orthographic projection for 2D
    Size windowSize = getWindowSize();
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowSize.width),
                                       static_cast<float>(windowSize.height), 0.0f,
                                       -1.0f, 1.0f);

    // Bind pipeline
    context_.bindPipeline(spritePipeline_);
    VkPipelineLayout layout = context_.getPipelineLayout(spritePipeline_);

    // Push constants: projection matrix
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                      sizeof(glm::mat4), &projection);

    // Bind vertex buffer
    VkBuffer vertexBuffer = context_.getBuffer(spriteVertexBuffer_);
    if (vertexBuffer != VK_NULL_HANDLE) {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
        vkCmdDraw(cmd, static_cast<std::uint32_t>(spriteVertices_.size()), 1, 0, 0);
    }

    spriteVertices_.clear();
}

void VulkanGraphicsSystem::flushPrimitives() {
    if (primitiveVertices_.empty()) return;

    VkCommandBuffer cmd = context_.getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE || primitivePipeline_ == 0) {
        primitiveVertices_.clear();
        primitiveColors_.clear();
        return;
    }

    // Would upload and draw primitives here
    primitiveVertices_.clear();
    primitiveColors_.clear();
}

namespace {
    std::vector<std::uint32_t> loadSpirv2D(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            return {};
        }
        std::size_t fileSize = static_cast<std::size_t>(file.tellg());
        std::vector<std::uint32_t> buffer(fileSize / sizeof(std::uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));
        return buffer;
    }

    // Find shader file using PathResolver with default paths
    std::optional<std::string> findShader2D(std::string_view shaderFilename) {
        // Default shader search paths for 2D system
        std::vector<std::string> searchPaths = {":assets:/shaders", ":library:/shaders", "shaders"};

        for (const auto& basePath : searchPaths) {
            auto resolvedBase = bestow::PathResolver::resolve(basePath);
            auto fullPath = resolvedBase / std::filesystem::path(shaderFilename);

            if (std::filesystem::exists(fullPath)) {
                return fullPath.string();
            }
        }
        return std::nullopt;
    }
}

void VulkanGraphicsSystem::createPipelines() {
    // Find 2D shaders using PathResolver
    auto sprite2dVertPath = findShader2D("sprite2d.vert.spv");
    auto sprite2dFragPath = findShader2D("sprite2d.frag.spv");

    // Load 2D sprite shaders
    auto sprite2dVertSpirv = sprite2dVertPath ? loadSpirv2D(*sprite2dVertPath) : std::vector<std::uint32_t>{};
    auto sprite2dFragSpirv = sprite2dFragPath ? loadSpirv2D(*sprite2dFragPath) : std::vector<std::uint32_t>{};

    if (!sprite2dVertSpirv.empty() && !sprite2dFragSpirv.empty()) {
        VulkanPipelineDef spriteDef;
        spriteDef.shaderStages = {
            {VK_SHADER_STAGE_VERTEX_BIT, sprite2dVertSpirv, "main"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, sprite2dFragSpirv, "main"}
        };

        // Sprite vertex layout: position (vec2) + texcoord (vec2) + color (vec4)
        spriteDef.vertexBindings = {
            {0, sizeof(SpriteVertex), VK_VERTEX_INPUT_RATE_VERTEX}
        };
        spriteDef.vertexAttributes = {
            {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SpriteVertex, position)},
            {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SpriteVertex, texCoord)},
            {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SpriteVertex, color)}
        };

        // Push constants: projection matrix
        spriteDef.pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)}
        };

        spriteDef.depthTestEnable = false;
        spriteDef.depthWriteEnable = false;
        spriteDef.blendEnable = true;
        spriteDef.cullMode = VK_CULL_MODE_NONE;

        auto result = context_.createPipeline(spriteDef);
        if (result) {
            spritePipeline_ = *result;
        }
    }

    // Create sprite vertex buffer (large enough for batched sprites)
    constexpr std::size_t MAX_SPRITES = 10000;
    constexpr std::size_t VERTICES_PER_SPRITE = 6;  // 2 triangles
    VulkanBufferDef vertexBufferDef{
        .size = MAX_SPRITES * VERTICES_PER_SPRITE * sizeof(SpriteVertex),
        .usage = VulkanBufferUsage::Vertex | VulkanBufferUsage::TransferDst,
        .hostVisible = true,
        .debugName = "SpriteVertexBuffer"
    };
    auto bufferResult = context_.createBuffer(vertexBufferDef);
    if (bufferResult) {
        spriteVertexBuffer_ = *bufferResult;
    }
}

}  // namespace bestow::vulkan
