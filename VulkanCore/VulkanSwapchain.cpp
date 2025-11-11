/**
 * @file VulkanSwapchain.cpp
 * @brief Megvalósítja a VulkanSwapchain osztályt.
 *
 * Ez az osztály felelős a Vulkan képcserélő lánc (Swap Chain) és a hozzá tartozó
 * képnézetek (Image Views) létrehozásáért és kezeléséért. A Swap Chain biztosítja
 * a renderelt képek tárolását és megjelenítésre való átadását az ablakkezelő rendszernek.
 */
// VulkanCore/VulkanSwapchain.cpp
#include "VulkanSwapchain.h"
#include <stdexcept>

using namespace std;

/**
 * @brief Konstruktor.
 * Inicializálja a context-et nullptr-re és a swapChain leírót VK_NULL_HANDLE-re.
 */
VulkanSwapchain::VulkanSwapchain() : context(nullptr), swapChain(VK_NULL_HANDLE) {
}

/**
 * @brief Destruktor.
 */
VulkanSwapchain::~VulkanSwapchain() {
}

/**
 * @brief Létrehozza a Swapchain-t és a hozzá tartozó Image View-kat.
 *
 * @param ctx A Vulkan környezet mutatója.
 * @param surface A renderelési felület (VkSurfaceKHR).
 * @param window A GLFW ablak (a felbontás lekéréséhez).
 */
void VulkanSwapchain::create(VulkanContext* ctx, VkSurfaceKHR surface, GLFWwindow* window) {
    this->context = ctx;

    createSwapChain(surface, window);
    createImageViews();
}

/**
 * @brief Felszabadítja a Swapchain erőforrásait.
 * Először az Image View-kat, majd magát a Swap Chain-t semmisíti meg.
 */
void VulkanSwapchain::cleanup() {
    // Képnézetek felszabadítása
    for (auto imageView : swapChainImageViews)
    {
        vkDestroyImageView(context->getDevice(), imageView, nullptr);
    }

    // Swapchain felszabadítása
    vkDestroySwapchainKHR(context->getDevice(), swapChain, nullptr);
}

/**
 * @brief Létrehozza a Vulkan Swap Chain (képcserélő lánc) objektumot.
 *
 * Lekérdezi az eszköz képességeit (formátum, megjelenítési mód, felbontás),
 * kiválasztja az optimális beállításokat, és létrehozza a VkSwapchainKHR objektumot.
 *
 * @param surface A renderelési felület.
 * @param window A GLFW ablak.
 */
void VulkanSwapchain::createSwapChain(VkSurfaceKHR surface, GLFWwindow* window)
{
    // 1. Swap Chain támogatás lekérdezése a VulkanContext segítségével
    SwapChainSupportDetails swapChainSupport = context->querySwapChainSupport(context->getPhysicalDevice(), surface);

    // 2. Optimális beállítások kiválasztása
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    // 3. Képek számának meghatározása a láncban
    // Általában a minimum + 1-et kérünk (pl. triple buffering-hez), de nem léphetjük túl a maximumot.
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // 4. VkSwapchainCreateInfoKHR struktúra kitöltése
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // Nem sztereoszkópikus (nem 3D szemüveges)
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // A képeket szín csatolmányként (Color Attachment) fogjuk használni

    // 5. Várólista családok (Queue Families) kezelése
    QueueFamilyIndices indices = context->getQueueFamilies();
    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    // Ha a grafikus és a megjelenítési várólista eltérő...
    if (indices.graphicsFamily != indices.presentFamily)
    {
        // ...akkor CONCURRENT (párhuzamos) módot használunk, ami lassabb, de nem igényel manuális tulajdonosváltást.
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        // ...ha megegyeznek (ez a gyakoribb dedikált GPU-knál), EXCLUSIVE módot használunk (jobb teljesítmény).
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    // 6. Egyéb beállítások
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // Nincs extra transzformáció (pl. forgatás)
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Az ablak nem átlátszó
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // Elfogadjuk az ablak által takart pixelek eldobását
    createInfo.oldSwapchain = VK_NULL_HANDLE; // Nincs régi swapchain (nem átméretezés)

    // 7. Swap Chain létrehozása
    if (vkCreateSwapchainKHR(context->getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw runtime_error("failed to create swap chain!");
    }

    // 8. Swap Chain képek leíróinak (handles) lekérése
    vkGetSwapchainImagesKHR(context->getDevice(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(context->getDevice(), swapChain, &imageCount, swapChainImages.data());

    // 9. Formátum és felbontás elmentése későbbi használatra
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

/**
 * @brief Létrehozza a képnézeteket (Image Views) a Swap Chain képeihez.
 *
 * Az Image View határozza meg, hogyan kell a Vulkan-nak értelmeznie a képet
 * (pl. milyen formátumú, 2D textúra-e stb.). Minden VkImage-hez kell egy VkImageView.
 */
void VulkanSwapchain::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i]; // A swapchain által létrehozott kép
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        // Komponens átkötés (swizzle) - alapértelmezett (Identity)
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // Al-erőforrás tartomány (Subresource Range)
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Szín aspektus
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(context->getDevice(), &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
        {
            throw runtime_error("failed to create image views!");
        }
    }
}

/**
 * @brief Kiválasztja az optimális felületi formátumot (Surface Format).
 *
 * Előnyben részesíti a B8G8R8A8 SRGB formátumot a pontos színmegjelenítés érdekében.
 *
 * @param availableFormats A GPU által támogatott formátumok listája.
 * @return A kiválasztott VkSurfaceFormatKHR.
 */
VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        // Keressük a 8 bites, SRGB, nemlineáris színterű formátumot.
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    // Ha nem találjuk, visszatérünk az elsővel (gyakran ez is jó).
    return availableFormats[0];
}

/**
 * @brief Kiválasztja az optimális megjelenítési módot (Present Mode).
 *
 * Előnyben részesíti a VK_PRESENT_MODE_MAILBOX_KHR (triple buffering) módot
 * az alacsony késleltetés és a szakadozásmentes (tear-free) megjelenítés érdekében.
 *
 * @param availablePresentModes A GPU által támogatott módok listája.
 * @return A kiválasztott VkPresentModeKHR.
 */
VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        // Mailbox (Triple Buffering): Amikor a GPU kész, a legújabb képet veszi el.
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }
    // Tartalék (Fallback): VK_PRESENT_MODE_FIFO_KHR (V-Sync, Double Buffering). Ez mindig garantáltan elérhető.
    return VK_PRESENT_MODE_FIFO_KHR;
}

/**
 * @brief Kiválasztja az optimális felbontást (Extent) a Swap Chain számára.
 *
 * Ha az ablakkezelő engedi (max()), akkor a GLFW framebuffer méretét használja.
 *
 * @param capabilities A GPU képességei.
 * @param window A GLFW ablak.
 * @return A kiválasztott VkExtent2D (felbontás).
 */
VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
    // Ha a képességek 'currentExtent'-je már definiált (nem a max uint32_t),
    // akkor azt kell használnunk (pl. egyes mobil platformokon).
    if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        // Különben lekérdezzük az ablak aktuális (pixelben mért) felbontását.
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // Biztosítjuk, hogy a felbontás a GPU által támogatott minimum és maximum határok közé essen.
        actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width,
                                   capabilities.maxImageExtent.width);
        actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height,
                                    capabilities.maxImageExtent.height);

        return actualExtent;
    }
}