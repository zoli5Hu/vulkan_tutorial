/**
 * @file Texture.cpp
 * @brief Textúra erőforrások kezelése: Diffuse, Roughness és Normal map betöltése és Descriptor Set frissítése.
 */
#include "Texture.h"
#include <stdexcept>
#include <array>

void Texture::create(VulkanContext* ctx,
                     const std::string& diffusePath,
                     const std::string& roughnessPath,
                     const std::string& normalPath,
                     VkDescriptorPool pool,
                     VkDescriptorSetLayout layout)
{
    // A Vulkan kontextus mentése a későbbi takarításhoz és eszköz eléréshez
    this->context = ctx;

    // --- 1. Diffuse (Alapszín) Textúra létrehozása ---
    // Betölti a képet, létrehozza a GPU-oldali Image-t, a nézetet (ImageView) és a mintavételezőt (Sampler)
    ctx->createTextureImage(diffusePath, image, imageMemory);
    ctx->createTextureImageView(image, imageView);
    ctx->createTextureSampler(sampler);

    // --- 2. Roughness (Érdesség) Textúra létrehozása ---
    // Meghatározza, hogy a felület mennyire szórja szét a fényt (PBR munkafolyamathoz)
    ctx->createTextureImage(roughnessPath, roughnessImage, roughnessImageMemory);
    ctx->createTextureImageView(roughnessImage, roughnessImageView);
    ctx->createTextureSampler(roughnessSampler);

    // --- 3. Normal Map (Részletgazdag felület) Textúra létrehozása ---
    // Lehetővé teszi a finom felületi egyenetlenségek szimulálását tangens térben
    ctx->createTextureImage(normalPath, normalImage, normalImageMemory);
    ctx->createTextureImageView(normalImage, normalImageView);
    ctx->createTextureSampler(normalSampler);

    // --- 4. Descriptor Set Allokálása ---
    // Lefoglalunk egy adatkészletet a pool-ból, ami a shader számára elérhetővé teszi a textúrákat
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(ctx->getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // --- 5. Descriptor Set Frissítése (Adatok összekötése a shader bindingokkal) ---

    // Binding 0: Diffuse (Szín információ)
    VkDescriptorImageInfo diffuseInfo{};
    diffuseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    diffuseInfo.imageView = imageView;
    diffuseInfo.sampler = sampler;

    // Binding 1: Roughness (Anyag tulajdonság)
    VkDescriptorImageInfo roughnessInfo{};
    roughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    roughnessInfo.imageView = roughnessImageView;
    roughnessInfo.sampler = roughnessSampler;

    // Binding 2: Normal Map (Geometriai torzítás a shaderben)
    VkDescriptorImageInfo normalInfo{};
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalInfo.imageView = normalImageView;
    normalInfo.sampler = normalSampler;

    // A frissítési műveletek tömbje (3 különböző binding írása)
    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

    // Diffuse descriptor beállítása (Binding 0)
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &diffuseInfo;

    // Roughness descriptor beállítása (Binding 1)
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &roughnessInfo;

    // Normal Map descriptor beállítása (Binding 2)
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSet;
    descriptorWrites[2].dstBinding = 2; // Fontos: a GLSL shaderben layout(binding = 2) sampler2D normalMap;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pImageInfo = &normalInfo;

    // Az adatok tényleges átadása a GPU felé
    vkUpdateDescriptorSets(context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Texture::cleanup() {
    VkDevice device = context->getDevice();

    // Erőforrások felszabadítása fordított sorrendben az életciklus végén

    // Diffuse törlése
    vkDestroySampler(device, sampler, nullptr);
    vkDestroyImageView(device, imageView, nullptr);
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, imageMemory, nullptr);

    // Roughness törlése
    vkDestroySampler(device, roughnessSampler, nullptr);
    vkDestroyImageView(device, roughnessImageView, nullptr);
    vkDestroyImage(device, roughnessImage, nullptr);
    vkFreeMemory(device, roughnessImageMemory, nullptr);

    // Normal Map törlése
    vkDestroySampler(device, normalSampler, nullptr);
    vkDestroyImageView(device, normalImageView, nullptr);
    vkDestroyImage(device, normalImage, nullptr);
    vkFreeMemory(device, normalImageMemory, nullptr);
}