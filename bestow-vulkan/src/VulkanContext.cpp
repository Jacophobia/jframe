// bestow-vulkan/src/VulkanContext.cpp
// Vulkan context implementation - instance, device, swapchain management

module;

// VMA implementation is in VmaImpl.cpp - this is just declarations
#include <vk_mem_alloc.h>

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <GLFW/glfw3.h>

module bestow.vulkan.impl;

import std;

namespace bestow::vulkan {

//==========================================================================
// Debug Callback
//==========================================================================

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    const char* severity = "UNKNOWN";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        severity = "ERROR";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        severity = "WARNING";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        severity = "INFO";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        severity = "VERBOSE";
    }

    std::fprintf(stderr, "[Vulkan %s] %s\n", severity, pCallbackData->pMessage);

    return VK_FALSE;
}

//==========================================================================
// VulkanContext Implementation
//==========================================================================

VulkanContext::~VulkanContext() {
    shutdown();
}

VulkanContext::VulkanContext(VulkanContext&& other) noexcept {
    *this = std::move(other);
}

VulkanContext& VulkanContext::operator=(VulkanContext&& other) noexcept {
    if (this != &other) {
        shutdown();

        config_ = other.config_;
        initialized_ = other.initialized_;
        window_ = other.window_;
        instance_ = other.instance_;
        debugMessenger_ = other.debugMessenger_;
        surface_ = other.surface_;
        physicalDevice_ = other.physicalDevice_;
        device_ = other.device_;
        graphicsQueue_ = other.graphicsQueue_;
        presentQueue_ = other.presentQueue_;
        graphicsQueueFamily_ = other.graphicsQueueFamily_;
        presentQueueFamily_ = other.presentQueueFamily_;
        swapchain_ = other.swapchain_;
        swapchainImages_ = std::move(other.swapchainImages_);
        swapchainImageViews_ = std::move(other.swapchainImageViews_);
        swapchainImageFormat_ = other.swapchainImageFormat_;
        swapchainExtent_ = other.swapchainExtent_;
        depthImage_ = other.depthImage_;
        depthImageAllocation_ = other.depthImageAllocation_;
        depthImageView_ = other.depthImageView_;
        depthFormat_ = other.depthFormat_;
        renderPass_ = other.renderPass_;
        framebuffers_ = std::move(other.framebuffers_);
        commandPool_ = other.commandPool_;
        commandBuffers_ = std::move(other.commandBuffers_);
        imageAvailableSemaphores_ = std::move(other.imageAvailableSemaphores_);
        renderFinishedSemaphores_ = std::move(other.renderFinishedSemaphores_);
        inFlightFences_ = std::move(other.inFlightFences_);
        currentFrame_ = other.currentFrame_;
        allocator_ = other.allocator_;
        buffers_ = std::move(other.buffers_);
        nextBufferHandle_ = other.nextBufferHandle_;
        images_ = std::move(other.images_);
        nextImageHandle_ = other.nextImageHandle_;
        pipelines_ = std::move(other.pipelines_);
        nextPipelineHandle_ = other.nextPipelineHandle_;

        // Reset other
        other.initialized_ = false;
        other.window_ = nullptr;
        other.instance_ = VK_NULL_HANDLE;
        other.device_ = VK_NULL_HANDLE;
        other.allocator_ = VK_NULL_HANDLE;
    }
    return *this;
}

