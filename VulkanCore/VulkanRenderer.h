/**
 * @file VulkanRenderer.h
 * @brief Deklarálja a VulkanRenderer osztályt, amely a Vulkan renderelési ciklus fő logikáját tartalmazza.
 *
 * Ez az osztály felelős a parancspufferek felvételéért, a szinkronizációs objektumok kezeléséért
 * (szemaforok, kerítések) és egy keret kirajzolásának kezdeményezéséért.
 */
// VulkanCore/VulkanRenderer.h
#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "MeshObject.h"
#include <vector>
#include <glm/glm.hpp> // GLM a kamera pozíciójának (vec3) kezeléséhez

/**
 * @class VulkanRenderer
 * @brief A renderelési ciklust és a CPU-GPU szinkronizációt kezelő osztály.
 */
class VulkanRenderer {
public:
    /**
     * @brief Konstruktor.
     */
    VulkanRenderer();

    /**
     * @brief Destruktor.
     */
    ~VulkanRenderer();

    /**
     * @brief Konstans: A maximálisan egyszerre futtatható (in-flight) képkockák száma.
     * Ez biztosítja, hogy a CPU legfeljebb ennyi képkockával járjon a GPU előtt,
     * elkerülve az erőforrások túlterhelését.
     */
    const int MAX_FRAMES_IN_FLIGHT = 3;

    /**
     * @brief Létrehozza a renderelőhöz szükséges erőforrásokat (parancspufferek, szinkronizációs objektumok).
     *
     * @param context A Vulkan környezet (eszköz, várólista, parancskészlet) mutatója.
     * @param swapchain A swapchain (képcsere lánc) objektum mutatója (a szinkronizációs objektumok méretéhez).
     */
    void create(VulkanContext* context, VulkanSwapchain* swapchain);

    /**
     * @brief Felszabadítja az osztály által lefoglalt Vulkan erőforrásokat (szemaforok, kerítések).
     */
    void cleanup();

    /**
     * @brief Elvégzi egyetlen képkocka renderelésének teljes folyamatát.
     *
     * Vár a megfelelő kerítésre, lekéri a következő swapchain képet, újra felveszi a parancspuffert,
     * elküldi a rajzparancsokat a GPU-nak, és elindítja a kép megjelenítését (present).
     *
     * @param swapchain A swapchain objektum mutatója.
     * @param pipeline A grafikus pipeline objektum mutatója (render pass, pipeline layout).
     * @param cameraPosition A kamera aktuális pozíciója a nézeti mátrix kiszámításához.
     * @param objects A kirajzolandó MeshObject-ek vektorreferenciája.
     */
    void drawFrame(VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPosition, const std::vector<MeshObject*>& objects);
private:
    /**
     * @brief Parancspufferek tárolója (VkCommandBuffer).
     * Egy parancspuffer minden egyes 'MAX_FRAMES_IN_FLIGHT' kerethez.
     */
    std::vector<VkCommandBuffer> commandBuffers;

    /**
     * @brief Szemaforok a kép elérésének jelzésére (imageAvailableSemaphores).
     * Jelez, amikor egy swapchain kép elkészült és elérhető a renderelésre.
     */
    std::vector<VkSemaphore> imageAvailableSemaphores;

    /**
     * @brief Szemaforok a renderelés befejezésének jelzésére (renderFinishedSemaphores).
     * Jelez, amikor a renderelés befejeződött, és a kép készen áll a megjelenítésre (present).
     */
    std::vector<VkSemaphore> renderFinishedSemaphores;

    /**
     * @brief Kerítések (Fences) a CPU és GPU szinkronizálására.
     * Lehetővé teszi, hogy a CPU megvárja a korábbi képkockák befejezését.
     * Minden 'in-flight' kerethez tartozik egy kerítés.
     */
    std::vector<VkFence> inFlightFences;

    /**
     * @brief A swapchain képek szinkronizációs állapotának nyomon követésére szolgáló tömb.
     * Minden swapchain képhez hozzárendel egy 'in-flight' kerítést, jelezve, hogy az adott képkocka
     * éppen melyik 'in-flight' keret által van használatban.
     */
    std::vector<VkFence> imagesInFlight;

    /**
     * @brief Jelenleg futó keret indexe.
     * Ezt a változót moduló aritmetikával léptetjük (0-tól MAX_FRAMES_IN_FLIGHT - 1-ig).
     */
    uint32_t currentFrame = 0;

    /**
     * @brief Mutató a Vulkan környezet objektumra.
     */
    VulkanContext* context;

    /**
     * @brief Létrehozza a parancspuffereket (VkCommandBuffer).
     * A parancspufferek számát a MAX_FRAMES_IN_FLIGHT határozza meg.
     */
    void createCommandBuffers();

    /**
     * @brief Létrehozza a képkocka-szinkronizációs objektumokat (szemaforok, kerítések).
     * Szinkronizációs objektum minden egyes MAX_FRAMES_IN_FLIGHT kerethez.
     * @param swapchain A swapchain objektum mutatója, a 'imagesInFlight' tömb méretéhez.
     */
    void createSyncObjects(VulkanSwapchain* swapchain);

    /**
     * @brief Felveszi a rajzolási parancsokat a megadott parancspufferbe.
     * Ez tartalmazza a render pass indítását, a pipeline kötését, a push konstansok beállítását
     * és az összes MeshObject rajzolási parancsát.
     *
     * @param commandBuffer A parancspuffer, amelybe a parancsok rögzítésre kerülnek.
     * @param imageIndex A swapchain azon indexe, amelyhez a parancspuffer felvételre kerül.
     * @param swapchain A swapchain objektum mutatója (extent lekéréséhez).
     * @param pipeline A pipeline objektum mutatója (render pass, pipeline binding).
     * @param cameraPosition A kamera pozíciója.
     * @param objects A kirajzolandó MeshObject-ek vektorreferenciája.
     */
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VulkanSwapchain* swapchain, VulkanPipeline* pipeline, glm::vec3 cameraPosition, const std::vector<MeshObject*>& objects);
};