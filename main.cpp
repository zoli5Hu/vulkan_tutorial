#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <GLFW/glfw3native.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
using namespace std;

//itt ugye drivernek már megmondtuk a createifoban hogy az instance használja de az extension még nincs betöltve cska az engedély van megadva itt betöltjük
//ez egy proxi/wrapper fügvény ami feladatot tovább adja vagy dinamikusan betölti itt meghívom az origin vulkan fgt plusz logolok vagy hibát hárítok el
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    //lekéri az extension címét ha sikerül (itt castolunk hogy fix jó objektumot kapjunk vissza)
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

//ugyanúgy proxi fg betölti a debug felszabadítót
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSwapchainKHR swapChain;
    vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkPipelineLayout pipelineLayout;

    //swapchan setup start extension enable
    const vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    const vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    vector<VkImageView> swapChainImageViews;


#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
    bool checkValidationLayerSupport()
    {
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

    vector<const char*> getRequiredExtensions()
    {
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
        void* pUserData)
    {
        cerr << "validation layer: " << pCallbackData->pMessage << endl;
        //VkBool32 visszatérési értéke
        return VK_FALSE;
    }


    // Visszaadja a Vulkan instance-hez szükséges (engedélyezendő) extensionök listáját (GLFW által kért + opcionális debug utils). Ezek csak akkor lesznek aktívak, ha bekerülnek a createInfo-ba és az vkCreateInstance sikeres.

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw runtime_error("failed to set up debug messenger!");
        }
    }

    void createInstance()
    {
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
        //
        // uint32_t glfwExtensionCount = 0;
        // //ez egy tömb lesz a kiterjesztésnek ami kell
        // const char** glfwExtensions;
        // //lekérem a kiterjesztéseket
        // glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        //
        //
        //
        // //beállítom a createinfo nak
        // createInfo.enabledExtensionCount = glfwExtensionCount;
        // createInfo.ppEnabledExtensionNames = glfwExtensions;


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

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        //ha nem sikerül az instance létrehozása akkor err
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw runtime_error("failed to create instance!");
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
        cout << "available extensions 2:\n";

        for (const auto& extension : extensions)
        {
            cout << '\t' << extension << '\n';
        }


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

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw runtime_error("failed to create instance!");
        }
    }

    // Röviden: ellenőrzi, hogy az adott fizikai eszköz (GPU) támogatja-e az általunk kért
    // eszköz-kiterjesztéseket (pl. swapchain). Ha minden szükséges kiterjesztés elérhető → true,
    // különben → false.
    bool checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        // 1) Lekérdezzük, hány eszköz-kiterjesztés érhető el ezen a fizikai eszközön
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        // 2) Lefoglaljuk a listát és betöltjük az elérhető kiterjesztések adatait
        vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // 3) A korábban meghatározott (szükséges) kiterjesztéseket halmazba tesszük
        //    Példa: VK_KHR_swapchain
        set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        // 4) Minden megtalált (elérhető) kiterjesztést kihúzunk a szükségesek közül
        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        // 5) Ha nem maradt elvárt kiterjesztés a halmazban, akkor minden támogatott
        return requiredExtensions.empty();
    }


    // Ellenőrzi, hogy egy fizikai eszköz (GPU) alkalmas-e: queue family-k, extension-ök és swap chain támogatottságát vizsgálja
    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        // Lekérdezi a graphics és present queue family indexeket
        QueueFamilyIndices indices = findQueueFamilies(device);
        // Ellenőrzi, hogy a GPU támogatja-e a szükséges extension-öket (pl. VK_KHR_swapchain)
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        // Swap chain megfelelőséget csak akkor ellenőrizzük, ha az extension elérhető
        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            // Lekérdezi a swap chain támogatási adatokat (capabilities, formats, presentModes)
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            // Ellenőrzi, hogy van-e legalább 1 formátum ÉS 1 prezentációs mód
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // GPU csak akkor alkalmas, ha minden kritérium teljesül
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        // vkEnumeratePhysicalDevices paraméterei röviden:
        // - instance: annak a VkInstance-nek a névjegye, amelyhez tartozó fizikai eszközöket (GPU-kat) fel akarjuk sorolni.
        // - pPhysicalDeviceCount: bemenetként a pPhysicalDevices tömb kapacitása, kimenetként a megtalált eszközök száma.
        // - pPhysicalDevices: ha nullptr, akkor a függvény csak a darabszámot adja vissza pPhysicalDeviceCount-ban;
        //   ha nem nullptr, akkor ebbe a tömbbe tölti be a VkPhysicalDevice handle-öket.
        // Tipikus 2-lépéses minta:
        // 1) első hívás pPhysicalDevices = nullptr → lekérdezzük, hány GPU érhető el (deviceCount),
        // 2) foglalunk egy vektort deviceCount mérettel, majd újrahívjuk a függvényt, hogy kitöltse a listát.
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
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw runtime_error("failed to find a suitable GPU!");
        }
    }


    struct QueueFamilyIndices
    {
        //jelenleg csak 1 változó de a struktúra hasznos lesz ha többet is hjozzá szeretnénk adni mert akkor egyszerűbb lesz visszaadni
        //azért adjuk meg optionalnak mert lehet ,hogy nincs is ilyen queue family a gpu-n (pl csak compute van) és az uint32 csak pozitív értékeket tud tárolni
        //ezért nemtudjuk ez lerendezn ia -1 értékkel
        optional<uint32_t> graphicsFamily; // Itt tároljuk a graphics queue family indexét
        //annak a családnak az indexe amelyik támogatja a prezentációt (ablakra rajzolást)
        optional<uint32_t> presentFamily;


        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    //kitöltjük a struktúrát hány családodt akarunk használni és visszaadjuk
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        // Ez a struct kerül majd visszaadásra, hogy a meghatározott queue family indexeket tartalmazza

        // // optional-t használsz a logikai ellenőrzéshez, hogy van-e érték
        // optional<uint32_t> graphicsFamily;
        // //boolalpha szövegtént jeleniti meg a true/false-t
        // cout << boolalpha << graphicsFamily.has_value() << endl;
        // // false, mert még nincs érték hozzárendelve
        // //ezt valós helyzetbe le kell kérni de általában 0||1 3080 gpunál jónak kell lennie
        // graphicsFamily = 0;
        // // Például az első queue family (index 0) megfelel a graphics queue-nak
        //
        // cout << boolalpha << graphicsFamily.has_value() << endl;
        // // true, most már van érték
        //
        // // FONTOS: itt még **nem töltöd ki a QueueFamilyIndices struct-ot**, csak az optional-t használtad.
        // // A Vulkan logikában így kellene:

        // //ezt valós helyzetbe le kell kérni de általában 0||1 3080 gpunál jónak kell lennie
        uint32_t queueFamilyCount = 0;
        //lekérdezzük a queue familyk számát modosítja a queueFamilyCountot
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        //a modosított queueFamilyCount alapján létrehozunk egy vektort amibe be fogjuk tölteni a queue familyket
        vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());


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
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

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

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

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

    void createSurface()
    {
        //lérehozza a surface t a glfw segítségével
        //ez gyakorlatilag olyan mintha three.jsben a cnavast hoznám létre
        //a &surfice már inicializálásnál megkapja a base adatokat és itt hozzá kell csak kötni
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw runtime_error("failed to create window surface!");
        }
    }

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

    // Lekérdezi a fizikai eszköz swap chain támogatási adatait
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        // Lekéri a fizikai eszköz és a felület közötti swap chain képességeket (méret, képkockák száma stb.)
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        // Lekérdezi, hányféle színformátumot támogat a felület (csak a számot)
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            // Lefoglalja a formátumokat tároló vektort a megfelelő méretre
            details.formats.resize(formatCount);

            // Lekéri a konkrét színformátumokat és betölti őket a vektorba
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        // Lekérdezi, hányféle prezentációs módot (present mode) támogat a felület
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            // Lefoglalja a prezentációs módokat tároló vektort
            details.presentModes.resize(presentModeCount);

            // Lekéri a konkrét prezentációs módokat (pl. FIFO, MAILBOX stb.)
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }


        // Visszaadja az összegyűjtött információkat (képességek + formátumok)
        return details;
    }

    // Kiválasztja a legjobb swap surface formátumot az elérhető formátumok közül
    // Preferált: B8G8R8A8_SRGB színformátum + SRGB_NONLINEAR színtér, egyébként az első elérhető
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            // Ha megtaláljuk a preferált SRGB formátumot és színteret, azt választjuk
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        // Ha nincs preferált formátum, az első elérhető formátumot használjuk
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            //kikeressük a mailbox presaentation modot
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        //ha nicns akkor az alappal térünk vissza
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // Meghatározza a swap chain képek felbontását (szélesség és magasság pixelben)
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        // Ha a currentExtent.width != UINT32_MAX, akkor a window manager már meghatározta
        // az ideális felbontást, amit kötelezően használnunk kell
        //a numeric:limits az adott típus maximum értékeét adja vissza
        //általában a fejlesztők pl windowsnál megadják fix értéknek a maxopt hogy tudjam nem kell beállítani
        //dep l mobilnál nem adják meg és ott muszáj
        if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            // Ha a width == UINT32_MAX, akkor mi magunk választhatjuk meg a felbontást
            // az ablak tényleges mérete alapján
            int width, height;
            // Lekérdezzük az ablak framebuffer méretét pixelben (nem ablakméret!)
            // Ez különbözhet az ablak logikai méretétől high-DPI kijelzőkön
            glfwGetFramebufferSize(window, &width, &height);

            // Létrehozzuk a választott extent struktúrát, int-ről uint32_t-re castolva
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // Biztosítjuk, hogy a választott szélesség a megengedett tartományban legyen
            // (nem lehet kisebb a minimum-nál, nem lehet nagyobb a maximum-nál)
            actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width,
                                       capabilities.maxImageExtent.width);
            // Ugyanez a magasságra is
            actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height,
                                        capabilities.maxImageExtent.height);

            // Visszaadjuk a korrigált, érvényes felbontást
            return actualExtent;
        }
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
    }


    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());
        // Átméretezi a vektor tárolót, hogy minden swap chain képhez tartozzon egy image view
        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            // Végigiterál minden swap chain képen
            VkImageViewCreateInfo createInfo{}; // Létrehozza az image view konfigurációs struktúrát
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; // Beállítja a struktúra típusát
            createInfo.image = swapChainImages[i]; // Megadja, melyik képhez tartozik ez az image view
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 2D textúraként kezeli a képet (nem 1D, 3D vagy cube map)
            createInfo.format = swapChainImageFormat; // Beállítja a kép színformátumát (pl. B8G8R8A8_SRGB)
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Piros csatorna marad eredeti (nem cserélődik)
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY; // Zöld csatorna marad eredeti
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY; // Kék csatorna marad eredeti
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY; // Alfa csatorna marad eredeti
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            // Megadja, hogy színes képként kezeljük (nem depth/stencil)
            createInfo.subresourceRange.baseMipLevel = 0;
            // A legmagasabb felbontású mipmap szintet használjuk (0 = teljes felbontás)
            createInfo.subresourceRange.levelCount = 1; // Csak 1 mipmap szintet használunk (nincs mipmap lánc)
            createInfo.subresourceRange.baseArrayLayer = 0;
            // Az első tömb rétegtől kezdjük (VR/stereo renderingnél van jelentősége)
            createInfo.subresourceRange.layerCount = 1; // Csak 1 réteget használunk (nem VR, csak sima 2D kép)

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw runtime_error("failed to create image views!");
            }
        }
    }


    // Fájlt olvas be bájtokként és visszaadja egy vector<char>-ben
    static vector<char> readFile(const string& filename)
    {
        // Megnyitjuk a fájlt bináris módban, és a fájl végére állítjuk a read pointert
        //ios::ate végéről olvas be ez a tellg miatt csináljuk könnyeb ba mretét megadni
        //ios::binary binárisként kezeli a filet nincsbenne /n
        ifstream file(filename, ios::ate | ios::binary);
        // Ha nem sikerült megnyitni, dobunk egy kivételt
        if (!file.is_open())
        {
            throw runtime_error("failed to open file!");
        }
        // Lekérdezzük a fájl méretét a tellg() függvénnyel
        size_t fileSize = (size_t)file.tellg();
        // Létrehozunk egy vector<char>-t, ami a fájl bájtjait fogja tárolni
        vector<char> buffer(fileSize);
        // Visszaállítjuk a read pointert a fájl elejére
        file.seekg(0);
        // Beolvassuk a fájl tartalmát a buffer-be
        file.read(buffer.data(), fileSize);
        // Bezárjuk a fájlt
        file.close();
        // Visszaadjuk a beolvasott bájtokat
        return buffer;
    }

    VkShaderModule createShaderModule(const vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        //a vulkan 32 bites egészeket vár de az adatunk charban van ezért aztmondjuk hogy kezelje untkéént
        //itt a vektor garantálja nekem a memória igazítást
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    void createRenderPass() {
        /**
  * Ez a rész egy egyszerű Vulkan render pass alapját készíti elő:
  * beállítja a szín attachmentet, annak használatát és a subpass leírását.
  */
        VkAttachmentDescription colorAttachment{}; // A render pass egyik "attachment"-jének (képének) leírása
        colorAttachment.format = swapChainImageFormat; // A swapchain formátumát használja, hogy a képernyőre lehessen kirajzolni
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Nincs multisampling, 1 mintát használ pixelként
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // A subpass elején törli (clear) a szín buffert
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // A rajzolás után elmenti, hogy a képernyőre lehessen küldeni
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // A stencil értékeket nem használjuk, nem érdekes a betöltésük
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // A stencil értékeket nem mentjük el
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Kezdetben a kép layoutja ismeretlen (Vulkan majd beállítja)
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // A renderelés végén a kép készen áll megjelenítésre a képernyőn

        VkAttachmentReference colorAttachmentRef{}; // Hivatkozás a fent létrehozott attachmentre
        colorAttachmentRef.attachment = 0; // Ez az első (0. indexű) attachment az attachment tömbben
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // A subpass alatt szín bufferként használjuk

        VkSubpassDescription subpass{}; // Egy subpass leírása a render pass-on belül
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Ez egy grafikus subpass (nem compute)
        subpass.colorAttachmentCount = 1; // Egy szín attachmentet használ a subpass
        subpass.pColorAttachments = &colorAttachmentRef; // A korábban létrehozott colorAttachmentRef-et használja



    }


    void createGraphicsPipeline()
    {
            /*
             * Shader fájlok beolvasása
             * - vertShaderCode: vertex shader SPIR-V bytecode
             * - fragShaderCode: fragment shader SPIR-V bytecode
             */
            auto vertShaderCode = readFile("shaders/vert.spv");
            auto fragShaderCode = readFile("shaders/frag.spv");

            /*
             * Shader modulok létrehozása a Vulkan számára
             * - vertShaderModule: vertex shader modul
             * - fragShaderModule: fragment shader modul
             */
            VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
            VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

            /*
             * Vertex shader stage konfiguráció
             * - sType: struktúra típus megadása
             * - stage: megadja hogy ez vertex shader
             * - module: a shader modul referenciája
             * - pName: a shader belépési pontja (main függvény)
             */
            VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
            vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertShaderStageInfo.module = vertShaderModule;
            vertShaderStageInfo.pName = "main";

            /*
             * Fragment shader stage konfiguráció
             * - sType: struktúra típus megadása
             * - stage: megadja hogy ez fragment shader
             * - module: a shader modul referenciája
             * - pName: a shader belépési pontja (main függvény)
             */
            VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = fragShaderModule;
            fragShaderStageInfo.pName = "main";

            /*
             * Shader stage-ek tömbje a pipeline-nak
             * Tartalmazza mind a vertex, mind a fragment shader konfigurációját
             */
            VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};


            /*
             * Dinamikus állapotok listája
             * Olyan pipeline beállítások, amiket nem fix értékkel adunk meg, hanem futásidőben állítunk be
             * - VK_DYNAMIC_STATE_VIEWPORT: viewport mérete és pozíciója dinamikusan állítható
             * - VK_DYNAMIC_STATE_SCISSOR: scissor mérete és pozíciója dinamikusan állítható
             */
            vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };

            /*
             * Dinamikus állapotok konfigurációs struktúra
             * - sType: struktúra típus
             * - dynamicStateCount: hány dinamikus állapotot használunk
             * - pDynamicStates: pointer a dinamikus állapotok tömbjére
             */
            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();

            /*
             * Viewport state konfiguráció
             * A tényleges viewport és scissor értékeket majd később állítjuk be rajzoláskor (draw time)
             * - viewportCount: 1 viewport-ot használunk
             * - scissorCount: 1 scissor-t használunk
             */
            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            /*
             * Vertex input state konfiguráció
             * Megadja, hogyan kell értelmezni a vertex adatokat
             * - vertexBindingDescriptionCount: 0 (shaderben hardcode-oljuk a pozíciókat)
             * - pVertexBindingDescriptions: nullptr (nincs vertex buffer)
             * - vertexAttributeDescriptionCount: 0 (shaderben adjuk meg a pozíciókat)
             * - pVertexAttributeDescriptions: nullptr (nincs vertex buffer)
             */
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 0;
            vertexInputInfo.pVertexBindingDescriptions = nullptr;
            vertexInputInfo.vertexAttributeDescriptionCount = 0;
            vertexInputInfo.pVertexAttributeDescriptions = nullptr;

            /*
             * Input assembly state konfiguráció
             * Megadja, hogyan kell összerakni a vertexeket primitívekké
             * - topology: VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST (minden 3 vertex egy háromszög, nem megosztottak)
             * - primitiveRestartEnable: VK_FALSE (nem használjuk, csak strip/fan topológiánál hasznos)
             */
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssembly.primitiveRestartEnable = VK_FALSE;


        /*
        * Rasterization state konfiguráció
        * A raszterizáló beállításai, amely a geometriát fragmentekké (pixelekké) alakítja
        * - sType: struktúra típus
        * - depthClampEnable: VK_FALSE = látótávolságon kívüli elemeket eldobja (nem clampeli)
        * - rasterizerDiscardEnable: VK_FALSE = a raszterizálás nem kerül eldobásra (folytatódik)
        * - polygonMode: VK_POLYGON_MODE_FILL = a háromszögeket kitölti (nem wireframe/point) pl wireframméél alaakítás
        * - lineWidth: 1.0f = vonalvastagság (wireframe módnál számít)
        * - cullMode: VK_CULL_MODE_BACK_BIT = hátul lévő háromszögeket nem rajzolja (face culling)
        * - frontFace: VK_FRONT_FACE_CLOCKWISE = óramutató járása szerint vannak az elülső háromszögek
        * - depthBiasEnable: VK_FALSE = nem használ depth bias-t (shadow mapping-nél hasznos)
        * - depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor: opcionális depth bias értékek
        */
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        //most kikapcsoljuk még
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        //ha lenen stancilünk vagy depthünk itt kéne konfigurálni

            /*
             * Color blend attachment konfiguráció (egy render target színkeverési beállításai)
             * - colorWriteMask: mely színcsatornákat írjuk (R, G, B, A mind engedélyezve)
             * - blendEnable: VK_FALSE = nincs színkeverés (az új szín felülírja a régit)
             * - srcColorBlendFactor: forrás szín szorzója (ONE = 1.0, nincs hatása ha blendEnable=false)
             * - dstColorBlendFactor: cél szín szorzója (ZERO = 0.0, nincs hatása ha blendEnable=false)
             * - colorBlendOp: színkeverési művelet (ADD = összeadás)
             * - srcAlphaBlendFactor: forrás alfa szorzója
             * - dstAlphaBlendFactor: cél alfa szorzója
             * - alphaBlendOp: alfa keverési művelet
             */
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

            /*
             * Color blending globális konfiguráció (az összes framebuffer attachmentre vonatkozik)
             * - sType: struktúra típus
             * - logicOpEnable: VK_FALSE = logikai műveletek kikapcsolva (bitwise operációk mint AND, OR)
             * - logicOp: logikai művelet típusa (COPY = másolás, nincs hatása ha logicOpEnable=false)
             * - attachmentCount: hány attachment-et használunk (1 = egy render target)
             * - pAttachments: pointer az attachment konfigurációra
             * - blendConstants: globális keverési konstansok (RGBA, mindegyik 0.0)
             */
            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
            colorBlending.blendConstants[0] = 0.0f; // Optional
            colorBlending.blendConstants[1] = 0.0f; // Optional
            colorBlending.blendConstants[2] = 0.0f; // Optional
            colorBlending.blendConstants[3] = 0.0f; // Optional

        /*
         * Pipeline layout konfiguráció
         * A pipeline layout meghatározza, milyen uniform bufferek, descriptor set-ek és push constantok
         * érhetők el a shaderekben. Most egyiket sem használjuk, ezért minden 0/nullptr.
         * - sType: struktúra típus
         * - setLayoutCount: hány descriptor set layout-ot használunk (0 = nincs)
         * - pSetLayouts: pointer a descriptor set layout-okra (nullptr = nincs)
         * - pushConstantRangeCount: hány push constant range-t használunk (0 = nincs)
         * - pPushConstantRanges: pointer a push constant range-ekre (nullptr = nincs)
         * - muszáj implementálnunk még akkor is ha üres ez a cpu és shader között híd
         */
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        /*
         * Pipeline layout létrehozása
         * Ha sikertelen, runtime_error kivételt dob
         */
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }



            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            // Felszabadítja a fragment shader modult (pipeline létrehozás után már nem kell)
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            // Felszabadítja a vertex shader modult (pipeline létrehozás után már nem kell)

    }


    // Létrehozza a swap chain-t, amely a képernyőre kerülő képek puffereit kezeli
    void createSwapChain()
    {
        /*
        // Lekérdezi a fizikai eszköz swap chain támogatási adatait
        // (capabilities: képességek, formats: színformátumok, presentModes: megjelenítési módok)
        */
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
        /*
        // Kiválasztja a legjobb színformátumot a támogatott formátumok közül
        // (preferált: B8G8R8A8_SRGB + SRGB_NONLINEAR színtér)
        */
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        /*
        // Kiválasztja a prezentációs módot (VSync beállítás)
        // (preferált: MAILBOX = triple buffering, fallback: FIFO = VSync)
        */
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        /*
        // Meghatározza a swap chain képek felbontását (szélesség és magasság pixelben)
        // Az ablak tényleges méretéhez igazítva, a GPU korlátai között
        */
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
        //ez jó mert nem fog vilkdzni ak ép double buffering általába + 1 kép = 3 kép
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        /*// Itt a swapChainSupport.capabilities.maxImageCount > 0 feltétel NEM azt jelenti, hogy 0 darab képet lehet.
        // A 0 azt jelzi, hogy NINCS felső korlát – vagyis bármennyi képet létrehozhatunk.
        // Ezért itt azt mondjuk:
        // ha VAN felső korlát (maxImageCount > 0) és a mi általunk kért képszám TÚLLÉPNÉ azt,
        // akkor állítsuk vissza a maximumra.*/
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // 🔹 Swapchain létrehozásához szükséges információkat feltöltjük
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; // Struktúratípus beállítása
        createInfo.surface = surface; // A felület, amire rajzolunk
        createInfo.minImageCount = imageCount; // Képek száma a láncban
        createInfo.imageFormat = surfaceFormat.format; // Kép színformátuma
        createInfo.imageColorSpace = surfaceFormat.colorSpace; // Színtér (pl. SRGB)
        createInfo.imageExtent = extent; // Felbontás (szélesség, magasság)
        createInfo.imageArrayLayers = 1; // 1 = normál 2D kép (nem VR)
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // A képeket színes rendercélként használjuk

        /*
        // 🔹 Lekérdezzük a GPU queue family indexeit
        // A QueueFamilyIndices egy struktúra, ami tartalmazhat opcionális graphicsFamily és presentFamily indexeket
        */
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        /*// 🔹 Két indexet készítünk: graphics és presentation queue családok
        // Ezek azok a sorok a GPU-n, amiken majd rajzolni és képernyőre küldeni fogunk*/
        uint32_t queueFamilyIndices[] = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        // 🔹 Ellenőrizzük, hogy a grafikai és a prezentációs queue ugyanaz-e
        if (indices.graphicsFamily != indices.presentFamily)
        {
            /*
            / 🔹 Különböző queue family-k használata esetén:
            // VK_SHARING_MODE_CONCURRENT: a képeket egyszerre több queue family is használhatja
            // explicit ownership átvitel nélkül. Ez kényelmes, ha két queue-t használunk.
            */
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2; // Hány queue family használja a képeket
            createInfo.pQueueFamilyIndices = queueFamilyIndices; // Melyik queue family-k között osztozik
        }
        else
        {
            /*
            // 🔹 Ha a grafikai és prezentációs queue ugyanaz:
            // VK_SHARING_MODE_EXCLUSIVE: az egyik queue “birtokolja” a képet,
            // nincs szükség explicit ownership átadásra, jobb teljesítmény.
            */
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // opcionális, mert csak egy queue van
            createInfo.pQueueFamilyIndices = nullptr; // opcionális, nincs több queue
        }

        /*// Ha akarunk transzformációt alkalmazni (pl. 90 fokos forgatás), itt állíthatjuk be
        // Most az aktuális/alapértelmezett transzformációt használjuk (nincs forgatás)*/
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        /*// Beállítja, hogyan keverődjön az ablak alfa csatornája más ablakokkal
        // OPAQUE_BIT: teljesen átlátszatlan, figyelmen kívül hagyja az alfa értékeket*/
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode; // Prezentációs mód beállítása (pl. FIFO, MAILBOX - VSync típusok)
        createInfo.clipped = VK_TRUE;
        // Ha más ablak takarja a képet, azokat a pixeleket nem rendereli (teljesítmény optimalizáció)
        createInfo.oldSwapchain = VK_NULL_HANDLE; // optimalizálási dolgok rasztrizálásnál

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        // Lekérdezi a swap chain-ben lévő képek számát
        swapChainImages.resize(imageCount); // Átméretezi a vektort a képek számára
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        // Betölti a swap chain képeket a vektorba

        swapChainImageFormat = surfaceFormat.format; // Elmenti a választott színformátumot későbbi használatra
        swapChainExtent = extent; // Elmenti a swap chain felbontását későbbi használatra
    }

    void initWindow()
    {
        glfwInit();
        //ezzel lekapcsolódik az opngl contex apiról
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //ez megakadályozza az átméretezést
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        //a paraméterek : szélesség, magasság, cím, monitor, megosztott ablak(ez csak opnglben számít)
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            //ez ellenőrzi hogy be akarom e csukni az a ablakot (vagy más események)
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        //felszabadítja a pipeline layoutot
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        //felszabadítja az imagevieweket
        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }
        //felszabadítja a swapchaint
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        //felszabadítja a logikai eszközt
        vkDestroyDevice(device, nullptr);
        //felszabadítja a surface t
        vkDestroySurfaceKHR(instance, surface, nullptr);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        //felszabadítja az instance t
        vkDestroyInstance(instance, nullptr);
        //memória felszabadítás és leállítás
        //cscak ablak
        glfwDestroyWindow(window);
        //minden ami a könyvtárban van
        glfwTerminate();
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
