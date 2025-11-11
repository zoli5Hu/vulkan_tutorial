// VulkanCore/VulkanRenderer.h
#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "MeshObject.h"
#include <vector>
#include <glm/glm.hpp>

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    const int MAX_FRAMES_IN_FLIGHT = 2;

    void create(VulkanContext* context, VulkanSwapchain* swapchain);
    void cleanup();

    void drawFrame(VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPosition, const std::vector<MeshObject*>& objects);
private:
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    uint32_t currentFrame = 0;
    VulkanContext* context;

    void createCommandBuffers();
    void createSyncObjects(VulkanSwapchain* swapchain);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPosition, const std::vector<MeshObject*>& objects);
};