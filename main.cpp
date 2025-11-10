#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <GLFW/glfw3native.h>

// 1. BEILLESZTJÜK AZ OSZTÁLYAINKAT
#include "VulkanCore/VulkanContext.h"
#include "VulkanCore/VulkanSwapchain.h"
#include "VulkanCore/VulkanPipeline.h" // <-- ÚJ

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
using namespace std;

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    // --- Core Komponensek ---
    VulkanContext vulkanContext;
    VulkanSwapchain vulkanSwapchain;
    VulkanPipeline vulkanPipeline; // <-- ÚJ OBJEKTUM

    // --- Alkalmazás-specifikus objektumok (MARADNAK) ---
    VkSurfaceKHR surface;
    VkCommandBuffer commandBuffer;

    // Szinkronizáció (Ezt is ki fogjuk szervezni, de egyelőre marad)
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    // TÖRÖLVE: renderPass, pipelineLayout, graphicsPipeline, swapChainFramebuffers

    void initVulkan()
    {
        // 1. A "MAG" INICIALIZÁLÁSA
        vulkanContext.initVulkan(window);

        // 2. LÉTREHOZZUK A SURFACE-T
        if (glfwCreateWindowSurface(vulkanContext.getInstance(), window, nullptr, &surface) != VK_SUCCESS)
        {
            throw runtime_error("failed to create window surface!");
        }

        // 3. LÉTREHOZZUK A SWAPCHAIN-T
        vulkanSwapchain.create(&vulkanContext, surface, window);

        // 4. LÉTREHOZZUK A PIPELINE-T (az új osztály segítségével)
        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain);

        // 5. AZ ALKALMAZÁS-SPECIFIKUS RÉSZEK INICIALIZÁLÁSA
        createCommandBuffer();
        createSyncObjects();
    }

    // TÖRÖLVE: Az összes Pipeline-hoz, RenderPass-hoz, Framebuffer-hez
    // és Shader-beolvasáshoz kapcsolódó függvény
    // (createRenderPass, createGraphicsPipeline, createFramebuffers, readFile, createShaderModule)

    // --- Ezek a segédfüggvények MARADNAK (de frissítve) ---

    void createCommandBuffer() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = vulkanContext.getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(vulkanContext.getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // A méretet a swapchain-től kérjük
        VkExtent2D extent = vulkanSwapchain.getExtent();

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

        // JAVÍTVA: A Pipeline osztálytól kérjük az adatokat
        renderPassInfo.renderPass = vulkanPipeline.getRenderPass();
        renderPassInfo.framebuffer = vulkanPipeline.getFramebuffer(imageIndex);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // JAVÍTVA: A Pipeline osztálytól kérjük az adatokat
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline.getGraphicsPipeline());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(vulkanContext.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(vulkanContext.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(vulkanContext.getDevice(), &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create semaphores!");
        }
    }

    void drawFrame() {
        vkWaitForFences(vulkanContext.getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vulkanContext.getDevice(), 1, &inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(
            vulkanContext.getDevice(),
            vulkanSwapchain.getSwapchain(),
            UINT64_MAX,
            imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex
        );

        vkResetCommandBuffer(commandBuffer, 0);
        recordCommandBuffer(commandBuffer, imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(vulkanContext.getGraphicsQueue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {vulkanSwapchain.getSwapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(vulkanContext.getPresentQueue(), &presentInfo);
    }

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(vulkanContext.getDevice());
    }

    void cleanup()
    {
        // 1. Az alkalmazás-specifikus szinkronizációs objektumok törlése
        vkDestroySemaphore(vulkanContext.getDevice(), imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(vulkanContext.getDevice(), renderFinishedSemaphore, nullptr);
        vkDestroyFence(vulkanContext.getDevice(), inFlightFence, nullptr);

        // TÖRÖLVE: Framebuffer, Pipeline, Layout, RenderPass törlése

        // 2. ÚJ: A VulkanPipeline takarítása
        // (ez törli: Framebuffer, Pipeline, Layout, RenderPass)
        vulkanPipeline.cleanup();

        // 3. A VulkanSwapchain takarítása
        // (ez törli: Swapchain, ImageViews)
        vulkanSwapchain.cleanup();

        // 4. FONTOS: A Surface törlése
        vkDestroySurfaceKHR(vulkanContext.getInstance(), surface, nullptr);

        // 5. A VÉGÉN hívjuk meg a Context saját takarítóját
        // (ez törli: Instance, Device, CommandPool, Debugger)
        vulkanContext.cleanup();

        // 6. Végül a GLFW takarítása
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}