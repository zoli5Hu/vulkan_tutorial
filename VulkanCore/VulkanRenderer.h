/**
 * @file VulkanRenderer.h
 * @brief Deklarálja a VulkanRenderer osztályt, kiegészítve az Árnyék (Shadow Mapping) támogatással.
 * Ez az osztály koordinálja a CPU és GPU közötti munkamenetet.
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

    /**
     * @brief Inicializálja a renderelőt, a szinkronizációs objektumokat és az árnyékoló rendszert.
     */
    void create(VulkanContext* ctx, VulkanSwapchain* swapchain);

    /**
     * @brief Felszabadítja a rendererhez tartozó összes GPU erőforrást.
     */
    void cleanup();

    /**
     * @brief Egy teljes képkocka lerenderelése (Shadow Pass + Main Pass).
     * @param cameraPos A nézőpont pozíciója a jelenetben.
     * @param objects A kirajzolni kívánt 3D objektumok listája.
     */
    void drawFrame(VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPos, const std::vector<MeshObject*>& objects);

    // Getterek az árnyékhoz, hogy a grafikai pipeline össze tudja kapcsolni az erőforrásokat
    VkDescriptorSetLayout getShadowDescriptorSetLayout() const { return shadowDescriptorSetLayout; }
    VkDescriptorSet getShadowDescriptorSet() const { return shadowDescriptorSet; }

private:
    VulkanContext* context; // Referencia a Vulkan környezetre

    // --- Szinkronizálás (Frame-ek kezelése) ---
    // Meghatározza, hány képkocka lehet egyszerre feldolgozás alatt a GPU-n (Double/Triple buffering)
    static const int MAX_FRAMES_IN_FLIGHT = 3;
    std::vector<VkSemaphore> imageAvailableSemaphores; // Jelzi, ha a swapchain kép készen áll
    std::vector<VkSemaphore> renderFinishedSemaphores; // Jelzi, ha a renderelés befejeződött
    std::vector<VkFence> inFlightFences;               // CPU-GPU szinkronizációhoz
    uint32_t currentFrame = 0;                         // Az aktuális frame indexe

    std::vector<VkCommandBuffer> commandBuffers;       // Parancspufferek a GPU parancsok rögzítéséhez

    void createCommandBuffers();                       // Parancspufferek lefoglalása
    void createSyncObjects(VulkanSwapchain* swapchain); // Szinkronizációs eszközök létrehozása

    // --- ÁRNYÉK (SHADOW MAPPING) RENDSZER ---

    // Az árnyéktérkép felbontása (2048x2048)
    const uint32_t shadowMapWidth = 2048;
    const uint32_t shadowMapHeight = 2048;

    // GPU erőforrások az árnyéktérkép tárolásához
    VkImage shadowImage = VK_NULL_HANDLE;              // A nyers képobjektum
    VkDeviceMemory shadowImageMemory = VK_NULL_HANDLE; // A képhez rendelt GPU memória
    VkImageView shadowImageView = VK_NULL_HANDLE;      // Hogyan érjük el a képet (View)
    VkSampler shadowSampler = VK_NULL_HANDLE;          // Hogyan mintavételezzük a textúrát

    // Árnyék-specifikus Render Pass és Framebuffer
    VkRenderPass shadowRenderPass = VK_NULL_HANDLE;    // A mélységi adatok írásának logikája
    VkFramebuffer shadowFramebuffer = VK_NULL_HANDLE;  // A célpuffer az árnyék-rendereléshez

    // Pipeline az árnyék generáláshoz (csak mélységi adatokat dolgoz fel)
    VkPipeline shadowPipeline = VK_NULL_HANDLE;
    VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;

    // Descriptor Set: Az árnyéktérkép textúraként való elérése a fő shaderben
    VkDescriptorSetLayout shadowDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool shadowDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet shadowDescriptorSet = VK_NULL_HANDLE;

    // --- Segédfüggvények az inicializáláshoz ---
    void createShadowResources();      // Kép és Sampler létrehozása
    void createShadowRenderPass();     // Render Pass definíció a mélységíráshoz
    void createShadowFramebuffer();    // Framebuffer összeállítása
    void createShadowPipeline();       // Speciális pipeline (csak vertex shader)
    void createShadowDescriptorSet();  // Az árnyéktérkép regisztrálása a shaderek felé

    /**
     * @brief Kiszámítja a fényforrás szemszögéből használt nézeti és vetítési mátrixot.
     * @param lightPos A fényforrás pozíciója a világban.
     */
    glm::mat4 getLightSpaceMatrix(glm::vec3 lightPos);
};