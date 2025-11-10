// VulkanContext.h
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
#include <algorithm> // clamp-hoz
#include <limits>    // numeric_limits-hez

struct QueueFamilyIndices
{
    //jelenleg csak 1 változó de a struktúra hasznos lesz ha többet is hjozzá szeretnénk adni mert akkor egyszerűbb lesz visszaadni
    //azért adjuk meg optionalnak mert lehet ,hogy nincs is ilyen queue family a gpu-n (pl csak compute van) és az uint32 csak pozitív értékeket tud tárolni
    //ezért nemtudjuk ez lerendezn ia -1 értékkel
    std::optional<uint32_t> graphicsFamily; // Itt tároljuk a graphics queue family indexét
    //annak a családnak az indexe amelyik támogatja a prezentációt (ablakra rajzolást)
    std::optional<uint32_t> presentFamily;


    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

// Swap chain támogatási adatokat tároló struktúra
struct SwapChainSupportDetails
{
    // Alap képességek: kép méretek, transzformációk, stb.
    VkSurfaceCapabilitiesKHR capabilities;
    // Támogatott formátumok: színformátum és színtér kombinációk
    std::vector<VkSurfaceFormatKHR> formats;
    // Elérhető megjelenítési módok: VSync, triple buffering, stb.
    std::vector<VkPresentModeKHR> presentModes;
};

// A fő Context osztály:
// Ez kezeli a Vulkan "mag" inicializálását és erőforrásait
// (Instance, Device, Queues, CommandPool, Debugger)
class VulkanContext {
public:
    // Konstruktor és destruktor
    VulkanContext();
    ~VulkanContext();

    // Fő inicializáló metódusok
    void initInstance(GLFWwindow* window);
    void initDevice(VkSurfaceKHR surface);
    void cleanup();

    // Get-terek (ezekre lesz szükség a többi osztályban)
    VkInstance getInstance() const { return instance; }
    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentQueue() const { return presentQueue; }
    VkCommandPool getCommandPool() const { return commandPool; }
    QueueFamilyIndices getQueueFamilies() const { return queueIndices; }

    // Publikus segédfüggvény (a Swapchainnek kelleni fog)
    // Lekérdezi a fizikai eszköz swap chain támogatási adatait
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surf);

private:
    // Vulkan "mag" objektumok
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkCommandPool commandPool;

    // Queue-k
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    QueueFamilyIndices queueIndices; // Eltároljuk a talált indexeket

    // Validációs rétegek
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    //swapchan setup start extension enable
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice(VkSurfaceKHR surface);
    void createLogicalDevice(VkSurfaceKHR surface);
    void createCommandPool();

    // Eszköz-választó segédfüggvények
    // Ellenőrzi, hogy egy fizikai eszköz (GPU) alkalmas-e
    bool isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR surface);
    // Röviden: ellenőrzi, hogy az adott fizikai eszköz (GPU) támogatja-e az általunk kért eszköz-kiterjesztéseket
    bool checkDeviceExtensionSupport(VkPhysicalDevice dev);
    //kitöltjük a struktúrát hány családodt akarunk használni és visszaadjuk
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface);

    // Debugger segédfüggvények
    bool checkValidationLayerSupport();
    // Visszaadja a Vulkan instance-hez szükséges (engedélyezendő) extensionök listáját
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};

//itt ugye drivernek már megmondtuk a createifoban hogy az instance használja de az extension még nincs betöltve cska az engedély van megadva itt betöltjük
//ez egy proxi/wrapper fügvény ami feladatot tovább adja vagy dinamikusan betölti itt meghívom az origin vulkan fgt plusz logolok vagy hibát hárítok el
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

//ugyanúgy proxi fg betölti a debug felszabadítót
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);