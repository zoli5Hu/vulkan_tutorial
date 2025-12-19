/**
 * @file MeshObject.h
 * @brief Egyedi 3D objektum reprezentációja Vulkan alatt.
 * Tartalmazza a transzformációs adatokat és a GPU-oldali vertex buffer referenciákat.
 */
#pragma once

#include "VulkanContext.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class MeshObject {
public:
    MeshObject();
    ~MeshObject();

    /**
     * @brief A shaderben használt textúra-erőforrás (Descriptor Set) hozzárendelése az objektumhoz.
     * @param descSet Az előre lefoglalt és frissített Vulkan Descriptor Set.
     */
    void setTexture(VkDescriptorSet descSet);

    /**
     * @brief Inicializálja a vertex puffert.
     * Kiszámítja a tangenseket, így a bemeneti 8 float/vertex adatot 11 float/vertexre bővíti.
     * @param context A Vulkan környezet (buffer létrehozáshoz és másoláshoz).
     * @param vertices Eredeti vertex adatok (Pos, Norm, UV).
     */
    void create(VulkanContext* context, const std::vector<float>& vertices);

    /**
     * @brief Felszabadítja a GPU memóriát (Vertex Buffer és Memory).
     * @param device A logikai eszköz, amihez az erőforrások tartoznak.
     */
    void cleanup(VkDevice device);

    /**
     * @brief Rögzíti a rajzolási parancsokat a parancspufferbe.
     * @param commandBuffer Aktuális Vulkan Command Buffer.
     * @param pipelineLayout A használt pipeline elrendezése (Push Constants és Descriptor kérésekhez).
     * @param viewProjection A kamera View * Projection mátrixa.
     * @param animationTime Időbélyeg a forgatási animációhoz.
     */
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, glm::mat4 viewProjection, float animationTime) const;

    // --- Publikus változók (Közvetlen elérés az egyszerűség és teljesítmény érdekében) ---

    // Transzformációs adatok: Világbeli pozíció és forgási paraméterek
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    float rotationSpeed = 0.0f;

    // Vulkan Erőforrások: Csak a publikus handle-ök a rajzoláshoz
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    uint32_t vertexCount = 0; // Az indexek nélküli rajzoláshoz szükséges vertex szám

private:
    // Belső erőforrás-kezelés: A memóriát csak ez az osztály kezelheti
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;
    VulkanContext* context = nullptr;
};