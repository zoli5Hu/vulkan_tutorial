// VulkanCore/VulkanRenderer.h
#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h" // Szükségünk van a pipeline-ra a parancs rögzítéséhez
#include <vector> // A std::vector használatához
#include <glm/glm.hpp> // <- Hozzáadva a glm::vec3 használatához

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    const int MAX_FRAMES_IN_FLIGHT = 2;

    void create(VulkanContext* context, VulkanSwapchain* swapchain);
    void cleanup();

    // MÓDOSÍTVA: A drawFrame már átveszi a vertex buffert és a kamera pozíciót
    void drawFrame(VulkanSwapchain* swapchain, VulkanPipeline* pipeline, VkBuffer vertexBuffer, glm::vec3 cameraPosition);
private:
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    uint32_t currentFrame = 0;
    VulkanContext* context;

    // MÓDOSÍTVA: Csúcspontszám (a torushoz)
    const uint32_t cubeVertexCount = 1728;

    void createCommandBuffers();
    void createSyncObjects(VulkanSwapchain* swapchain);

    // MÓDOSÍTVA: recordCommandBuffer is átveszi
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VulkanSwapchain* swapchain, VulkanPipeline* pipeline, VkBuffer vertexBuffer, glm::vec3 cameraPosition);
};