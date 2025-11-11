/**
 * @file VulkanContext.cpp
 * @brief Megvalósítja a VulkanContext osztályt, amely a Vulkan környezet (Instance, Device, Queues, CommandPool)
 * és az alacsony szintű erőforráskezelés (Buffer, Image, Memory) fő logikáját tartalmazza.
 */
// VulkanContext.cpp
#include "VulkanContext.h"

using namespace std;

/**
 * @brief Statikus callback függvény a Vulkan validációs rétegek üzeneteinek feldolgozására.
 *
 * A Vulkan hívja meg futási időben, ha érvénytelen hívást észlel.
 *
 * @param messageSeverity Az üzenet súlyossága (VERBOSE, WARNING, ERROR).
 * @param messageType Az üzenet típusa (GENERAL, VALIDATION, PERFORMANCE).
 * @param pCallbackData A validációs üzenet tartalmát (pl. hibaüzenet szövege) tartalmazó adatszerkezet.
 * @param pUserData Felhasználói adatok (nem használt).
 * @return VK_FALSE, ami azt jelzi, hogy a Vulkan meghívásakor nem kell megszakítani az aktuális hívást.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

/**
 * @brief Dinamikus betöltő (dispatcher) a vkCreateDebugUtilsMessengerEXT függvény számára.
 *
 * A debug messenger egy kiterjesztés (extension), nem része a Vulkan mag API-nak,
 * ezért a függvénycímét manuálisan kell lekérdezni a vkGetInstanceProcAddr segítségével.
 *
 * @param instance A Vulkan Instance.
 * @param pCreateInfo A debug messenger létrehozási beállításai.
 * @param pAllocator Az allokátor callbackek (null).
 * @param pDebugMessenger Mutató az elkészült VkDebugUtilsMessengerEXT leíróra.
 * @return A Vulkan függvény visszatérési értéke.
 */
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    // Lekéri a kiterjesztés függvény pointerét.
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

/**
 * @brief Dinamikus betöltő (dispatcher) a vkDestroyDebugUtilsMessengerEXT függvény számára.
 *
 * @param instance A Vulkan Instance.
 * @param debugMessenger A megsemmisítendő debug messenger leíró.
 * @param pAllocator Az allokátor callbackek (null).
 */
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {
    // Lekéri a kiterjesztés függvény pointerét.
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

/**
 * @brief Konstruktor.
 */
VulkanContext::VulkanContext() {
}

/**
 * @brief Destruktor.
 */
VulkanContext::~VulkanContext() {
}

/**
 * @brief Inicializálja a Vulkan Instance-t és a Debug Messengert.
 *
 * @param window A GLFW ablak objektum (az Instance létrehozásához szükséges kiterjesztések miatt).
 */
void VulkanContext::initInstance(GLFWwindow* window) {
    createInstance();
    setupDebugMessenger();
}

/**
 * @brief Inicializálja a Vulkan Logikai Eszközt.
 *
 * Kiválasztja a megfelelő fizikai eszközt, létrehozza a logikai eszközt és a parancskészletet.
 *
 * @param surface A renderelési felület (VkSurfaceKHR).
 */
void VulkanContext::initDevice(VkSurfaceKHR surface) {
    pickPhysicalDevice(surface);
    createLogicalDevice(surface);
    createCommandPool();
}

/**
 * @brief Felszabadítja az osztály által birtokolt Vulkan erőforrásokat.
 *
 * Fontos, hogy a felszabadítás fordított sorrendben történjen a létrehozáshoz képest.
 */
void VulkanContext::cleanup() {
    // Parancskészlet felszabadítása
    vkDestroyCommandPool(device, commandPool, nullptr);
    // Logikai eszköz felszabadítása
    vkDestroyDevice(device, nullptr);

    // Debug messenger felszabadítása (csak ha engedélyezve van a validáció)
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // Instance felszabadítása
    vkDestroyInstance(instance, nullptr);
}

/**
 * @brief Létrehozza a Vulkan Instance-t.
 *
 * Beállítja az alkalmazás adatait, kéri a szükséges kiterjesztéseket (GLFW, Debug Utils)
 * és engedélyezi a validációs rétegeket (ha nincsen NDEBUG definiálva).
 */
void VulkanContext::createInstance() {
        // Ellenőrzi, hogy a kért validációs rétegek elérhetőek-e.
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw runtime_error("validation layers requested, but not available!");
        }
        // VkApplicationInfo: Opcionális adatok a Vulkan illesztőprogram számára a teljesítmény-optimalizáláshoz.
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // VkInstanceCreateInfo: A Vulkan Instance létrehozásához szükséges fő szerkezet.
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Kéri a szükséges GLFW kiterjesztéseket (pl. VK_KHR_surface, VK_KHR_win32_surface stb.).
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Validációs rétegek engedélyezése.
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // Beállítja a Debug Messenger callbacket.
            populateDebugMessengerCreateInfo(debugCreateInfo);
            // A pNext pointer használata a debug create infó Instance létrehozásához való csatolására.
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // Maga a Vulkan Instance létrehozása.
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
        }
}
/**
 * @brief Beállítja a Vulkan Debug Messengert (Validációs Rétegek).
 *
 * Csak akkor fut le, ha a validációs rétegek engedélyezve vannak (DEBUG build).
 */
