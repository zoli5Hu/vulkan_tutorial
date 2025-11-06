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
    //swapchan setup start extension enable
    const vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    const vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    VkDebugUtilsMessengerEXT debugMessenger;

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
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            // Ha megtaláljuk a preferált SRGB formátumot és színteret, azt választjuk
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        // Ha nincs preferált formátum, az első elérhető formátumot használjuk
        return availableFormats[0];
    }


    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
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
