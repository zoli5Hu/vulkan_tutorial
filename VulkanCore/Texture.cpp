#include "Texture.h"
#include <stdexcept>
#include <array>

void Texture::create(VulkanContext* ctx, const std::string& diffusePath, const std::string& roughnessPath, VkDescriptorPool pool, VkDescriptorSetLayout layout) {
    this->context = ctx;

    // --- 1. Diffuse (Szín) Textúra létrehozása ---
    ctx->createTextureImage(diffusePath, image, imageMemory);
    ctx->createTextureImageView(image, imageView);
    ctx->createTextureSampler(sampler);

    // --- 2. Roughness (Érdesség) Textúra létrehozása ---
    ctx->createTextureImage(roughnessPath, roughnessImage, roughnessImageMemory);
    ctx->createTextureImageView(roughnessImage, roughnessImageView);
    ctx->createTextureSampler(roughnessSampler);

    // --- 3. Descriptor Set Allokálása ---
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(ctx->getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // --- 4. Descriptor Set Frissítése (Mindkét textúrával) ---

    // Binding 0: Diffuse
    VkDescriptorImageInfo diffuseInfo{};
    diffuseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    diffuseInfo.imageView = imageView;
    diffuseInfo.sampler = sampler;

    // Binding 1: Roughness
    VkDescriptorImageInfo roughnessInfo{};
    roughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    roughnessInfo.imageView = roughnessImageView;
    roughnessInfo.sampler = roughnessSampler;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    // Szín textúra írása (Binding 0)
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &diffuseInfo;

    // Roughness textúra írása (Binding 1)
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSet;
    descriptorWrites[1].dstBinding = 1; // FONTOS: Ez a shaderben a layout(binding = 1)
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &roughnessInfo;

    vkUpdateDescriptorSets(ctx->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Texture::cleanup() {
    // Diffuse takarítás
    vkDestroySampler(context->getDevice(), sampler, nullptr);
    vkDestroyImageView(context->getDevice(), imageView, nullptr);
    vkDestroyImage(context->getDevice(), image, nullptr);
    vkFreeMemory(context->getDevice(), imageMemory, nullptr);

    // Roughness takarítás
    vkDestroySampler(context->getDevice(), roughnessSampler, nullptr);
    vkDestroyImageView(context->getDevice(), roughnessImageView, nullptr);
    vkDestroyImage(context->getDevice(), roughnessImage, nullptr);
    vkFreeMemory(context->getDevice(), roughnessImageMemory, nullptr);
}