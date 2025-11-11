/**
 * @file VulkanSwapchain.h
 * @brief Deklarálja a VulkanSwapchain osztályt.
 *
 * Ez az osztály felelős a Vulkan képcserélő lánc (Swap Chain) és a hozzá tartozó
 * képnézetek (Image Views) létrehozásáért és kezeléséért. A Swap Chain egy
 * puffersorozatot (képeket) biztosít, amelyekbe renderelhetünk, és amelyek
 * azután megjeleníthetők a képernyőn.
 */
// VulkanCore/VulkanSwapchain.h
#pragma once

#include "VulkanContext.h" // Szükséges a VulkanContext-re való hivatkozáshoz

/**
 * @class VulkanSwapchain
 * @brief A Vulkan képcserélő láncát (Swap Chain) és a hozzá tartozó erőforrásokat kezelő osztály.
 */
class VulkanSwapchain {
public:
    /**
     * @brief Konstruktor.
     */
    VulkanSwapchain();

    /**
     * @brief Destruktor.
     */
    ~VulkanSwapchain();

    /**
     * @brief Létrehozza a Swapchain-t és a hozzá tartozó Image View-kat.
     *
     * @param context A Vulkan környezet mutatója (eszköz, stb.).
     * @param surface A renderelési felület (VkSurfaceKHR), amelyre a swapchain rajzolni fog.
     * @param window A GLFW ablak (a felbontás és egyéb ablak-specifikus adatok lekéréséhez).
     */
    void create(VulkanContext* context, VkSurfaceKHR surface, GLFWwindow* window);

    /**
     * @brief Felszabadítja az összes lefoglalt Swapchain erőforrást (Image View-k és maga a Swapchain).
     */
    void cleanup();

    // --- Getter függvények a Swapchain tulajdonságainak eléréséhez ---

    /** @return A VkSwapchainKHR leírója. */
    VkSwapchainKHR getSwapchain() const { return swapChain; }
    /** @return A Swapchain képeinek formátuma (pl. VK_FORMAT_B8G8R8A8_SRGB). */
    VkFormat getImageFormat() const { return swapChainImageFormat; }
    /** @return A Swapchain képeinek felbontása (szélesség, magasság). */
    VkExtent2D getExtent() const { return swapChainExtent; }
    /** @return Konstans referencia a Swapchain képnézeteit (ImageViews) tartalmazó vektorra. */
    const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }
    /** @return A Swapchain-ben lévő képek száma. */
    uint32_t getImageCount() const { return static_cast<uint32_t>(swapChainImages.size()); }

private:
    /** @brief A Vulkan Swap Chain objektum leírója. */
    VkSwapchainKHR swapChain;
    /** @brief A Swap Chain által birtokolt VkImage-ek (képek) listája. */
    std::vector<VkImage> swapChainImages;
    /** @brief A Swap Chain képeinek formátuma. */
    VkFormat swapChainImageFormat;
    /** @brief A Swap Chain képeinek felbontása (mérete). */
    VkExtent2D swapChainExtent;
    /** @brief A Swap Chain képeihez tartozó képnézetek (Image Views) listája. */
    std::vector<VkImageView> swapChainImageViews;

    /** @brief Mutató a központi Vulkan környezetre. */
    VulkanContext* context;

    // --- Privát inicializáló segédfüggvények ---

    /**
     * @brief Létrehozza magát a VkSwapchainKHR objektumot és lekéri a képeit.
     */
    void createSwapChain(VkSurfaceKHR surface, GLFWwindow* window);

    /**
     * @brief Létrehozza a VkImageView-kat a swapChainImages vektorban lévő VkImage-ekhez.
     */
    void createImageViews();

    // --- Privát képesség-választó segédfüggvények ---

    /**
     * @brief Kiválasztja a legmegfelelőbb felületi formátumot az elérhetők közül.
     * (Előnyben részesíti az SRGB-t a jobb színhelyesség érdekében).
     */
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    /**
     * @brief Kiválasztja a legmegfelelőbb megjelenítési módot (Present Mode).
     * (Előnyben részesíti a MAILBOX-ot (triple buffering) a FIFO (v-sync) helyett).
     */
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    /**
     * @brief Meghatározza a Swap Chain felbontását (Extent).
     * (Általában az ablak méretét használja, figyelembe véve a GPU korlátait).
     */
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
};