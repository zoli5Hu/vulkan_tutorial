#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include <string>
#include <vector>

class VulkanPipeline {
public:
    VulkanPipeline();
    ~VulkanPipeline();

    void create(VulkanContext* ctx, VulkanSwapchain* swapchain, VkImageView depthImageView, VkFormat depthFormat);
    void cleanup();

    VkPipeline getGraphicsPipeline() { return graphicsPipeline; }
    VkPipelineLayout getPipelineLayout() { return pipelineLayout; }
    VkRenderPass getRenderPass() { return renderPass; }
    VkDescriptorSetLayout getDescriptorSetLayout() { return descriptorSetLayout; }

    // --- ÚJ: Getter a framebufferekhez ---
    const std::vector<VkFramebuffer>& getFramebuffers() const { return swapChainFramebuffers; }

private:
    VulkanContext* context;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSetLayout shadowSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkPipeline wireframePipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    void createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat);
    void createGraphicsPipeline();
    void createWireframePipeline();
    void createFramebuffers(VulkanSwapchain* swapchain, VkImageView depthImageView);

    VkShaderModule createShaderModule(const std::vector<char>& code);
    static std::vector<char> readFile(const std::string& filename);
};