Result<void, VulkanError> VulkanContext::initialize(const VulkanConfig& config) {
    config_ = config;

    // Create window (unless headless)
    if (!config_.headless) {
        if (!glfwInit()) {
            return std::unexpected(VulkanError::InstanceCreationFailed);
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window_ = glfwCreateWindow(
            config_.windowWidth,
            config_.windowHeight,
            config_.windowTitle.c_str(),
            nullptr,
            nullptr
        );

        if (!window_) {
            glfwTerminate();
            return std::unexpected(VulkanError::SurfaceCreationFailed);
        }
    }

    // Initialize Vulkan
    if (auto result = createInstance(); !result) return result;
    if (!config_.headless) {
        if (auto result = createSurface(); !result) return result;
    }
    if (auto result = selectPhysicalDevice(); !result) return result;
    if (auto result = createLogicalDevice(); !result) return result;
    if (auto result = createAllocator(); !result) return result;

    if (config_.headless) {
        swapchainExtent_ = {
            static_cast<std::uint32_t>(config_.headlessWidth),
            static_cast<std::uint32_t>(config_.headlessHeight)
        };
        if (auto result = createHeadlessResources(); !result) return result;
    } else {
        if (auto result = createSwapchain(); !result) return result;
    }

    if (auto result = createDepthResources(); !result) return result;
    if (auto result = createRenderPass(); !result) return result;
    if (auto result = createFramebuffers(); !result) return result;
    if (auto result = createCommandPool(); !result) return result;
    if (auto result = createCommandBuffers(); !result) return result;
    if (auto result = createSyncObjects(); !result) return result;

    initialized_ = true;
    return {};
}

void VulkanContext::shutdown() {
    if (!initialized_) return;

    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    // Destroy user resources
    for (auto& [handle, buffer] : buffers_) {
        if (buffer.buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator_, buffer.buffer, buffer.allocation);
        }
    }
    buffers_.clear();

    for (auto& [handle, image] : images_) {
        if (image.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device_, image.sampler, nullptr);
        }
        if (image.view != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, image.view, nullptr);
        }
        if (image.image != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator_, image.image, image.allocation);
        }
    }
    images_.clear();

    for (auto& [handle, pipeline] : pipelines_) {
        if (pipeline.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, pipeline.pipeline, nullptr);
        }
        if (pipeline.layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_, pipeline.layout, nullptr);
        }
    }
    pipelines_.clear();

    // Destroy sync objects
    for (auto semaphore : imageAvailableSemaphores_) {
        vkDestroySemaphore(device_, semaphore, nullptr);
    }
    for (auto semaphore : renderFinishedSemaphores_) {
        vkDestroySemaphore(device_, semaphore, nullptr);
    }
    for (auto fence : inFlightFences_) {
        vkDestroyFence(device_, fence, nullptr);
    }

    // Destroy command pool
    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
    }

    // Destroy framebuffers
    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    // Destroy render pass
    if (renderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_, renderPass_, nullptr);
    }

    // Destroy depth resources
    if (depthImageView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, depthImageView_, nullptr);
    }
    if (depthImage_ != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator_, depthImage_, depthImageAllocation_);
    }

    // Destroy headless resources
    if (config_.headless) {
        if (headlessFramebuffer_ != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device_, headlessFramebuffer_, nullptr);
        }
        if (headlessColorView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, headlessColorView_, nullptr);
        }
        if (headlessColorImage_ != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator_, headlessColorImage_, headlessColorAllocation_);
        }
    }

    // Destroy swapchain
    cleanupSwapchain();

    // Destroy VMA allocator
    if (allocator_ != VK_NULL_HANDLE) {
        vmaDestroyAllocator(allocator_);
    }

    // Destroy device
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
    }

    // Destroy surface
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }

    // Destroy debug messenger
    if (debugMessenger_ != VK_NULL_HANDLE) {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
        if (func) {
            func(instance_, debugMessenger_, nullptr);
        }
    }

    // Destroy instance
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }

    // Destroy window
    if (window_) {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    initialized_ = false;
}

Result<void, VulkanError> VulkanContext::createInstance() {
    vkb::InstanceBuilder builder;

    auto instRet = builder
        .set_app_name(config_.applicationName.c_str())
        .set_app_version(config_.applicationVersion)
        .require_api_version(1, 2, 0)
        .set_debug_callback(debugCallback);

    if (config_.enableValidation) {
        instRet = instRet.request_validation_layers();
    }

    auto inst = instRet.build();
    if (!inst) {
        return std::unexpected(VulkanError::InstanceCreationFailed);
    }

    vkbInstance_ = inst.value();
    instance_ = vkbInstance_->instance;
    debugMessenger_ = vkbInstance_->debug_messenger;

    return {};
}

Result<void, VulkanError> VulkanContext::createSurface() {
    if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
        return std::unexpected(VulkanError::SurfaceCreationFailed);
    }
    return {};
}

Result<void, VulkanError> VulkanContext::selectPhysicalDevice() {
    if (!vkbInstance_) {
        return std::unexpected(VulkanError::NoSuitableGPU);
    }

    vkb::PhysicalDeviceSelector selector{*vkbInstance_};

    auto selectResult = selector
        .set_minimum_version(1, 2);

    if (!config_.headless) {
        selectResult = selectResult.set_surface(surface_);
    }

    if (config_.preferDiscreteGPU) {
        selectResult = selectResult.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);
    }

    auto physDevice = selectResult.select();
    if (!physDevice) {
        return std::unexpected(VulkanError::NoSuitableGPU);
    }

    vkbPhysicalDevice_ = physDevice.value();
    physicalDevice_ = vkbPhysicalDevice_->physical_device;
    return {};
}

