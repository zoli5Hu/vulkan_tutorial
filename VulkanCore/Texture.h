#pragma once
#include <string>
#include <vulkan/vulkan_core.h>

#include "VulkanContext.h"

struct Texture {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
    VkSampler sampler;
    VkDescriptorSet descriptorSet;
    VulkanContext* context;

    // Csak deklarációk
    void create(VulkanContext* ctx, const std::string& filename, VkDescriptorPool pool, VkDescriptorSetLayout layout);
    void cleanup();
};
