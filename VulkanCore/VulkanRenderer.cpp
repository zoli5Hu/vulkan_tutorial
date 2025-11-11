/**
 * @file VulkanRenderer.cpp
 * @brief Megvalósítja a VulkanRenderer osztályt, amely a Vulkan renderelési ciklusát kezeli.
 *
 * Ez az osztály felelős a CPU és a GPU közötti szinkronizációért (Fences, Semaphores),
 * a parancspufferek (Command Buffers) rögzítéséért és a képkockák megjelenítésre (Present)
 * történő elküldéséért.
 */
// VulkanCore/VulkanRenderer.cpp
#include "VulkanRenderer.h"
#include <stdexcept>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono> // Időméréshez (animáció)
#include <array>  // VkClearValue tömbhöz

using namespace std;

/**
 * @brief Konstruktor.
 * Inicializálja a context-et nullptr-re és a currentFrame-et 0-ra.
 */
VulkanRenderer::VulkanRenderer() : context(nullptr), currentFrame(0) {
    std::cout << "KACSA RENDERER INICIALIZALVA" << std::endl;
}

/**
 * @brief Destruktor.
 */
VulkanRenderer::~VulkanRenderer() {
}

/**
 * @brief Létrehozza a renderelőhöz szükséges erőforrásokat.
 *
 * @param ctx A Vulkan környezet (eszköz, parancskészlet) mutatója.
 * @param swapchain A swapchain objektum mutatója (szinkronizációhoz).
 */
void VulkanRenderer::create(VulkanContext* ctx, VulkanSwapchain* swapchain) {
    this->context = ctx;
    createCommandBuffers(); // Parancspufferek allokálása
    createSyncObjects(swapchain); // Szemaforok és Kerítések létrehozása
}

/**
 * @brief Felszabadítja a szinkronizációs objektumokat.
 * A parancspuffereket a Command Pool szabadítja fel, amikor a Command Pool-t
 * a VulkanContext::cleanup()-ban megsemmisítik.
 */
void VulkanRenderer::cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(context->getDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(context->getDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(context->getDevice(), inFlightFences[i], nullptr);
    }
}

/**
 * @brief Elvégzi egyetlen képkocka renderelésének teljes folyamatát.
 *
 * Ez a függvény felelős a CPU-GPU szinkronizációért, a parancsok elküldéséért
 * és a kép megjelenítésre való kéréséért.
 *
 * @param swapchain A swapchain objektum mutatója.
 * @param pipeline A grafikus pipeline objektum mutatója.
 * @param cameraPosition A kamera aktuális pozíciója.
 * @param objects A kirajzolandó MeshObject-ek vektora.
 */