Result<void, VulkanError> VulkanContext::createLogicalDevice() {
    if (!vkbPhysicalDevice_) {
        return std::unexpected(VulkanError::DeviceCreationFailed);
    }

    vkb::DeviceBuilder deviceBuilder{*vkbPhysicalDevice_};

    auto devRet = deviceBuilder.build();

    if (!devRet) {
        return std::unexpected(VulkanError::DeviceCreationFailed);
    }

    device_ = devRet.value().device;

    auto graphicsQueueRet = devRet.value().get_queue(vkb::QueueType::graphics);
    if (!graphicsQueueRet) {
        return std::unexpected(VulkanError::DeviceCreationFailed);
    }
    graphicsQueue_ = graphicsQueueRet.value();

    auto graphicsQueueIdxRet = devRet.value().get_queue_index(vkb::QueueType::graphics);
    if (!graphicsQueueIdxRet) {
        return std::unexpected(VulkanError::DeviceCreationFailed);
    }
    graphicsQueueFamily_ = graphicsQueueIdxRet.value();

    if (!config_.headless) {
        auto presentQueueRet = devRet.value().get_queue(vkb::QueueType::present);
        if (presentQueueRet) {
            presentQueue_ = presentQueueRet.value();
        } else {
            presentQueue_ = graphicsQueue_;
        }

        auto presentQueueIdxRet = devRet.value().get_queue_index(vkb::QueueType::present);
        if (presentQueueIdxRet) {
            presentQueueFamily_ = presentQueueIdxRet.value();
        } else {
            presentQueueFamily_ = graphicsQueueFamily_;
        }
    }

    return {};
}

Result<void, VulkanError> VulkanContext::createAllocator() {
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorInfo.physicalDevice = physicalDevice_;
    allocatorInfo.device = device_;
    allocatorInfo.instance = instance_;

    if (vmaCreateAllocator(&allocatorInfo, &allocator_) != VK_SUCCESS) {
        return std::unexpected(VulkanError::MemoryAllocationFailed);
    }

    return {};
}

Result<void, VulkanError> VulkanContext::createSwapchain() {
    vkb::SwapchainBuilder swapchainBuilder{physicalDevice_, device_, surface_};

    auto swapRet = swapchainBuilder
        .set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(config_.vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
        .set_desired_extent(config_.windowWidth, config_.windowHeight)
        .build();

    if (!swapRet) {
        return std::unexpected(VulkanError::SwapchainCreationFailed);
    }

    swapchain_ = swapRet.value().swapchain;
    swapchainImages_ = swapRet.value().get_images().value();
    swapchainImageViews_ = swapRet.value().get_image_views().value();
    swapchainImageFormat_ = swapRet.value().image_format;
    swapchainExtent_ = swapRet.value().extent;

    return {};
}

Result<void, VulkanError> VulkanContext::createHeadlessResources() {
    swapchainImageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;

    // Create color image for headless rendering
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = swapchainImageFormat_;
    imageInfo.extent.width = swapchainExtent_.width;
    imageInfo.extent.height = swapchainExtent_.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(allocator_, &imageInfo, &allocInfo,
                       &headlessColorImage_, &headlessColorAllocation_, nullptr) != VK_SUCCESS) {
        return std::unexpected(VulkanError::ImageCreationFailed);
    }

    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = headlessColorImage_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapchainImageFormat_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &viewInfo, nullptr, &headlessColorView_) != VK_SUCCESS) {
        return std::unexpected(VulkanError::ImageCreationFailed);
    }

    return {};
}

Result<void, VulkanError> VulkanContext::createDepthResources() {
    depthFormat_ = findDepthFormat();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = depthFormat_;
    imageInfo.extent.width = swapchainExtent_.width;
    imageInfo.extent.height = swapchainExtent_.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(allocator_, &imageInfo, &allocInfo,
                       &depthImage_, &depthImageAllocation_, nullptr) != VK_SUCCESS) {
        return std::unexpected(VulkanError::ImageCreationFailed);
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &viewInfo, nullptr, &depthImageView_) != VK_SUCCESS) {
        return std::unexpected(VulkanError::ImageCreationFailed);
    }

    return {};
}

Result<void, VulkanError> VulkanContext::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = config_.headless ?
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL :
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat_;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        return std::unexpected(VulkanError::RenderPassCreationFailed);
    }

    return {};
}

