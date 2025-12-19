/**
 * @file VulkanContext.h
 * @brief Deklarálja a VulkanContext osztályt és a kapcsolódó segédstruktúrákat.
 * Ez az osztály felelős a GPU-val való közvetlen kommunikációért.
 */
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <set>
#include <string>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <functional>
#include <limits>

/**
 * @brief Segédstruktúra a GPU várakozási sorainak (Queue Families) azonosításához.
 */
struct QueueFamilyIndices
{
    // A grafikai parancsok (rajzolás) feldolgozásáért felelős sor indexe
    std::optional<uint32_t> graphicsFamily;
    // Az ablakrendszer felé történő képküldésért (megjelenítés) felelős sor indexe
    std::optional<uint32_t> presentFamily;

    // Ellenőrzi, hogy megtaláltunk-e minden szükséges sort
    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

/**
 * @brief A Swapchain (képváltó lánc) támogatásának technikai részletei.
 */
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;      // Alapvető képességek (méretek, képek száma)
    std::vector<VkSurfaceFormatKHR> formats;     // Támogatott színformátumok (pl. B8G8R8A8_SRGB)
    std::vector<VkPresentModeKHR> presentModes; // Megjelenítési módok (pl. V-Sync, Mailbox)
};

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    // --- Életciklus kezelés ---
    void initInstance(GLFWwindow* window);      // Vulkan Instance és Debugger inicializálása
    void initDevice(VkSurfaceKHR surface);      // Fizikai és logikai eszközök felépítése
    void cleanup();                             // Minden Vulkan erőforrás leállítása és törlése

    // --- Getter függvények (Csak olvasható hozzáférés a belső handle-ökhöz) ---
    VkInstance getInstance() const { return instance; }
    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentQueue() const { return presentQueue; }
    VkCommandPool getCommandPool() const { return commandPool; }
    QueueFamilyIndices getQueueFamilies() const { return queueIndices; }

    // --- Segédfüggvények a rendereléshez és memóriakezeléshez ---
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surf);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // --- Erőforrás-kezelés (Pufferek, Képek) ---
    // Általános célú GPU puffer (Vertex, Index, Uniform) létrehozása
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    // Nyers képobjektum létrehozása a GPU-n
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    // Képnézet (ImageView) létrehozása, ami meghatározza a kép értelmezését a shaderben
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    // Azonnali parancsvégrehajtás (pl. adatátvitel staging pufferből végleges helyre)
    void executeSingleTimeCommands(std::function<void(VkCommandBuffer)> commandFunction);

    // Adatmásolás két puffer között (GPU-n belül)
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    // --- Textúra és Descriptor kezelés ---
    // Komplex textúra betöltése fájlból közvetlenül a GPU-ra
    void createTextureImage(const std::string& filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory);

    // Textúra-specifikus ImageView készítése
    void createTextureImageView(VkImage image, VkImageView& imageView);

    // Mintavételező (Sampler) létrehozása szűréssel és anizotrópiával
    void createTextureSampler(VkSampler& sampler);

    // Tároló a Descriptor Set-ek számára
    void createDescriptorPool(VkDescriptorPool& descriptorPool);

    // Kép elrendezésének (Layout) váltása (pl. UNDEFINED -> SHADER_READ_ONLY)
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    // Pufferben lévő pixeladatok másolása egy képobjektumba
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

private:
    // Alapvető Vulkan handle-ök
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // A kiválasztott videókártya
    VkDevice device;                                 // A szoftveres interfész a kártyához
    VkCommandPool commandPool;                       // A parancspufferek gyűjtőhelye
    VkPhysicalDeviceFeatures enabledDeviceFeatures = {}; // Engedélyezett hardveres funkciók

    VkQueue graphicsQueue;                           // Grafikai műveletek sora
    VkQueue presentQueue;                            // Megjelenítési műveletek sora
    QueueFamilyIndices queueIndices;                 // A sorok indexei

    // Szükséges rétegek és kiterjesztések
    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // Validációs rétegek automatikus kapcsolása Build konfiguráció alapján
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    // Belső inicializáló függvények
    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice(VkSurfaceKHR surface);
    void createLogicalDevice(VkSurfaceKHR surface);
    void createCommandPool();

    // Eszköz alkalmassági vizsgálatok
    bool isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice dev);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface);

    // Validáció és kiterjesztés segédletek
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};

// --- Globális segédfüggvények a Debug Messenger kezeléséhez ---
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);