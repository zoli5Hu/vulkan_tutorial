// VulkanCore/VulkanSwapchain.cpp
#include "VulkanSwapchain.h"
#include <stdexcept>

using namespace std;

VulkanSwapchain::VulkanSwapchain() : context(nullptr), swapChain(VK_NULL_HANDLE) {
    // Konstruktor: inicializáljuk a pointereket és handle-öket
}

VulkanSwapchain::~VulkanSwapchain() {
    // Destruktor (a tényleges takarítás a cleanup()-ban lesz)
}

void VulkanSwapchain::create(VulkanContext* ctx, VkSurfaceKHR surface, GLFWwindow* window) {
    // Elmentjük a context pointert
    this->context = ctx;

    // Létrehozzuk a swapchain-t és az image view-kat
    createSwapChain(surface, window);
    createImageViews();
}

void VulkanSwapchain::cleanup() {
    // Felszabadítjuk az imagevieweket
    for (auto imageView : swapChainImageViews)
    {
        vkDestroyImageView(context->getDevice(), imageView, nullptr);
    }

    // Felszabadítjuk a swapchaint
    vkDestroySwapchainKHR(context->getDevice(), swapChain, nullptr);
}


// --- Privát segédfüggvények (áthelyezve a main.cpp-ből) ---

// Létrehozza a swap chain-t, amely a képernyőre kerülő képek puffereit kezeli
void VulkanSwapchain::createSwapChain(VkSurfaceKHR surface, GLFWwindow* window)
{
    /*
    // Lekérdezi a fizikai eszköz swap chain támogatási adatait
    // (capabilities: képességek, formats: színformátumok, presentModes: megjelenítési módok)
    */
    SwapChainSupportDetails swapChainSupport = context->querySwapChainSupport(context->getPhysicalDevice(), surface);
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
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

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
    QueueFamilyIndices indices = context->getQueueFamilies();
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

    if (vkCreateSwapchainKHR(context->getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(context->getDevice(), swapChain, &imageCount, nullptr);
    // Lekérdezi a swap chain-ben lévő képek számát
    swapChainImages.resize(imageCount); // Átméretezi a vektort a képek számára
    vkGetSwapchainImagesKHR(context->getDevice(), swapChain, &imageCount, swapChainImages.data());
    // Betölti a swap chain képeket a vektorba

    swapChainImageFormat = surfaceFormat.format; // Elmenti a választott színformátumot későbbi használatra
    swapChainExtent = extent; // Elmenti a swap chain felbontását későbbi használatra
}

void VulkanSwapchain::createImageViews()
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

        if (vkCreateImageView(context->getDevice(), &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
        {
            throw runtime_error("failed to create image views!");
        }
    }
}

// Kiválasztja a legjobb swap surface formátumot az elérhető formátumok közül
// Preferált: B8G8R8A8_SRGB színformátum + SRGB_NONLINEAR színtér, egyébként az első elérhető
VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes)
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
VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
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