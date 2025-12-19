/**
* @file MeshObject.h
 * @brief Frissítve: Publikus vertexBuffer/vertexCount az árnyék passhoz, és javított draw szignatúra.
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

    // Textúra beállítása (A descriptor set, ami a textúrákra mutat)
    void setTexture(VkDescriptorSet descSet);

    // Létrehozás (Vertex Buffer feltöltése)
    void create(VulkanContext* context, const std::vector<float>& vertices);

    // Takarítás
    void cleanup(VkDevice device);

    // Kirajzolás
    // FONTOS: A paraméterek sorrendje megváltozott, hogy illeszkedjen a .cpp-hez!
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, glm::mat4 viewProjection, float animationTime);

    // --- Publikus változók ---
    // (Azért publikusak, hogy a VulkanRenderer könnyen elérje őket az árnyék renderelésnél)

    // Transzformáció
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    float rotationSpeed = 0.0f;

    // Vulkan Erőforrások
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;

private:
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;
    VulkanContext* context = nullptr;
};