/**
 * @file MeshObject.h
 * @brief Deklarálja a MeshObject osztályt, amely egy egyedi 3D modellt képvisel a Vulkan környezetben.
 *
 * Ez az osztály tartalmazza az objektum geometriáját (vertex puffer) és a világtérbeli
 * transzformációs állapotát (pozíció, forgatás).
 */
#pragma once

#include "VulkanContext.h"
#include <vector>
#include <glm/glm.hpp> // GLM fő header a vektorokhoz és mátrixokhoz
#include <glm/gtc/matrix_transform.hpp> // GLM segédfüggvények a transzformációkhoz (translate, rotate)

/**
 * @class MeshObject
 * @brief Egy 3D háló (mesh) objektumot kezel, beleértve a Vulkan vertex puffert és a transzformációt.
 */
class MeshObject {
public:
    /**
     * @brief Konstruktor.
     */
    MeshObject();

    /**
     * @brief Destruktor.
     */
    ~MeshObject();

    /**
     * @brief Létrehozza a MeshObject Vulkan erőforrásait.
     *
     * Inicializálja és feltölti a Vulkan Vertex Puffert a megadott vertex adatokkal.
     *
     * @param context A Vulkan környezet (eszköz, memória, parancskészlet) mutatója.
     * @param vertices A feltöltendő vertex adatok vektora.
     */
    void create(VulkanContext* context, const std::vector<float>& vertices);

    /**
     * @brief Felszabadítja az objektum által birtokolt Vulkan erőforrásokat (Vertex Puffer és Memória).
     *
     * @param device A Vulkan logikai eszköz.
     */
    void cleanup(VkDevice device);

    /**
     * @brief Rögzíti az objektum rajzolási parancsait a megadott parancspufferbe.
     *
     * Kiszámítja az MVP mátrixot (Model-View-Projection), feltölti Push Konstansként,
     * majd elvégzi a vertex puffer kötését és a vkCmdDraw hívást.
     *
     * @param commandBuffer A rögzítés alatt álló parancspuffer.
     * @param pipelineLayout A pipeline elrendezése (a Push Konstansok számára).
     * @param animationTime A futásidő az animált forgatáshoz.
     * @param viewProjection A már kiszámított nézeti-vetítési (View * Projection) mátrix.
     * @param isWireframe Logikai érték, ami jelzi, hogy wireframe-ként rajzolódik-e (logikailag kezelt a rendererben).
     */
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, float animationTime, const glm::mat4& viewProjection, bool isWireframe) const;

    // --- Objektum Transzformációs Állapot ---

    /** @brief Az objektum világkoordinátás pozíciója (alapértelmezett: origó). */
    glm::vec3 position = glm::vec3(0.0f);

    /** @brief A forgatás tengelye (alapértelmezett: Y tengely). */
    glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);

    /** @brief A forgatás sebessége fok/másodpercben (0.0f esetén statikus). */
    float rotationSpeed = 0.0f;
private:
    // --- Vulkan Geometria Erőforrások ---

    /** @brief A Vulkan Vertex Puffer leírója. */
    VkBuffer vertexBuffer = VK_NULL_HANDLE;

    /** @brief A Vertex Pufferhez allokált eszközmemória leírója. */
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    /** @brief A rajzoláshoz használandó csúcsok száma. */
    uint32_t vertexCount = 0;

    /** @brief Mutató a Vulkan környezetre, a belső Vulkan hívásokhoz. */
    VulkanContext* context = nullptr;
};