void VulkanContext::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    // Létrehozza a debug messengert a debugCallback függvény regisztrálásával.
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw runtime_error("failed to set up debug messenger!");
    }
}

/**
 * @brief Kiválasztja a Vulkan renderelésre legalkalmasabb fizikai eszközt (GPU).
 *
 * Iterál az elérhető eszközökön és az `isDeviceSuitable` függvény segítségével választ.
 *
 * @param surface A renderelési felület (VkSurfaceKHR), amelyhez a Present (megjelenítési) képességet ellenőrizni kell.
 */
void VulkanContext::pickPhysicalDevice(VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw runtime_error("failed to find GPUs with Vulkan support!");
    }
    vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (const auto& dev : devices)
    {
        if (isDeviceSuitable(dev, surface))
        {
            physicalDevice = dev;
            // Megkeresi és elmenti a szükséges várólista családokat.
            queueIndices = findQueueFamilies(dev, surface);
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw runtime_error("failed to find a suitable GPU!");
    }
}

/**
 * @brief Létrehozza a Vulkan Logikai Eszközt (Logical Device).
 *
 * Ez a GPU absztrakciója, amivel kommunikálunk.
 *
 * @param surface A renderelési felület, ami segít megtalálni a Present queue-t (várólistát).
 */
void VulkanContext::createLogicalDevice(VkSurfaceKHR surface) {

    QueueFamilyIndices indices = queueIndices;

    // Beállítja az összes egyedi Queue Family (várólista család) adatait.
    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f; // A várólista prioritása (fontos, ha több várólistát használunk)
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1; // Egy várólistát kérünk a családból
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Lekérdezi az eszköz (GPU) támogatott funkcióit.
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    // Kéri az engedélyezni kívánt fizikai eszköz funkciókat.
    VkPhysicalDeviceFeatures featuresToEnable = {};
    // Engedélyezi a "fillModeNonSolid" funkciót a wireframe (drótváz) rendereléshez.
    if (supportedFeatures.fillModeNonSolid) {
        featuresToEnable.fillModeNonSolid = VK_TRUE;
    }
    enabledDeviceFeatures = featuresToEnable;

    // VkDeviceCreateInfo: A Logikai Eszköz létrehozásához szükséges fő szerkezet.
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &enabledDeviceFeatures;

    // Engedélyezi a kötelező eszköz kiterjesztéseket (pl. VK_KHR_SWAPCHAIN).
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Validációs rétegek csatolása a Logikai Eszközhöz (ha engedélyezve van).
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    // Maga a Logikai Eszköz létrehozása.
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw runtime_error("failed to create logical device!");
    }

    // Lekéri a Graphics (grafikus) és Present (megjelenítő) várólista kezelőket.
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

/**
 * @brief Létrehozza a Command Pool-t (Parancskészlet).
 *
 * A parancspufferek (Command Buffer) lefoglalására szolgáló gyűjtő.
 */
