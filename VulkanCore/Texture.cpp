#include "Texture.h"
#include <stdexcept>


// TÖRÖLVE: #include "VulkanContext.h"  <- Ez okozta a hibát, mert már a Texture.h tartalmazza, ráadásul rossz az útvonal (VulkanCore mappában van)

void Texture::create(VulkanContext* ctx, const std::string& filename, VkDescriptorPool pool, VkDescriptorSetLayout layout) {
    this->context = ctx;

    // 1. Kép betöltése és feltöltése GPU-ra
    ctx->createTextureImage(filename, image, imageMemory);

    // 2. ImageView létrehozása
    ctx->createTextureImageView(image, imageView);

    // 3. Sampler létrehozása
    ctx->createTextureSampler(sampler);

    // 4. Descriptor Set allokálása
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(ctx->getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // 5. Descriptor Set frissítése
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(ctx->getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void Texture::cleanup() {
    vkDestroySampler(context->getDevice(), sampler, nullptr);
    vkDestroyImageView(context->getDevice(), imageView, nullptr);
    vkDestroyImage(context->getDevice(), image, nullptr);
    vkFreeMemory(context->getDevice(), imageMemory, nullptr);
}