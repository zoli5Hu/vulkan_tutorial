// VulkanContext.cpp
#include "VulkanContext.h"

using namespace  std;

/*
//ezek a platformfüggetlenség miatt kellenek
//vkbool vulakn féle boolian igazából 32 bites integer ez mindig 4 byte zeért platformfüggetlen mert alapból cben nicns boolean
//VKAPI_ATTR egy makró ami általában üres de bizonyos platformok adatokat tesznek bele ls bizonyos fügvényeket megtud hívni a könyvtárból
//VKAPI_CALL hívásmakró ami aztmondja meg ,hogy a paraméterek ,hogyan kerülnek a stackre ez hívás konvenció vulkan driverrel
//multi platform fg struktúra [visibility / linkage] [storage] [return type] [calling convention] [name] (parameters)

//     🔹 Linkage (összekapcsolás / láthatóság):
// Megadja, hogy a függvény vagy változó más fájlokból is elérhető-e (extern), vagy csak a jelenlegi fordítási egységen belül (static).
//     🔹 Storage (tárolási osztály):
// Meghatározza, hogyan és meddig él egy változó vagy függvény (pl. auto, static, register, extern).
//
// 🔹 Calling convention (hívási konvenció):
// Előírja, hogyan történik a függvényhívás technikailag — pl. a paraméterek átadása, veremhasználat, visszatérési érték kezelése (__cdecl, __stdcall, __fastcall, stb.).
*/
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    //mennyire súlyos
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    //mi a hiba
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    //hogy írodik pl consolra
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    //saját struktúrt is lehet vele csinálni
    void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    //VkBool32 visszatérési értéke
    return VK_FALSE;
}

//itt ugye drivernek már megmondtuk a createifoban hogy az instance használja de az extension még nincs betöltve cska az engedély van megadva itt betöltjük
//ez egy proxi/wrapper fügvény ami feladatot tovább adja vagy dinamikusan betölti itt meghívom az origin vulkan fgt plusz logolok vagy hibát hárítok el
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    //lekéri az extension címét ha sikerül (itt castolunk hogy fix jó objektumot kapjunk vissza)
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

//ugyanúgy proxi fg betölti a debug felszabadítót
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// --- Konstruktor / Destruktor ---
VulkanContext::VulkanContext() {
    // Üres, az inicializálás az initVulkan-ban történik
}

VulkanContext::~VulkanContext() {
    // Üres, a cleanup() függvényt manuálisan hívjuk
}

// --- Fő metódusok ---

void VulkanContext::initInstance(GLFWwindow* window) {
    // Csak az instance-t és a debuggert hozza létre
    createInstance();
    setupDebugMessenger();
}

void VulkanContext::initDevice(VkSurfaceKHR surface) {
    // A surface-t paraméterként kapja, és beállítja a fizikai
    // és logikai eszközt, valamint a command pool-t.
    pickPhysicalDevice(surface);
    createLogicalDevice(surface);
    createCommandPool();
}

void VulkanContext::cleanup() {
    // A létrehozással ellentétes sorrendben törlünk
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // A Surface-t a main.cpp törli!

    //felszabadítja az instance t
    vkDestroyInstance(instance, nullptr);
}

// --- Privát segédfüggvények implementációja ---

void VulkanContext::createInstance() {
       //ellenőrzi hogy a layerek elérhetőek e
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw runtime_error("validation layers requested, but not available!");
        }
        //a {} a struktúra teljes kinullázásához kell
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        //itt ez 1.0.0 verziót használjuk
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        //itt enginet adunk meg de nem használunk most
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        //ha van validation layer akkor beállítja azt
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            //beállítja a layert 0 ra mert most nem kell debugolni
            createInfo.enabledLayerCount = 0;
        }

        //létrehozok egy új extensiont csak a vknak itt már nem a glfwnek
        uint32_t extensionCount = 0;
        //listába teszem a kiterjesztéseket
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        vector<VkExtensionProperties> availableExtensions(extensionCount);
        //beállítpm a listát vknak hogy töltse fel a .data az első elemre mutató pointer
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

        cout << "available extensions:\n";
        for (const auto& extension : availableExtensions)
        {
            cout << '\t' << extension.extensionName << '\n';
        }
        std::cout << "available extensions 2:\n";
        for (const auto& extension : extensions)
        {
            cout << '\t' << extension << '\n';
        }

        // Itt állítjuk be a debuggert
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        //ha nem sikerül az instance létrehozása akkor err
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
        }
}
void VulkanContext::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw runtime_error("failed to set up debug messenger!");
    }
}