void VulkanContext::createCommandPool() {
    QueueFamilyIndices indices = queueIndices;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // Lehetővé teszi a parancspufferek egyéni resetelését.
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // A Command Pool-nak a grafikus várólista családhoz kell kötődnie.
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

/**
 * @brief Ellenőrzi, hogy egy adott fizikai eszköz (GPU) alkalmas-e a renderelésre.
 *
 * Több kritériumot ellenőriz: várólista családok, kiterjesztések, swapchain támogatás, funkciók.
 *
 * @param dev Az ellenőrizendő fizikai eszköz.
 * @param surface A felület, amelyhez a megjelenítési támogatást ellenőrizni kell.
 * @return true, ha az eszköz használható, egyébként false.
 */
bool VulkanContext::isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    // 1. Várólista családok ellenőrzése (grafikus és megjelenítési is kell).
    QueueFamilyIndices indices = findQueueFamilies(dev, surface);
    // 2. Szükséges eszköz-kiterjesztések támogatásának ellenőrzése (pl. swapchain).
    bool extensionsSupported = checkDeviceExtensionSupport(dev);


    // 3. Swap Chain támogatás ellenőrzése (legalább 1 formátum és 1 megjelenítési mód).
    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(dev, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // 4. Szükséges fizikai eszköz funkciók ellenőrzése (pl. drótváz).
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(dev, &supportedFeatures);
    bool featuresAdequate = supportedFeatures.fillModeNonSolid;

    return indices.isComplete() && extensionsSupported && swapChainAdequate && featuresAdequate;
}

/**
 * @brief Megkeresi a Vulkan eszköz (GPU) támogatott várólista családjait.
 *
 * Különösen a Grafikus (VK_QUEUE_GRAPHICS_BIT) és a Megjelenítési (Present) támogatású családot keresi.
 *
 * @param dev A fizikai eszköz.
 * @param surface A felület, amelyhez a megjelenítési támogatást lekérdezzük.
 * @return A megtalált várólista indexeket tartalmazó struktúra.
 */
QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());


    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        // Grafikus család keresése
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        // Megjelenítési (Present) támogatás keresése
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }


    return indices;
}

/**
 * @brief Ellenőrzi, hogy a fizikai eszköz támogatja-e az összes kötelező eszköz-kiterjesztést.
 *
 * @param dev A fizikai eszköz.
 * @return true, ha minden kiterjesztés támogatott, egyébként false.
 */
bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice dev) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

    vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

    // Egy set (halmaz) használatával ellenőrizzük, hogy minden kötelező kiterjesztés megtalálható-e.
    set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty(); // Ha a halmaz üres, mindent megtaláltunk.
}

/**
 * @brief Lekérdezi a Swap Chain (képcserélő lánc) támogatási részleteit a fizikai eszközhöz és felülethez.
 *
 * Ez az információ szükséges a Swap Chain megfelelő létrehozásához.
 *
 * @param dev A fizikai eszköz.
 * @param surf A renderelési felület.
 * @return A támogatott képességeket, formátumokat és megjelenítési módokat tartalmazó struktúra.
 */
SwapChainSupportDetails VulkanContext::querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surf) {
    SwapChainSupportDetails details;

    // 1. Alapvető képességek (min/max képek, felbontás, transzformációk)
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surf, &details.capabilities);

    // 2. Támogatott felületi formátumok (színmélység, színterek)
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &formatCount, details.formats.data());
    }

    // 3. Támogatott megjelenítési módok (pl. VSZINKRON: MAILBOX, FIFO)
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &presentModeCount, details.presentModes.data());
    }

    return details;
}

/**
 * @brief Ellenőrzi, hogy az összes kért validációs réteg elérhető-e a rendszeren.
 *
 * @return true, ha a rétegek támogatottak, egyébként false.
 */
bool VulkanContext::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Összehasonlítja a kért rétegeket az elérhető rétegekkel.
    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Lekérdezi a Vulkan Instance létrehozásához szükséges kiterjesztéseket.
 *
 * Hozzáadja a GLFW által kért platformspecifikus kiterjesztéseket és opcionálisan a Debug Utils kiterjesztést.
 *
 * @return A szükséges kiterjesztések neveit tartalmazó vektor.
 */
std::vector<const char*> VulkanContext::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    // Lekéri a GLFW által kért kiterjesztéseket (pl. Windowing System integrációhoz).
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Hozzáadja a debug kiterjesztést, ha a validációs rétegek engedélyezve vannak.
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }


    return extensions;
}

/**
 * @brief Kitölti a VkDebugUtilsMessengerCreateInfoEXT struktúrát a kívánt súlyossági és típusú üzenetekkel.
 *
 * @param createInfo A kitöltendő struktúra.
 */
void VulkanContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // Üzenetek súlyossága: Minden (Verbose), Figyelmeztetés (Warning), Hiba (Error).
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // Üzenetek típusa: Általános (General), Validáció (Validation), Teljesítmény (Performance).
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    // A callback függvényre mutató pointer.
    createInfo.pfnUserCallback = debugCallback;
}

/**
 * @brief Megtalálja a megadott igényeknek megfelelő memória típust.
 *
 * Ez a függvény alapvető a Vulkan memóriakezelésében, mivel a memóriát külön kell allokálni a leírótól.
 *
 * @param typeFilter Bitmaszk, amely jelzi, hogy mely memória típusok kompatibilisek az erőforrással (pl. Bufferrel).
 * @param properties A memória kívánt tulajdonságai (pl. HOST_VISIBLE, DEVICE_LOCAL).
 * @return A megfelelő memória típus indexe.
 */
uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    // Lekérdezi a GPU memóriatípusait.
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // Ellenőrzi, hogy:
        // 1. Az adott memória típus (1 << i) szerepel-e a typeFilter-ben (kompatibilis-e az erőforrással).
        // 2. Az adott memória típus rendelkezik-e az összes kért tulajdonsággal (properties).
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

/**
 * @brief Létrehoz egy Vulkan puffert (VkBuffer) és lefoglalja hozzá a memóriát.
 *
 * @param size A puffer kívánt mérete.
 * @param usage A puffer használati módja (pl. VK_BUFFER_USAGE_VERTEX_BUFFER_BIT).
 * @param properties A memória kívánt tulajdonságai (pl. VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).
 * @param buffer Kimeneti paraméter a létrehozott VkBuffer leíró számára.
 * @param bufferMemory Kimeneti paraméter a lefoglalt VkDeviceMemory leíró számára.
 */
void VulkanContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // 1. Buffer leíró létrehozása
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    // 2. Memória igények lekérdezése
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // 3. Memória allokálása
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    // Megtalálja a megfelelő memóriatípust a követelmények és a kívánt tulajdonságok alapján.
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    // 4. Memória kötése a bufferhez
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

/**
 * @brief Végrehajt egy parancsot egyetlen alkalommal a grafikus várólistán (Queue).
 *
 * Ez egy segédfüggvény, amelyet tipikusan transzfer (másolási) és Image Layout Transition (kép elrendezés váltás)
 * műveletekhez használnak. A hívás szinkron.
 *
 * @param commandFunction A lambda függvény, amely tartalmazza a rögzítendő Vulkan parancsokat (pl. vkCmdCopyBuffer).
 */
void VulkanContext::executeSingleTimeCommands(std::function<void(VkCommandBuffer)> commandFunction) {
    // 1. Parancspuffer allokálása
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // 2. Parancspuffer rögzítésének indítása
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // Jelzi, hogy ez a puffer csak egyszer lesz elküldve.
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // 3. A felhasználó által megadott parancsok rögzítése
    commandFunction(commandBuffer);

    // 4. Parancspuffer rögzítésének befejezése
    vkEndCommandBuffer(commandBuffer);

    // 5. Parancspuffer elküldése a grafikus várólistára
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    // Várja meg a végrehajtást (szinkron hívás).
    vkQueueWaitIdle(graphicsQueue);

    // 6. Parancspuffer felszabadítása
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

/**
 * @brief Egy puffer tartalmának átmásolása egy másikba a grafikus várólista használatával.
 *
 * @param srcBuffer A forrás puffer.
 * @param dstBuffer A cél puffer.
 * @param size A másolandó adat mérete.
 */
void VulkanContext::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    executeSingleTimeCommands([&](VkCommandBuffer commandBuffer) {
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    });
}

/**
 * @brief Létrehoz egy Vulkan képet (VkImage) és lefoglalja hozzá a memóriát.
 *
 * Ezt a funkciót használják textúrák, mélységi pufferek, stb. létrehozásához.
 *
 * @param width A kép szélessége.
 * @param height A kép magassága.
 * @param format A kép formátuma (pl. VK_FORMAT_D32_SFLOAT).
 * @param tiling A kép csempézési módja (TILING_OPTIMAL a legjobb teljesítmény).
 * @param usage A kép használati módja (pl. VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT).
 * @param properties A memória kívánt tulajdonságai.
 * @param image Kimeneti paraméter a létrehozott VkImage leíró számára.
 * @param imageMemory Kimeneti paraméter a lefoglalt VkDeviceMemory leíró számára.
 */
void VulkanContext::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    // 1. Image leíró létrehozása
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    // 2. Memória igények lekérdezése
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    // 3. Memória allokálása
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    // 4. Memória kötése a képhez
    vkBindImageMemory(device, image, imageMemory, 0);
}

/**
 * @brief Létrehoz egy VkImageView-t a megadott VkImage-hez.
 *
 * Az Image View definiálja, hogyan kell a Vulkan-nak értelmeznie a képet (2D textúra, 3D textúra, array, stb.).
 *
 * @param image A megtekintendő kép.
 * @param format A kép formátuma.
 * @param aspectFlags A kép aspektusa (pl. VK_IMAGE_ASPECT_COLOR_BIT vagy VK_IMAGE_ASPECT_DEPTH_BIT).
 * @return A létrehozott VkImageView leíró.
 */
VkImageView VulkanContext::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    // A megtekintett képterület specifikációja.
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}