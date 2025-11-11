// VulkanCore/VulkanPipeline.h
#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include <fstream>

class VulkanPipeline {
public:
    VulkanPipeline();
    ~VulkanPipeline();

    void create(VulkanContext* context, VulkanSwapchain* swapchain, VkImageView depthImageView, VkFormat depthFormat);
    void cleanup();

    VkRenderPass getRenderPass() const { return renderPass; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }
    VkPipeline getWireframePipeline() const { return wireframePipeline; }
    VkFramebuffer getFramebuffer(uint32_t index) const { return swapChainFramebuffers[index]; }

private:
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkPipeline wireframePipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VulkanContext* context;

    void createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat);

    void createGraphicsPipeline();
    void createWireframePipeline();

    void createFramebuffers(VulkanSwapchain* swapchain, VkImageView depthImageView);

    static std::vector<char> readFile(const std::string& filename);

    VkShaderModule createShaderModule(const std::vector<char>& code);
};