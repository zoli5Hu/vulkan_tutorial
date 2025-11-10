// VulkanCore/VulkanPipeline.h
#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include <fstream> // A readFile-hez

class VulkanPipeline {
public:
    VulkanPipeline();
    ~VulkanPipeline();

    // Fő inicializáló és törlő metódusok
    // MÓDOSÍTVA: Átadjuk a depth buffer adatait
    void create(VulkanContext* context, VulkanSwapchain* swapchain, VkImageView depthImageView, VkFormat depthFormat);
    void cleanup();

    // Get-terek (ezeket fogja használni a main.cpp a rajzoláshoz)
    VkRenderPass getRenderPass() const { return renderPass; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }
    VkFramebuffer getFramebuffer(uint32_t index) const { return swapChainFramebuffers[index]; }

private:
    // --- Tagváltozók (áthelyezve a main.cpp-ből) ---
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // --- Függőségek ---
    VulkanContext* context; // Pointer a "motorra"

    // --- Privát segédfüggvények (áthelyezve a main.cpp-ből) ---

    // Létrehozza a Render Pass-t
    // MÓDOSÍTVA: Megkapja a depth formátumot
    void createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat);

    // Létrehozza a teljes grafikus pipeline-t (shaderekkel, beállításokkal)
    void createGraphicsPipeline();

    // Létrehozza a framebuffer-eket a swapchain image view-jaihoz
    // MÓDOSÍTVA: Megkapja a depth view-t
    void createFramebuffers(VulkanSwapchain* swapchain, VkImageView depthImageView);

    // Beolvas egy fájlt (pl. shader) binárisan
    static std::vector<char> readFile(const std::string& filename);

    // Létrehoz egy VkShaderModule-t a beolvasott SPIR-V kódból
    VkShaderModule createShaderModule(const std::vector<char>& code);
};