// VulkanCore/VulkanRenderer.cpp
#include "VulkanRenderer.h"
#include <stdexcept>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <array>

using namespace std;

VulkanRenderer::VulkanRenderer() : context(nullptr), currentFrame(0) {
    std::cout << "KACSA RENDERER INICIALIZALVA" << std::endl;
}

VulkanRenderer::~VulkanRenderer() {
    // Destruktor
}

void VulkanRenderer::create(VulkanContext* ctx, VulkanSwapchain* swapchain) {
    this->context = ctx;
    createCommandBuffers();
    createSyncObjects(swapchain);
}

void VulkanRenderer::cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(context->getDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(context->getDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(context->getDevice(), inFlightFences[i], nullptr);
    }
}

// JAVÍTVA: Szignatúra frissítve (eltűnt a VkBuffer)
void VulkanRenderer::drawFrame(VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPosition, const std::vector<MeshObject*>& objects) {
    vkWaitForFences(context->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(
        context->getDevice(),
        swapchain->getSwapchain(),
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(context->getDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    vkResetFences(context->getDevice(), 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    // JAVÍTVA: Hívás frissítve
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, swapchain, pipeline, cameraPosition, objects);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
    };

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchain->getSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(context->getPresentQueue(), &presentInfo);
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(context->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void VulkanRenderer::createSyncObjects(VulkanSwapchain* swapchain)
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapchain->getImageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

// JAVÍTVA: Szignatúra frissítve + rajzolási logika behelyezve a MeshObject.draw()-ba
void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPosition, const std::vector<MeshObject*>& objects) {

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkExtent2D extent = swapchain->getExtent();

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

    // 1. Definiáljuk a törlési értékeket (Color és Depth)
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipeline->getRenderPass();
    renderPassInfo.framebuffer = pipeline->getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Alapértelmezett pipeline kötése (FILL)
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getGraphicsPipeline());

    // 1. Időalapú forgás kiszámítása
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // --- VP (View Projection) SZÁMÍTÁS ---

    // Projection (Vetorítés)
    const float aspectRatio = (float)extent.width / (float)extent.height;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    // View (Kamera)
    glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 center = cameraPosition + cameraDirection;
    glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(cameraPosition, center, up);

    glm::mat4 viewProjection = projection * view;


    // --- OBJEKTUMOK RAJZOLÁSA CIKLUSBAN ---
    for (size_t i = 0; i < objects.size(); ++i) {
        const auto& object = objects[i];

        // CUBE-hoz (index 1) megkötjük a Wireframe Pipeline-t
        if (i == 1) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getWireframePipeline());
        } else {
            // A többihez (torus, pyramid) a normál pipeline-t kötjük
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getGraphicsPipeline());
        }

        // A MeshObject végzi a Push Constant feltöltést, buffer kötést és vkCmdDraw hívást
        object->draw(commandBuffer, pipeline->getPipelineLayout(), time, viewProjection, i == 1);
    }
    // --- CIKLUS VÉGE ---




    // Eltávolítva a régi vkCmdDraw hívás, azt már a MeshObject.draw() csinálja
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}