Result<void, VulkanError> VulkanContext::createFramebuffers() {
    if (config_.headless) {
        std::array<VkImageView, 2> attachments = {headlessColorView_, depthImageView_};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent_.width;
        framebufferInfo.height = swapchainExtent_.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &headlessFramebuffer_) != VK_SUCCESS) {
            return std::unexpected(VulkanError::FramebufferCreationFailed);
        }

        framebuffers_.push_back(headlessFramebuffer_);
    } else {
        framebuffers_.resize(swapchainImageViews_.size());

        for (std::size_t i = 0; i < swapchainImageViews_.size(); ++i) {
            std::array<VkImageView, 2> attachments = {swapchainImageViews_[i], depthImageView_};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass_;
            framebufferInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapchainExtent_.width;
            framebufferInfo.height = swapchainExtent_.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
                return std::unexpected(VulkanError::FramebufferCreationFailed);
            }
        }
    }

    return {};
}

Result<void, VulkanError> VulkanContext::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamily_;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        return std::unexpected(VulkanError::CommandPoolCreationFailed);
    }

    return {};
}

Result<void, VulkanError> VulkanContext::createCommandBuffers() {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<std::uint32_t>(commandBuffers_.size());

    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
        return std::unexpected(VulkanError::CommandBufferAllocationFailed);
    }

    return {};
}

Result<void, VulkanError> VulkanContext::createSyncObjects() {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    // Per Vulkan best practices: renderFinishedSemaphores should be indexed by swapchain image
    // to avoid semaphore reuse issues. See: https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
    std::size_t swapchainImageCount = config_.headless ? 1 : swapchainImages_.size();
    renderFinishedSemaphores_.resize(swapchainImageCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create per-frame resources (acquire semaphores and fences)
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            return std::unexpected(VulkanError::SyncObjectCreationFailed);
        }
    }

    // Create per-swapchain-image resources (render finished semaphores)
    for (std::size_t i = 0; i < swapchainImageCount; ++i) {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS) {
            return std::unexpected(VulkanError::SyncObjectCreationFailed);
        }
    }

    return {};
}

void VulkanContext::cleanupSwapchain() {
    for (auto imageView : swapchainImageViews_) {
        vkDestroyImageView(device_, imageView, nullptr);
    }
    swapchainImageViews_.clear();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

Result<void, VulkanError> VulkanContext::recreateSwapchain() {
    int width = 0, height = 0;
    if (window_) {
        glfwGetFramebufferSize(window_, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window_, &width, &height);
            glfwWaitEvents();
        }
    }

    vkDeviceWaitIdle(device_);

    // Cleanup
    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    framebuffers_.clear();

    vkDestroyImageView(device_, depthImageView_, nullptr);
    vmaDestroyImage(allocator_, depthImage_, depthImageAllocation_);

    vkDestroyRenderPass(device_, renderPass_, nullptr);

    cleanupSwapchain();

    // Recreate
    if (auto result = createSwapchain(); !result) return result;
    if (auto result = createDepthResources(); !result) return result;
    if (auto result = createRenderPass(); !result) return result;
    if (auto result = createFramebuffers(); !result) return result;

    return {};
}

VkFormat VulkanContext::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat VulkanContext::findSupportedFormat(
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) {

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    return VK_FORMAT_D32_SFLOAT;  // Fallback
}

Result<void, VulkanError> VulkanContext::beginFrame() {
    // Wait for previous frame to finish
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    if (!config_.headless) {
        // Acquire next swapchain image
        VkResult result = vkAcquireNextImageKHR(
            device_, swapchain_, UINT64_MAX,
            imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &currentImageIndex_);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            if (auto recreateResult = recreateSwapchain(); !recreateResult) {
                return recreateResult;
            }
            return {};
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            return std::unexpected(VulkanError::SwapchainCreationFailed);
        }
    } else {
        currentImageIndex_ = 0;
    }

    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    // Reset and begin command buffer
    currentCommandBuffer_ = commandBuffers_[currentFrame_];
    vkResetCommandBuffer(currentCommandBuffer_, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(currentCommandBuffer_, &beginInfo) != VK_SUCCESS) {
        return std::unexpected(VulkanError::CommandBufferAllocationFailed);
    }

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = config_.headless ?
        headlessFramebuffer_ : framebuffers_[currentImageIndex_];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent_;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3]}};
    // Reversed-Z: clear depth to 0.0 (far plane), Standard: clear to 1.0 (far plane)
    clearValues[1].depthStencil = {config_.useReversedZ ? 0.0f : 1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<std::uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(currentCommandBuffer_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent_.width);
    viewport.height = static_cast<float>(swapchainExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(currentCommandBuffer_, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent_;
    vkCmdSetScissor(currentCommandBuffer_, 0, 1, &scissor);

    return {};
}

Result<void, VulkanError> VulkanContext::endFrame() {
    vkCmdEndRenderPass(currentCommandBuffer_);

    if (vkEndCommandBuffer(currentCommandBuffer_) != VK_SUCCESS) {
        return std::unexpected(VulkanError::CommandBufferAllocationFailed);
    }

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    if (!config_.headless) {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
    }

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentCommandBuffer_;

    // Use currentImageIndex_ for render finished semaphore (per Vulkan best practices)
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentImageIndex_]};
    if (!config_.headless) {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
    }

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS) {
        return std::unexpected(VulkanError::Unknown);
    }

    // Present (only for non-headless)
    if (!config_.headless) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapchain_};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &currentImageIndex_;

        VkResult result = vkQueuePresentKHR(presentQueue_, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            if (auto recreateResult = recreateSwapchain(); !recreateResult) {
                return recreateResult;
            }
        } else if (result != VK_SUCCESS) {
            return std::unexpected(VulkanError::Unknown);
        }
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;

    // Poll events
    if (window_) {
        glfwPollEvents();
    }

    return {};
}

