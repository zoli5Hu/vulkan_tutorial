#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <vector>

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

    const std::vector<const char*> validationLayers = {
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

        std::vector<VkLayerProperties> availableLayers(layerCount);
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
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
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
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createInstance()
    {
        //ellenőrzi hogy a layerek elérhetőek e
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
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
            throw std::runtime_error("failed to create instance!");
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
            throw std::runtime_error("failed to create instance!");
        }
    }

    //visszadja hogy van e kiválasztot queue family ami megfelel a gpu nak
    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);

        return indices.isComplete();
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
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }


    struct QueueFamilyIndices
    {
        //jelenleg csak 1 változó de a struktúra hasznos lesz ha többet is hjozzá szeretnénk adni mert akkor egyszerűbb lesz visszaadni
        //azért adjuk meg optionalnak mert lehet ,hogy nincs is ilyen queue family a gpu-n (pl csak compute van) és az uint32 csak pozitív értékeket tud tárolni
        //ezért nemtudjuk ez lerendezn ia -1 értékkel
        std::optional<uint32_t> graphicsFamily; // Itt tároljuk a graphics queue family indexét

        bool isComplete()
        {
            return graphicsFamily.has_value();
        }
    };

    //kitöltjük a struktúrát hány családodt akarunk használni és visszaadjuk
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        // Ez a struct kerül majd visszaadásra, hogy a meghatározott queue family indexeket tartalmazza

        // // optional-t használsz a logikai ellenőrzéshez, hogy van-e érték
        // std::optional<uint32_t> graphicsFamily;
        // //boolalpha szövegtént jeleniti meg a true/false-t
        // std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl;
        // // false, mert még nincs érték hozzárendelve
        // //ezt valós helyzetbe le kell kérni de általában 0||1 3080 gpunál jónak kell lennie
        // graphicsFamily = 0;
        // // Például az első queue family (index 0) megfelel a graphics queue-nak
        //
        // std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl;
        // // true, most már van érték
        //
        // // FONTOS: itt még **nem töltöd ki a QueueFamilyIndices struct-ot**, csak az optional-t használtad.
        // // A Vulkan logikában így kellene:

        // //ezt valós helyzetbe le kell kérni de általában 0||1 3080 gpunál jónak kell lennie
        uint32_t queueFamilyCount = 0;
        //lekérdezzük a queue familyk számát modosítja a queueFamilyCountot
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        //a modosított queueFamilyCount alapján létrehozunk egy vektort amibe be fogjuk tölteni a queue familyket
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        //megmondom ,hogy melyik queue family a graphics queue
        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            //itt a & az biwise műveletet jelenti (és művelet) ha a queueflags ben benne van a graphics bit akkor true lesz
            //a VK_QUEUE_GRAPHICS_BIT egy konstans
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }
            if (indices.isComplete()) {
                break;
            }

            i++;
        }


        return indices;
        // Visszaadjuk a struct-ot, amiben a graphics queue family index van
    }


    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        // A queueCount azt adja meg, hogy hány queue-t hozunk létre ebből a queue family-ből.
        // Itt 1-et állítunk be, mert egyetlen grafikus queue bőven elég az alap rendereléshez.
        // Ha párhuzamos (multi-threaded) renderelést vagy külön compute queue-kat akarnánk,
        // akkor ezt az értéket lehetne 2-re vagy több-re növelni.
        //itt a már kiválasztott családból mennyit akarunk használni
        queueCreateInfo.queueCount = 1;
        //1 a legmmagasabb
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        //jelenleg ürest theát minden érték VK_FALSEal inicializáljuk
        VkPhysicalDeviceFeatures deviceFeatures{};
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        //megmondja melyik queue familyt akarjuk használni
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        //hány queue familyt akarunk használni
        createInfo.queueCreateInfoCount = 1;
        //milyen eszköz funkciókat akarunk használni
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);


    }




    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
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
        //felszabadítja a logikai eszközt
        vkDestroyDevice(device, nullptr);

    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