// vkEnumeratePhysicalDevices paraméterei röviden:
// - instance: annak a VkInstance-nek a névjegye, amelyhez tartozó fizikai eszközöket (GPU-kat) fel akarjuk sorolni.
// - pPhysicalDeviceCount: bemenetként a pPhysicalDevices tömb kapacitása, kimenetként a megtalált eszközök száma.
// - pPhysicalDevices: ha nullptr, akkor a függvény csak a darabszámot adja vissza pPhysicalDeviceCount-ban;
//   ha nem nullptr, akkor ebbe a tömbbe tölti be a VkPhysicalDevice handle-öket.
// Tipikus 2-lépéses minta:
// 1) első hívás pPhysicalDevices = nullptr → lekérdezzük, hány GPU érhető el (deviceCount),
// 2) foglalunk egy vektort deviceCount mérettel, majd újrahívjuk a függvényt, hogy kitöltse a listát.
void VulkanContext::pickPhysicalDevice(VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    //ez modosítja a devicecountot mert lekéri hány gpu van ezért adjuk ét referenciaként
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw runtime_error("failed to find GPUs with Vulkan support!");
    }
    vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    //ha talál jó eszközt akkor beállítja azt
    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device, surface))
        {
            physicalDevice = device;
            // Mentsük el az indexeket későbbi használatra
            queueIndices = findQueueFamilies(device, surface);
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw runtime_error("failed to find a suitable GPU!");
    }
}

void VulkanContext::createLogicalDevice(VkSurfaceKHR surface) {
    // Közvetlenül a tagváltozót használjuk, amit a pickPhysicalDevice elmentett
    QueueFamilyIndices indices = queueIndices;

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //jelenleg ürest theát minden érték VK_FALSEal inicializáljuk
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //megmondja melyik queue familyt akarjuk használni
    //hány queue familyt akarunk használni
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    //milyen eszköz funkciókat akarunk használni
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void VulkanContext::createCommandPool() {
    // A pickPhysicalDevice-ben elmentett indexeket használjuk
    QueueFamilyIndices indices = queueIndices; // Használjuk az elmentett indexet

    VkCommandPoolCreateInfo poolInfo{}; // Létrehozunk egy struktúrát a command pool beállításaihoz
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; // Megadjuk a struktúra típusát Vulkan számára
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Lehetővé tesszük, hogy a command buffer-eket egyenként újra lehessen rögzíteni
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value(); // A pool a grafikus queue family-hoz lesz kötve

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) { // Létrehozzok a command pool-t a Vulkan API-val
        throw std::runtime_error("failed to create command pool!"); // Hibakezelés, ha a létrehozás nem sikerül
    }
}

// --- Eszköz-választó segédfüggvények ---

// Ellenőrzi, hogy egy fizikai eszköz (GPU) alkalmas-e: queue family-k, extension-ök és swap chain támogatottságát vizsgálja
bool VulkanContext::isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    // Lekérdezi a graphics és present queue family indexeket
    QueueFamilyIndices indices = findQueueFamilies(dev, surface);
    // Ellenőrzi, hogy a GPU támogatja-e a szükséges extension-öket (pl. VK_KHR_swapchain)
    bool extensionsSupported = checkDeviceExtensionSupport(dev);

    // Swap chain megfelelőséget csak akkor ellenőrizzük, ha az extension elérhető
    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        // Lekérdezi a swap chain támogatási adatokat (capabilities, formats, presentModes)
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(dev, surface);
        // Ellenőrzi, hogy van-e legalább 1 formátum ÉS 1 prezentációs mód
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // GPU csak akkor alkalmas, ha minden kritérium teljesül
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