void VulkanContext::setClearColor(float r, float g, float b, float a) {
    clearColor_ = {r, g, b, a};
}

Result<VulkanBufferHandle, VulkanError> VulkanContext::createBuffer(const VulkanBufferDef& def) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = def.size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Map usage flags
    bufferInfo.usage = 0;
    if (def.usage & VulkanBufferUsage::Vertex) bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (def.usage & VulkanBufferUsage::Index) bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (def.usage & VulkanBufferUsage::Uniform) bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (def.usage & VulkanBufferUsage::Storage) bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (def.usage & VulkanBufferUsage::TransferSrc) bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (def.usage & VulkanBufferUsage::TransferDst) bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (def.hostVisible) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }
    if (def.persistentlyMapped) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    BufferResource resource;
    resource.size = def.size;
    resource.hostVisible = def.hostVisible;
    resource.persistentlyMapped = def.persistentlyMapped;

    VmaAllocationInfo allocResult;
    if (vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo,
                        &resource.buffer, &resource.allocation, &allocResult) != VK_SUCCESS) {
        return std::unexpected(VulkanError::BufferCreationFailed);
    }

    if (def.persistentlyMapped) {
        resource.mappedPtr = allocResult.pMappedData;
    }

    VulkanBufferHandle handle = nextBufferHandle_++;
    buffers_[handle] = resource;

    return handle;
}

void VulkanContext::destroyBuffer(VulkanBufferHandle handle) {
    auto it = buffers_.find(handle);
    if (it != buffers_.end()) {
        vmaDestroyBuffer(allocator_, it->second.buffer, it->second.allocation);
        buffers_.erase(it);
    }
}

void* VulkanContext::mapBuffer(VulkanBufferHandle handle) {
    auto it = buffers_.find(handle);
    if (it == buffers_.end()) return nullptr;

    // Already mapped
    if (it->second.mappedPtr) return it->second.mappedPtr;

    // Can't map GPU-only memory - don't even try (avoids validation errors)
    if (!it->second.hostVisible) return nullptr;

    void* data;
    if (vmaMapMemory(allocator_, it->second.allocation, &data) != VK_SUCCESS) {
        return nullptr;
    }
    it->second.mappedPtr = data;
    return data;
}

void VulkanContext::unmapBuffer(VulkanBufferHandle handle) {
    auto it = buffers_.find(handle);
    if (it != buffers_.end() && it->second.mappedPtr) {
        // Don't unmap persistently mapped buffers - VMA handles them automatically
        if (!it->second.persistentlyMapped) {
            vmaUnmapMemory(allocator_, it->second.allocation);
            it->second.mappedPtr = nullptr;
        }
    }
}

Result<void, VulkanError> VulkanContext::uploadToBuffer(VulkanBufferHandle handle, const void* data, std::size_t size, std::size_t offset) {
    auto it = buffers_.find(handle);
    if (it == buffers_.end()) {
        return std::unexpected(VulkanError::BufferCreationFailed);
    }

    // Try direct mapping first (works for host-visible memory)
    void* mapped = mapBuffer(handle);
    if (mapped) {
        std::memcpy(static_cast<char*>(mapped) + offset, data, size);
        unmapBuffer(handle);
        return {};
    }

    // For GPU-only memory, use staging buffer
    // Use VMA_MEMORY_USAGE_AUTO with HOST_ACCESS flag per VMA 3.x recommendations
    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = size;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocResult;

    if (vmaCreateBuffer(allocator_, &stagingInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, &stagingAllocResult) != VK_SUCCESS) {
        return std::unexpected(VulkanError::BufferCreationFailed);
    }

    // Copy data to staging buffer
    std::memcpy(stagingAllocResult.pMappedData, data, size);

    // Create one-time command buffer for transfer
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = commandPool_;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(device_, &cmdAllocInfo, &cmdBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = offset;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmdBuffer, stagingBuffer, it->second.buffer, 1, &copyRegion);

    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);

    vkFreeCommandBuffers(device_, commandPool_, 1, &cmdBuffer);
    vmaDestroyBuffer(allocator_, stagingBuffer, stagingAllocation);

    return {};
}

