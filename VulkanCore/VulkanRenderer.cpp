// VulkanCore/VulkanRenderer.cpp
#include "VulkanRenderer.h"
#include <stdexcept> // A std::runtime_error használatához

using namespace std;

VulkanRenderer::VulkanRenderer() : context(nullptr), currentFrame(0) {
    // Konstruktor
    std::cout << "KACSA RENDERER INICIALIZALVA" << std::endl; // <-- 2. ADD HOZZÁ EZT A SORT
}

VulkanRenderer::~VulkanRenderer() {
    // Destruktor
}

void VulkanRenderer::create(VulkanContext* ctx, VulkanSwapchain* swapchain) {
    this->context = ctx;

    // Létrehozzuk a parancspuffereket (egyet minden "frame in flight"-hoz)
    createCommandBuffers();

    // Létrehozzuk a szinkronizációs objektumokat (2 db-ot minden típusból)
    createSyncObjects(swapchain);
}

void VulkanRenderer::cleanup() {
    // Felszabadítjuk az összes szinkronizációs objektumot
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(context->getDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(context->getDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(context->getDevice(), inFlightFences[i], nullptr);
    }

    // A parancspuffereket nem kell 'free'-elni,
    // mert a 'commandPool' (ami a VulkanContext-é) törlésekor automatikusan felszabadulnak.
}

// --- Fő Rajzoló Függvény ---

// Ez a függvény veszi át a 'drawFrame' teljes logikáját a main.cpp-ből
void VulkanRenderer::drawFrame(VulkanSwapchain* swapchain, VulkanPipeline* pipeline) {

    // 1. VÁRAKOZÁS A FENCE-RE (CPU oldali várakozás)
    // Várunk, amíg a GPU befejezi azt a képkockát, ami az 'inFlightFences[currentFrame]'
    // szinkronizációs készletet használta.
    vkWaitForFences(context->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // 2. KÖVETKEZŐ KÉP LEKÉRÉSE A SWAPCHAIN-TŐL
    uint32_t imageIndex;
    // Megkérjük a swapchain-t, hogy adjon egy képet.
    // Ha a kép elérhetővé válik, jelezze az 'imageAvailableSemaphores[currentFrame]'-t.
    vkAcquireNextImageKHR(
        context->getDevice(),
        swapchain->getSwapchain(),
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame], // 🔹 JELZENDŐ SZEMAFOR
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        // Ha igen, várunk arra a fence-re (ami egy *másik* frame-hez tartozhat)
        vkWaitForFences(context->getDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Most már biztosan szabad a kép, társítjuk az *aktuális* frame fence-éhez
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];


    // Miután megkaptuk a kép indexét, reseteljük a CPU-oldali fence-t.
    // Most már biztonságosan megtehetjük, mert tudjuk, hogy a GPU végzett az előző
    // munkával (a vkWaitForFences miatt), és újra beküldhetünk egy parancsot.
    vkResetFences(context->getDevice(), 1, &inFlightFences[currentFrame]);

    // 3. PARANCSPUFFER RÖGZÍTÉSE
    // Reseteljük az aktuális 'frame-in-flight'-hez tartozó parancspuffert
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    // Újrarögzítjük a parancsokat (háromszög rajzolása)
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, swapchain, pipeline);

    // 4. PARANCS BEKÜLDÉSE (SUBMIT) A GPU-NAK
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Megmondjuk, melyik szemaforra VÁRJON, mielőtt elkezdi a rajzolást
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // Megmondjuk, melyik parancspuffert futtassa
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame]; // Az aktuális frame parancspuffere

    // Megmondjuk, melyik szemafor-t JELEZZE (signal), ha végzett a rajzolással
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Beküldjük a parancsot a grafikus queue-ra, és megmondjuk,
    // hogy az 'inFlightFences[currentFrame]'-t is jelezze,
    // hogy a CPU tudja, mikor használhatja újra ezt a 'frame-in-flight' indexet.
    if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // 5. PREZENTÁCIÓ
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // Megmondjuk, hogy a prezentálás VÁRJON arra a szemaforra,
    // ami a rajzolás végét jelzi
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // 🔹 VÁRAKOZÁS ERRE

    VkSwapchainKHR swapChains[] = {swapchain->getSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex; // A kép indexe, amit prezentálni kell

    vkQueuePresentKHR(context->getPresentQueue(), &presentInfo);

    // 6. LÉPTETÉS A KÖVETKEZŐ FRAME-RE
    // Váltunk a másik "kulcs-készletre" (0 -> 1 vagy 1 -> 0)
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


// --- Privát segédfüggvények (áthelyezve a main.cpp-ből) ---

// JAVÍTVA: A parancspufferek létrehozása
void VulkanRenderer::createCommandBuffers() {
    // A parancspuffer vektor méretét beállítjuk MAX_FRAMES_IN_FLIGHT-ra (2)
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size(); // Az összeset egyszerre allokáljuk

    if (vkAllocateCommandBuffers(context->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

// JAVÍTVA: A szinkronizációs objektumok létrehozása
void VulkanRenderer::createSyncObjects(VulkanSwapchain* swapchain)
{
    // Átméretezzük a vektorokat MAX_FRAMES_IN_FLIGHT méretűre (2)
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    imagesInFlight.resize(swapchain->getImageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Fontos: Azonnal "jelzett" állapotban hozzuk létre

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

// ÁTHELYEZVE: A parancsok rögzítése
void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VulkanSwapchain* swapchain, VulkanPipeline* pipeline) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkExtent2D extent = swapchain->getExtent();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipeline->getRenderPass();
    renderPassInfo.framebuffer = pipeline->getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getGraphicsPipeline());

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

    // Rajzoljuk a hard-coded háromszöget
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}