//kitöltjük a struktúrát hány családodt akarunk használni és visszaadjuk
QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    // Ez a struct kerül majd visszaadásra, hogy a meghatározott queue family indexeket tartalmazza

    //ezt valós helyzetbe le kell kérni de általában 0||1 3080 gpunál jónak kell lennie
    uint32_t queueFamilyCount = 0;
    //lekérdezzük a queue familyk számát modosítja a queueFamilyCountot
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    //a modosított queueFamilyCount alapján létrehozunk egy vektort amibe be fogjuk tölteni a queue familyket
    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());


    //megmondom ,hogy melyik queue family a graphics queue
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        //itt a & az biwise műveletet jelenti (és művelet) ha a queueflags ben benne van a graphics bit akkor true lesz
        //a VK_QUEUE_GRAPHICS_BIT egy konstans
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

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
    // Visszaadjuk a struct-ot, amiben a graphics queue family index van
}

// Röviden: ellenőrzi, hogy az adott fizikai eszköz (GPU) támogatja-e az általunk kért
// eszköz-kiterjesztéseket (pl. swapchain). Ha minden szükséges kiterjesztés elérhető → true,
// különben → false.
bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice dev) {
    // 1) Lekérdezzük, hány eszköz-kiterjesztés érhető el ezen a fizikai eszközön
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

    // 2) Lefoglaljuk a listát és betöltjük az elérhető kiterjesztések adatait
    vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

    // 3) A korábban meghatározott (szükséges) kiterjesztéseket halmazba tesszük
    //    (a deviceExtensions tagváltozóból)
    set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    // 4) Minden megtalált (elérhető) kiterjesztést kihúzunk a szükségesek közül
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    // 5) Ha nem maradt elvárt kiterjesztés a halmazban, akkor minden támogatott
    return requiredExtensions.empty();
}

// Lekérdezi a fizikai eszköz swap chain támogatási adatait
SwapChainSupportDetails VulkanContext::querySwapChainSupport(VkPhysicalDevice dev, VkSurfaceKHR surf) {
    SwapChainSupportDetails details;

    // Lekéri a fizikai eszköz és a felület közötti swap chain képességeket (méret, képkockák száma stb.)
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surf, &details.capabilities);

    uint32_t formatCount;
    // Lekérdezi, hányféle színformátumot támogat a felület (csak a számot)
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &formatCount, nullptr);

    if (formatCount != 0)
    {
        // Lefoglalja a formátumokat tároló vektort a megfelelő méretre
        details.formats.resize(formatCount);

        // Lekéri a konkrét színformátumokat és betölti őket a vektorba
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    // Lekérdezi, hányféle prezentációs módot (present mode) támogat a felület
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        // Lefoglalja a prezentációs módokat tároló vektort
        details.presentModes.resize(presentModeCount);

        // Lekéri a konkrét prezentációs módokat (pl. FIFO, MAILBOX stb.)
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &presentModeCount, details.presentModes.data());
    }


    // Visszaadja az összegyűjtött információkat (képességek + formátumok)
    return details;
}

// --- Debugger segédfüggvények ---

bool VulkanContext::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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

// Visszaadja a Vulkan instance-hez szükséges (engedélyezendő) extensionök listáját (GLFW által kért + opcionális debug utils).
std::vector<const char*> VulkanContext::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }


    return extensions;
}

void VulkanContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

// Megkeresi a GPU-n a kért tulajdonságoknak megfelelő memória típust
uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // 'typeFilter' (bitmaszk) jelzi, hogy melyik memória típusok *lehetnek* jók
        // 'properties' (bitmaszk) jelzi, hogy mely tulajdonságokkal kell rendelkeznie (pl. CPU-látható)
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

// Létrehoz egy Vulkan buffert és memóriát allokál neki
void VulkanContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // 1. Buffer létrehozása (még memória nélkül)
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    // 2. Memóriaigény lekérdezése
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // 3. Memória allokálása
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    // 4. Memória hozzárendelése a bufferhez
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

// Létrehoz egy parancspuffert, elindítja, futtatja a parancsokat, leállítja és elküldi
void VulkanContext::executeSingleTimeCommands(std::function<void(VkCommandBuffer)> commandFunction) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Futtatjuk a kapott parancsokat (pl. másolás)
    commandFunction(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue); // Megvárjuk, amíg a másolás befejeződik

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// Átmásolja az adatot egyik bufferből a másikba (staging -> device)
void VulkanContext::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    executeSingleTimeCommands([&](VkCommandBuffer commandBuffer) {
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    });
}

// Létrehoz egy 2D-s képet (pl. Depth Buffer)
void VulkanContext::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
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

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

// Létrehoz egy képnézetet (ImageView)
VkImageView VulkanContext::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
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