Result<void, VulkanError> VulkanContext::uploadToImage(VulkanImageHandle handle, const void* data, std::size_t size) {
    return uploadToImageLayer(handle, data, size, 0);
}

Result<void, VulkanError> VulkanContext::uploadToImageLayer(VulkanImageHandle handle, const void* data, std::size_t size, std::uint32_t layer) {
    auto it = images_.find(handle);
    if (it == images_.end()) {
        return std::unexpected(VulkanError::ImageCreationFailed);
    }

    auto& imageRes = it->second;
    const auto& def = imageRes.def;

    // Create staging buffer
    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = size;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocResult;

    if (vmaCreateBuffer(allocator_, &stagingInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, &stagingAllocResult) != VK_SUCCESS) {
        return std::unexpected(VulkanError::BufferCreationFailed);
    }

    // Copy data to staging buffer
    std::memcpy(stagingAllocResult.pMappedData, data, size);

    // Create one-time command buffer
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = commandPool_;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(device_, &cmdAllocInfo, &cmdBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    // Transition image to transfer destination layout
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = imageRes.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = def.mipLevels;
    barrier.subresourceRange.baseArrayLayer = layer;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {def.width, def.height, def.depth};

    vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, imageRes.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition to shader read layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmdBuffer);

    // Submit and wait
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);

    vkFreeCommandBuffers(device_, commandPool_, 1, &cmdBuffer);
    vmaDestroyBuffer(allocator_, stagingBuffer, stagingAllocation);

    return {};
}

VkImageView VulkanContext::getImageView(VulkanImageHandle handle) const {
    auto it = images_.find(handle);
    if (it != images_.end()) {
        return it->second.view;
    }
    return VK_NULL_HANDLE;
}

VkSampler VulkanContext::getImageSampler(VulkanImageHandle handle) const {
    auto it = images_.find(handle);
    if (it != images_.end()) {
        return it->second.sampler;
    }
    return VK_NULL_HANDLE;
}

Result<VulkanImageHandle, VulkanError> VulkanContext::createImage(const VulkanImageDef& def) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = def.type;
    imageInfo.format = def.format;
    imageInfo.extent.width = def.width;
    imageInfo.extent.height = def.height;
    imageInfo.extent.depth = def.depth;
    imageInfo.mipLevels = def.mipLevels;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Handle cubemap: must have 6 array layers and cube compatible flag
    if (def.isCubemap) {
        imageInfo.arrayLayers = 6;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    } else {
        imageInfo.arrayLayers = def.arrayLayers;
        imageInfo.flags = 0;
    }

    // Map usage flags
    imageInfo.usage = 0;
    auto usage = static_cast<std::uint32_t>(def.usage);
    if (usage & static_cast<std::uint32_t>(VulkanImageUsage::Sampled))
        imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (usage & static_cast<std::uint32_t>(VulkanImageUsage::Storage))
        imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (usage & static_cast<std::uint32_t>(VulkanImageUsage::ColorAttachment))
        imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (usage & static_cast<std::uint32_t>(VulkanImageUsage::DepthStencilAttachment))
        imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (usage & static_cast<std::uint32_t>(VulkanImageUsage::TransferSrc))
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (usage & static_cast<std::uint32_t>(VulkanImageUsage::TransferDst))
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    ImageResource resource;
    resource.def = def;

    if (vmaCreateImage(allocator_, &imageInfo, &allocInfo,
                       &resource.image, &resource.allocation, nullptr) != VK_SUCCESS) {
        return std::unexpected(VulkanError::ImageCreationFailed);
    }

    // Determine aspect mask based on format
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bool isDepthFormat = (def.format == VK_FORMAT_D32_SFLOAT ||
                          def.format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                          def.format == VK_FORMAT_D24_UNORM_S8_UINT ||
                          def.format == VK_FORMAT_D16_UNORM ||
                          def.format == VK_FORMAT_D16_UNORM_S8_UINT);
    bool hasStencil = (def.format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                       def.format == VK_FORMAT_D24_UNORM_S8_UINT ||
                       def.format == VK_FORMAT_D16_UNORM_S8_UINT ||
                       def.format == VK_FORMAT_S8_UINT);

    if (isDepthFormat) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        // Note: For sampling depth/stencil, we only use depth aspect
    } else if (def.format == VK_FORMAT_S8_UINT) {
        aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    // Determine image view type
    VkImageViewType viewType;
    if (def.isCubemap) {
        viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    } else if (imageInfo.arrayLayers > 1) {
        viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    } else {
        viewType = VK_IMAGE_VIEW_TYPE_2D;
    }

    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = resource.image;
    viewInfo.viewType = viewType;
    viewInfo.format = def.format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = def.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = imageInfo.arrayLayers;

    if (vkCreateImageView(device_, &viewInfo, nullptr, &resource.view) != VK_SUCCESS) {
        vmaDestroyImage(allocator_, resource.image, resource.allocation);
        return std::unexpected(VulkanError::ImageCreationFailed);
    }

    // Create sampler for sampled images
    if (imageInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = isDepthFormat ? VK_TRUE : VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(def.mipLevels);

        if (vkCreateSampler(device_, &samplerInfo, nullptr, &resource.sampler) != VK_SUCCESS) {
            vkDestroyImageView(device_, resource.view, nullptr);
            vmaDestroyImage(allocator_, resource.image, resource.allocation);
            return std::unexpected(VulkanError::ImageCreationFailed);
        }
    }

    VulkanImageHandle handle = nextImageHandle_++;
    images_[handle] = resource;

    return handle;
}