void VulkanRenderer::drawFrame(VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPosition, const std::vector<MeshObject*>& objects) {
    // 1. CPU-GPU szinkronizáció: Várakozás a kerítésre (Fence)
    // Megvárja, amíg az 'currentFrame' indexű keret befejeződik a GPU-n,
    // így a CPU nem szalad előre (pl. MAX_FRAMES_IN_FLIGHT képkockánál többel).
    vkWaitForFences(context->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // 2. Kép lekérése a Swapchain-től
    uint32_t imageIndex;
    // Lekéri a következő elérhető kép indexét a swapchain-ből.
    // Jelez az 'imageAvailableSemaphores'-nak, amikor a kép valóban elérhető.
    vkAcquireNextImageKHR(
        context->getDevice(),
        swapchain->getSwapchain(),
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame], // Szemafor, ami jelez, ha a kép elérhető
        VK_NULL_HANDLE,
        &imageIndex
    );

    // 3. Ellenőrzés, hogy a lekért kép még használatban van-e
    // Ha a lekért 'imageIndex'-hez tartozó kép még mindig használatban van egy
    // korábbi (még be nem fejezett) keret által, várakoznia kell arra a kerítésre.
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(context->getDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Tárolja, hogy mostantól az 'currentFrame' kerítés felel ezért a képért.
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // 4. Kerítés (Fence) és Parancspuffer (Command Buffer) visszaállítása
    // Visszaállítja a kerítést "jelzetlen" állapotba, mivel most kezdjük el a munkát.
    vkResetFences(context->getDevice(), 1, &inFlightFences[currentFrame]);

    // Visszaállítja a parancspuffert, hogy újra rögzíthessük a parancsokat.
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    // Újrarögzíti a parancsokat (rajzolás, MVP mátrixok stb.)
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, swapchain, pipeline, cameraPosition, objects);

    // 5. Parancsok elküldése (Submit) a GPU-nak
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Várakozás a szemaforra: Várja meg az 'imageAvailableSemaphores'-t (amit a vkAcquireNextImageKHR jelez).
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    // Pipeline fázisok, ahol a várakozás történik (szín írása előtt).
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
    };

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    // Jelzés a szemafornak: Jelez a 'renderFinishedSemaphores'-nak, amikor a renderelés befejeződött.
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Parancs elküldése a grafikus várólistára.
    // Amikor ez a parancs befejeződik, jelezni fogja az 'inFlightFences[currentFrame]'-t.
    if (vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // 6. Kép megjelenítése (Present)
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // Várakozás a szemaforra: Várja meg a 'renderFinishedSemaphores'-t (amit a vkQueueSubmit jelez).
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchain->getSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex; // A megjelenítendő kép indexe.

    // Kérés a kép megjelenítésére.
    vkQueuePresentKHR(context->getPresentQueue(), &presentInfo);

    // 7. Ugrás a következő keret indexére
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/**
 * @brief Allokálja a parancspuffereket (Command Buffers).
 * Létrehoz egy parancspuffert minden egyes 'MAX_FRAMES_IN_FLIGHT' kerethez.
 */
void VulkanRenderer::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Fő parancspuffer
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(context->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

/**
 * @brief Létrehozza a CPU-GPU szinkronizációs objektumokat.
 *
 * Szemaforok (Semaphores): GPU-GPU szinkronizációra (pl. kép lekérése -> renderelés -> megjelenítés).
 * Kerítések (Fences): CPU-GPU szinkronizációra (pl. CPU megvárja a GPU-t).
 *
 * @param swapchain A swapchain (a 'imagesInFlight' tömb méretéhez).
 */
void VulkanRenderer::createSyncObjects(VulkanSwapchain* swapchain)
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    // 'imagesInFlight' tömb: Nyomon követi, hogy melyik swapchain képet melyik kerítés használja.
    imagesInFlight.resize(swapchain->getImageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // A kerítések "jelzett" állapotban jönnek létre, hogy az első 'drawFrame' hívás
    // 'vkWaitForFences' része ne blokkoljon azonnal.
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

/**
 * @brief Rögzíti a rajzolási parancsokat a megadott parancspufferbe.
 *
 * Ez a függvény minden keretben lefut, hogy az aktuális állapotot (pl. MVP mátrix)
 * rögzítse a parancspufferbe.
 *
 * @param commandBuffer A parancspuffer, amelybe a rögzítés történik.
 * @param imageIndex A swapchain képindex, amelyhez a framebuffer tartozik.
 * @param swapchain A swapchain (extent lekéréséhez).
 * @param pipeline A pipeline (Render Pass, Layout és Pipeline-ok lekéréséhez).
 * @param cameraPosition A kamera pozíciója.
 * @param objects A kirajzolandó objektumok.
 */
void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPosition, const std::vector<MeshObject*>& objects) {

    // 1. Parancspuffer rögzítésének indítása
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkExtent2D extent = swapchain->getExtent();

    // 2. Dinamikus Állapotok Beállítása: Viewport és Scissor
    // (Ezeket a pipeline-ban dinamikusnak jelöltük)
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

    // 3. Render Pass Indítása
    // Törlési értékek: Fekete háttér (szín) és 1.0f (legnagyobb) mélység.
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipeline->getRenderPass();
    renderPassInfo.framebuffer = pipeline->getFramebuffer(imageIndex); // Az aktuális swapchain képhez tartozó framebuffer
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 4. Alapértelmezett (kitöltött) Pipeline kötése
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getGraphicsPipeline());

    // 5. MVP (Model-View-Projection) Mátrixok Kiszámítása
    // Idő kiszámítása az animációhoz
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Projection Mátrix (Perspektíva)
    const float aspectRatio = (float)extent.width / (float)extent.height;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

    // View Mátrix (Kamera)
    // A kamera a Z+ irányba néz (a Vulkan alapértelmezett koordináta-rendszere miatt),
    // és az Y tengely lefelé mutat (ezért up = -1.0f).
    glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 center = cameraPosition + cameraDirection;
    glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(cameraPosition, center, up);

    // Kombinált View-Projection Mátrix
    glm::mat4 viewProjection = projection * view;

    // 6. Objektumok kirajzolása (Ciklus)
    for (size_t i = 0; i < objects.size(); ++i) {
        const auto& object = objects[i];

        // Pipeline váltás: Ha az objektum indexe 1 (a 'cube' a main.cpp-ben),
        // akkor átváltunk a drótvázas (wireframe) pipeline-ra.
        if (i == 1) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getWireframePipeline());
        } else {
            // Egyébként a normál (kitöltött) pipeline-t használjuk.
            // (Ez azért kell, mert az 'if (i==1)' ág után vissza kell váltani)
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getGraphicsPipeline());
        }

        // Objektum kirajzolása: Az objektum felelős a saját Model mátrixának
        // kiszámításáért és a Push Konstans feltöltéséért.
        object->draw(commandBuffer, pipeline->getPipelineLayout(), time, viewProjection, i == 1);
    }

    // 7. Render Pass Befejezése
    vkCmdEndRenderPass(commandBuffer);

    // 8. Parancspuffer Rögzítésének Befejezése
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}