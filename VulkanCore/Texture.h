#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "VulkanContext.h"

class Texture {
public:
    void create(VulkanContext* ctx, const std::string& diffusePath, const std::string& roughnessPath, VkDescriptorPool pool, VkDescriptorSetLayout layout);
    void cleanup();

    VkDescriptorSet descriptorSet;

private:
    VulkanContext* context;

    // --- 1. Diffuse (Szín) Textúra Erőforrásai ---
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
    VkSampler sampler;

    // --- 2. Roughness (Érdesség) Textúra Erőforrásai ---
    VkImage roughnessImage;
    VkDeviceMemory roughnessImageMemory;
    VkImageView roughnessImageView;
    VkSampler roughnessSampler;
};