void VulkanContext::destroyImage(VulkanImageHandle handle) {
    auto it = images_.find(handle);
    if (it != images_.end()) {
        if (it->second.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device_, it->second.sampler, nullptr);
        }
        vkDestroyImageView(device_, it->second.view, nullptr);
        vmaDestroyImage(allocator_, it->second.image, it->second.allocation);
        images_.erase(it);
    }
}

Result<VulkanPipelineHandle, VulkanError> VulkanContext::createPipeline(const VulkanPipelineDef& def) {
    PipelineResource resource;

    // Convert push constant ranges
    std::vector<VkPushConstantRange> pushConstantRanges;
    for (const auto& range : def.pushConstantRanges) {
        VkPushConstantRange vkRange{};
        vkRange.stageFlags = range.stageFlags;
        vkRange.offset = range.offset;
        vkRange.size = range.size;
        pushConstantRanges.push_back(vkRange);
    }

    // Create pipeline layout with push constants and descriptor set layouts
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<std::uint32_t>(def.descriptorSetLayouts.size());
    layoutInfo.pSetLayouts = def.descriptorSetLayouts.empty() ? nullptr : def.descriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount = static_cast<std::uint32_t>(pushConstantRanges.size());
    layoutInfo.pPushConstantRanges = pushConstantRanges.data();

    if (vkCreatePipelineLayout(device_, &layoutInfo, nullptr, &resource.layout) != VK_SUCCESS) {
        return std::unexpected(VulkanError::PipelineCreationFailed);
    }

    // Create shader modules
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule> shaderModules;

    for (const auto& stage : def.shaderStages) {
        VkShaderModuleCreateInfo moduleInfo{};
        moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.codeSize = stage.spirv.size() * sizeof(std::uint32_t);
        moduleInfo.pCode = stage.spirv.data();

        VkShaderModule module;
        if (vkCreateShaderModule(device_, &moduleInfo, nullptr, &module) != VK_SUCCESS) {
            for (auto m : shaderModules) {
                vkDestroyShaderModule(device_, m, nullptr);
            }
            vkDestroyPipelineLayout(device_, resource.layout, nullptr);
            return std::unexpected(VulkanError::ShaderModuleCreationFailed);
        }
        shaderModules.push_back(module);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = stage.stage;
        stageInfo.module = module;
        stageInfo.pName = stage.entryPoint.c_str();
        shaderStages.push_back(stageInfo);
    }

    // Vertex input
    std::vector<VkVertexInputBindingDescription> bindings;
    for (const auto& b : def.vertexBindings) {
        bindings.push_back({b.binding, b.stride, b.inputRate});
    }

    std::vector<VkVertexInputAttributeDescription> attributes;
    for (const auto& a : def.vertexAttributes) {
        attributes.push_back({a.location, a.binding, a.format, a.offset});
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<std::uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = def.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = def.polygonMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = def.cullMode;
    rasterizer.frontFace = def.frontFace;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = def.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = def.depthWriteEnable ? VK_TRUE : VK_FALSE;

    // For reversed-Z depth buffer, we need to flip the comparison operators
    // LESS becomes GREATER, LESS_OR_EQUAL becomes GREATER_OR_EQUAL, etc.
    VkCompareOp depthOp = def.depthCompareOp;
    if (config_.useReversedZ) {
        switch (def.depthCompareOp) {
            case VK_COMPARE_OP_LESS:
                depthOp = VK_COMPARE_OP_GREATER;
                break;
            case VK_COMPARE_OP_LESS_OR_EQUAL:
                depthOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
                break;
            case VK_COMPARE_OP_GREATER:
                depthOp = VK_COMPARE_OP_LESS;
                break;
            case VK_COMPARE_OP_GREATER_OR_EQUAL:
                depthOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                break;
            default:
                // EQUAL, NOT_EQUAL, ALWAYS, NEVER - unchanged
                break;
        }
    }
    depthStencil.depthCompareOp = depthOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = def.blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = def.srcColorBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = def.dstColorBlendFactor;
    colorBlendAttachment.colorBlendOp = def.colorBlendOp;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<std::uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = resource.layout;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &resource.pipeline) != VK_SUCCESS) {
        for (auto m : shaderModules) {
            vkDestroyShaderModule(device_, m, nullptr);
        }
        vkDestroyPipelineLayout(device_, resource.layout, nullptr);
        return std::unexpected(VulkanError::PipelineCreationFailed);
    }

    // Cleanup shader modules
    for (auto m : shaderModules) {
        vkDestroyShaderModule(device_, m, nullptr);
    }

    VulkanPipelineHandle handle = nextPipelineHandle_++;
    pipelines_[handle] = resource;

    return handle;
}

