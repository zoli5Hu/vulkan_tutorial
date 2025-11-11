/**
 * @file VulkanContext.h
 * @brief Deklarálja a VulkanContext osztályt és a kapcsolódó segédstruktúrákat.
 *
 * Ez az osztály felelős a Vulkan Instance (példány), a Physical Device (fizikai eszköz, pl. GPU),
 * a Logical Device (logikai eszköz), a várólisták (Queues) és a parancskészlet (Command Pool)
 * létrehozásáért és kezeléséért. Ezenkívül segédfüggvényeket biztosít az alacsony szintű
 * erőforrás-kezeléshez (pufferek, képek).
 */
// VulkanContext.h
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional> // A választható (nullable) indexek tárolásához
#include <set>      // Az egyedi várólista indexek kezeléséhez
#include <string>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <functional> // std::function használatához (executeSingleTimeCommands)
#include <limits>

/**
 * @struct QueueFamilyIndices
 * @brief Struktúra a szükséges várólista családok (Queue Families) indexeinek tárolására.
 *
 * A Vulkan különböző művelettípusokhoz (grafika, számítás, transzfer, megjelenítés)
 * külön várólistákat használhat.
 */
struct QueueFamilyIndices
{
    /** @brief A grafikus műveleteket támogató család indexe. */
    std::optional<uint32_t> graphicsFamily;
    /** @brief A megjelenítést (Present) támogató család indexe. */
    std::optional<uint32_t> presentFamily;

    /**
     * @brief Ellenőrzi, hogy minden szükséges várólista családot megtaláltunk-e.
     * @return true, ha a grafikus és a megjelenítési index is érvényes, egyébként false.
     */
    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

/**
 * @struct SwapChainSupportDetails
 * @brief Struktúra a Swap Chain (képcserélő lánc) támogatási részleteinek tárolására.
 *
 * Ezek az adatok szükségesek a Swap Chain megfelelő konfigurálásához és létrehozásához.
 */
struct SwapChainSupportDetails
{
    /** @brief Alapvető képességek (min/max képek, felbontás, transzformációk). */
    VkSurfaceCapabilitiesKHR capabilities;
    /** @brief Támogatott felületi formátumok (színmélység, színterek). */
    std::vector<VkSurfaceFormatKHR> formats;
    /** @brief Támogatott megjelenítési módok (pl. VSZINKRON: MAILBOX, FIFO). */
    std::vector<VkPresentModeKHR> presentModes;
};

/**
 * @class VulkanContext
 * @brief A központi Vulkan környezetet kezelő osztály.
 */
class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    /**
     * @brief Inicializálja a Vulkan Instance-t és a Debug Messengert.
     * @param window A GLFW ablak, amelyhez a Vulkan kiterjesztéseket lekérdezzük.
     */
    void initInstance(GLFWwindow* window);

    /**
     * @brief Inicializálja a fizikai és logikai eszközt, valamint a parancskészletet.
     * @param surface A Vulkan felület (VkSurfaceKHR), amelyhez az eszközt választjuk.
     */
    void initDevice(VkSurfaceKHR surface);

    /**
     * @brief Felszabadítja az összes lefoglalt Vulkan erőforrást (Device, Instance, CommandPool stb.).
     */
    void cleanup();

    // --- Getter függvények az alapvető Vulkan objektumok eléréséhez ---

    VkInstance getInstance() const { return instance; }
    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentQueue() const { return presentQueue; }
    VkCommandPool getCommandPool() const { return commandPool; }
    QueueFamilyIndices getQueueFamilies() const { return queueIndices; }

