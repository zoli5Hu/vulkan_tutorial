// VulkanCore/VulkanSwapchain.h
#pragma once

#include "VulkanContext.h"

class VulkanSwapchain {
public:
    VulkanSwapchain();
    ~VulkanSwapchain();

    void create(VulkanContext* context, VkSurfaceKHR surface, GLFWwindow* window);
    void cleanup();

    VkSwapchainKHR getSwapchain() const { return swapChain; }
    VkFormat getImageFormat() const { return swapChainImageFormat; }
    VkExtent2D getExtent() const { return swapChainExtent; }
    const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(swapChainImages.size()); }

private:
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    VulkanContext* context;

    void createSwapChain(VkSurfaceKHR surface, GLFWwindow* window);

    void createImageViews();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
};