void VulkanContext::destroyPipeline(VulkanPipelineHandle handle) {
    auto it = pipelines_.find(handle);
    if (it != pipelines_.end()) {
        vkDestroyPipeline(device_, it->second.pipeline, nullptr);
        vkDestroyPipelineLayout(device_, it->second.layout, nullptr);
        pipelines_.erase(it);
    }
}

void VulkanContext::bindPipeline(VulkanPipelineHandle handle) {
    auto it = pipelines_.find(handle);
    if (it != pipelines_.end() && currentCommandBuffer_ != VK_NULL_HANDLE) {
        vkCmdBindPipeline(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, it->second.pipeline);
    }
}

VkPipeline VulkanContext::getPipeline(VulkanPipelineHandle handle) const {
    auto it = pipelines_.find(handle);
    if (it != pipelines_.end()) {
        return it->second.pipeline;
    }
    return VK_NULL_HANDLE;
}

VkPipelineLayout VulkanContext::getPipelineLayout(VulkanPipelineHandle handle) const {
    auto it = pipelines_.find(handle);
    if (it != pipelines_.end()) {
        return it->second.layout;
    }
    return VK_NULL_HANDLE;
}

VkBuffer VulkanContext::getBuffer(VulkanBufferHandle handle) const {
    auto it = buffers_.find(handle);
    if (it != buffers_.end()) {
        return it->second.buffer;
    }
    return VK_NULL_HANDLE;
}

VulkanStats VulkanContext::getStats() const {
    VulkanStats stats;
    stats.bufferCount = static_cast<std::uint32_t>(buffers_.size());
    stats.imageCount = static_cast<std::uint32_t>(images_.size());
    stats.pipelineCount = static_cast<std::uint32_t>(pipelines_.size());

    // Get VMA stats
    VmaTotalStatistics vmaStats;
    vmaCalculateStatistics(allocator_, &vmaStats);
    stats.totalAllocatedMemory = vmaStats.total.statistics.blockBytes;
    stats.usedMemory = vmaStats.total.statistics.allocationBytes;
    stats.allocationCount = vmaStats.total.statistics.allocationCount;

    return stats;
}

Size VulkanContext::getWindowSize() const {
    if (config_.headless) {
        return Size{
            static_cast<int>(swapchainExtent_.width),
            static_cast<int>(swapchainExtent_.height)
        };
    }

    int width, height;
    glfwGetWindowSize(window_, &width, &height);
    return Size{width, height};
}

void VulkanContext::setWindowSize(Size size) {
    if (!config_.headless && window_) {
        glfwSetWindowSize(window_, size.width, size.height);
    }
}

}  // namespace bestow::vulkan