    /**
     * @brief Lekérdezi a Swap Chain támogatási részleteit a megadott eszközhöz és felülethez.
     * @param dev A fizikai eszköz.
     * @param surf A renderelési felület.
     * @return SwapChainSupportDetails struktúra.
     */
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surf);

    /**
     * @brief Megtalálja a megfelelő memóriatípus indexét a fizikai eszközön.
     * @param typeFilter Bitmaszk, amely a kompatibilis memóriatípusokat jelzi.
     * @param properties A memória kívánt tulajdonságai (pl. HOST_VISIBLE, DEVICE_LOCAL).
     * @return A talált memóriatípus indexe.
     */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // --- Erőforrás-kezelő segédfüggvények ---

    /**
     * @brief Létrehoz egy Vulkan puffert és lefoglalja hozzá a memóriát.
     * @param size A puffer mérete.
     * @param usage A puffer használati módja (pl. Vertex Buffer, Transfer Src).
     * @param properties A memória kívánt tulajdonságai.
     * @param buffer Kimeneti paraméter a VkBuffer leíróhoz.
     * @param bufferMemory Kimeneti paraméter a VkDeviceMemory leíróhoz.
     */
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    /**
     * @brief Létrehoz egy Vulkan képet (Image) és lefoglalja hozzá a memóriát.
     * @param width A kép szélessége.
     * @param height A kép magassága.
     * @param format A kép formátuma.
     * @param tiling A kép csempézési módja (Optimal/Linear).
     * @param usage A kép használati módja (pl. Depth Attachment, Sampled).
     * @param properties A memória kívánt tulajdonságai.
     * @param image Kimeneti paraméter a VkImage leíróhoz.
     * @param imageMemory Kimeneti paraméter a VkDeviceMemory leíróhoz.
     */
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    /**
     * @brief Létrehoz egy VkImageView-t (képnézetet) egy meglévő VkImage-hez.
     * @param image A kép, amelyhez a nézet készül.
     * @param format A kép formátuma.
     * @param aspectFlags A kép aspektusa (pl. Color, Depth).
     * @return A létrehozott VkImageView leíró.
     */
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    /**
     * @brief Végrehajt egy parancssorozatot szinkron módon (egyszeri parancsokhoz).
     * @param commandFunction A lambda függvény, amely a rögzítendő parancsokat tartalmazza.
     */
    void executeSingleTimeCommands(std::function<void(VkCommandBuffer)> commandFunction);

    /**
     * @brief Átmásolja az adatot egyik pufferből a másikba.
     * @param srcBuffer Forrás puffer.
     * @param dstBuffer Cél puffer.
     * @param size A másolandó adat mérete.
     */
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
private:
    /** @brief A Vulkan API példánya (Instance). */
    VkInstance instance;
    /** @brief A validációs rétegek debug üzeneteit kezelő leíró. */
    VkDebugUtilsMessengerEXT debugMessenger;
    /** @brief A kiválasztott fizikai eszköz (GPU). */
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    /** @brief A logikai eszköz, amellyel a műveleteket végezzük. */
    VkDevice device;
    /** @brief A parancspufferek allokálására szolgáló parancskészlet. */
    VkCommandPool commandPool;

    /** @brief Az engedélyezett fizikai eszköz funkciók (pl. wireframe). */
    VkPhysicalDeviceFeatures enabledDeviceFeatures = {};

    /** @brief A grafikus műveletek várólistája. */
    VkQueue graphicsQueue;
    /** @brief A megjelenítési műveletek várólistája. */
    VkQueue presentQueue;
    /** @brief A kiválasztott várólista családok indexei. */
    QueueFamilyIndices queueIndices;

    /** @brief A kért validációs rétegek listája. */
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    /** @brief A kért eszköz-szintű kiterjesztések listája (pl. SwapChain). */
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

// A validációs rétegek engedélyezése/tiltása fordítási időben (Debug/Release).
#ifdef NDEBUG
    /** @brief Validációs rétegek állapota (Release build: false). */
    const bool enableValidationLayers = false;
#else
    /** @brief Validációs rétegek állapota (Debug build: true). */
    const bool enableValidationLayers = true;
#endif

    // --- Privát inicializáló segédfüggvények ---

    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice(VkSurfaceKHR surface);
    void createLogicalDevice(VkSurfaceKHR surface);
    void createCommandPool();

    // --- Privát eszközválasztó segédfüggvények ---

    bool isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice dev);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface);

    // --- Privát validációs segédfüggvények ---

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};

// --- Debug Messenger dinamikus betöltő függvények (a .cpp fájlban definiálva) ---

/**
 * @brief Dinamikus betöltő a vkCreateDebugUtilsMessengerEXT függvényhez.
 */
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

/**
 * @brief Dinamikus betöltő a vkDestroyDebugUtilsMessengerEXT függvényhez.
 */
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);