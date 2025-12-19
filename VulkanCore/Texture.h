/**
 * @file Texture.h
 * @brief Textúra erőforrások kezelése.
 * Támogatja a Diffuse (szín), Roughness (érdesség) és Normal (domborzat) térképeket.
 */
#pragma once

#include "VulkanContext.h"
#include <string>
#include <vulkan/vulkan.h>

class Texture {
public:
    Texture() = default;
    ~Texture() = default;

    /**
     * @brief Létrehozza a textúra objektumokat és a hozzájuk tartozó Descriptor Set-et.
     * @param ctx Vulkan kontextus a GPU műveletekhez.
     * @param diffusePath Az alapszín textúra elérési útja.
     * @param roughnessPath Az érdesség (roughness) térkép elérési útja.
     * @param normalPath A normal map (tangens térbeli domborzat) elérési útja.
     * @param pool A descriptor pool, amiből a set allokálásra kerül.
     * @param layout A descriptor set elrendezése (Binding 0, 1, 2).
     */
    void create(VulkanContext* ctx,
                const std::string& diffusePath,
                const std::string& roughnessPath,
                const std::string& normalPath,
                VkDescriptorPool pool,
                VkDescriptorSetLayout layout);

    /**
     * @brief Felszabadítja az összes textúrához tartozó Image, ImageView, Sampler és Memória erőforrást.
     */
    void cleanup();

    // A GPU-n tárolt erőforrásokhoz való hozzáférést biztosító handle a shader számára
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

private:
    VulkanContext* context = nullptr;

    // --- 1. Diffuse (Szín) erőforrások ---
    // Az objektum alapvető vizuális megjelenéséért felelős
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    // --- 2. Roughness (Érdesség) erőforrások ---
    // Meghatározza a felületi fényvisszaverődés mikroszkopikus egyenetlenségeit
    VkImage roughnessImage = VK_NULL_HANDLE;
    VkDeviceMemory roughnessImageMemory = VK_NULL_HANDLE;
    VkImageView roughnessImageView = VK_NULL_HANDLE;
    VkSampler roughnessSampler = VK_NULL_HANDLE;

    // --- 3. Normal Map (Domborzat) erőforrások ---
    // A felületi normálvektorok módosításával imitál nagy felbontású geometriai részleteket
    VkImage normalImage = VK_NULL_HANDLE;
    VkDeviceMemory normalImageMemory = VK_NULL_HANDLE;
    VkImageView normalImageView = VK_NULL_HANDLE;
    VkSampler normalSampler = VK_NULL_HANDLE;
};