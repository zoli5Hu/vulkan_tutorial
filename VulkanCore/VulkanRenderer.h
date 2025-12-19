/**
 * @file VulkanRenderer.h
 * @brief Deklarálja a VulkanRenderer osztályt, kiegészítve az Árnyék (Shadow Mapping) támogatással.
 */
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

    void create(VulkanContext* ctx, VulkanSwapchain* swapchain);
    void cleanup();

    // Frissített rajzoló függvény
    void drawFrame(VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPos, const std::vector<MeshObject*>& objects);

    // Getterek az árnyékhoz (hogy a Pipeline be tudja kötni)
    VkDescriptorSetLayout getShadowDescriptorSetLayout() const { return shadowDescriptorSetLayout; }
    VkDescriptorSet getShadowDescriptorSet() const { return shadowDescriptorSet; }

private:


    VulkanContext* context;

    // --- Szinkronizálás (Frame-ek kezelése) ---
    static const int MAX_FRAMES_IN_FLIGHT = 3;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    std::vector<VkCommandBuffer> commandBuffers;
    void createCommandBuffers();
    void createSyncObjects(VulkanSwapchain* swapchain);

    // --- ÁRNYÉK (SHADOW MAPPING) RENDSZER ---

    // Az árnyéktérkép felbontása (minél nagyobb, annál élesebb az árnyék)
    const uint32_t shadowMapWidth = 2048;
    const uint32_t shadowMapHeight = 2048;

    // Erőforrások
    VkImage shadowImage = VK_NULL_HANDLE;
    VkDeviceMemory shadowImageMemory = VK_NULL_HANDLE;
    VkImageView shadowImageView = VK_NULL_HANDLE;
    VkSampler shadowSampler = VK_NULL_HANDLE;

    // Render Pass és Framebuffer (külön a képernyőtől)
    VkRenderPass shadowRenderPass = VK_NULL_HANDLE;
    VkFramebuffer shadowFramebuffer = VK_NULL_HANDLE;

    // Pipeline az árnyék generáláshoz
    VkPipeline shadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;

    // Descriptor Set: Ezen keresztül olvassa majd a fő shader az árnyéktérképet
    VkDescriptorSetLayout shadowDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool shadowDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet shadowDescriptorSet = VK_NULL_HANDLE;

    // --- Segédfüggvények az inicializáláshoz ---
    void createShadowResources();      // Kép és Sampler létrehozása
    void createShadowRenderPass();     // Render Pass a mélységhez
    void createShadowFramebuffer();    // Framebuffer csatolása
    void createShadowPipeline();       // Pipeline (csak vertex shader)
    void createShadowDescriptorSet();  // Bekötés a fő shaderhez

    // Fény szemszögének kiszámítása
    glm::mat4 getLightSpaceMatrix(glm::vec3 lightPos);
};