// VulkanCore/VulkanSwapchain.h
#pragma once

#include "VulkanContext.h" // Szükségünk van a Context-re az eszköz (device) és a queue-k eléréséhez

class VulkanSwapchain {
public:
    // Konstruktor és destruktor
    VulkanSwapchain();
    ~VulkanSwapchain();

    // Fő inicializáló és törlő metódusok
    void create(VulkanContext* context, VkSurfaceKHR surface, GLFWwindow* window);
    void cleanup();

    // Get-terek (ezeket fogja használni a main.cpp)
    VkSwapchainKHR getSwapchain() const { return swapChain; }
    VkFormat getImageFormat() const { return swapChainImageFormat; }
    VkExtent2D getExtent() const { return swapChainExtent; }
    const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(swapChainImages.size()); }

private:
    // --- Tagváltozók (áthelyezve a main.cpp-ből) ---
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    // --- Függőségek ---
    VulkanContext* context; // Pointer a "motorra", hogy elérjük a VkDevice-t stb.

    // --- Privát segédfüggvények (áthelyezve a main.cpp-ből) ---

    // Létrehozza a swap chain-t, amely a képernyőre kerülő képek puffereit kezeli
    void createSwapChain(VkSurfaceKHR surface, GLFWwindow* window);

    // Létrehozza az image view-kat minden egyes swap chain képhez
    void createImageViews();

    // Kiválasztja a legjobb swap surface formátumot az elérhető formátumok közül
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    // Kiválasztja a prezentációs módot (VSync beállítás)
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    // Meghatározza a swap chain képek felbontását (szélesség és magasság pixelben)
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
};