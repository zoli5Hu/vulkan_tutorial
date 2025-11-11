/**
 * @file VulkanPipeline.h
 * @brief Deklarálja a VulkanPipeline osztályt.
 *
 * Ez az osztály felelős a Vulkan grafikus pipeline (állapotgép) és a kapcsolódó
 * objektumok (Render Pass, Pipeline Layout, Framebufferek) létrehozásáért és kezeléséért.
 * Két pipeline-t kezel: egyet a normál kitöltött (solid) rendereléshez és egyet a
 * drótvázas (wireframe) rendereléshez.
 */
// VulkanCore/VulkanPipeline.h
#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include <fstream> // Fájl olvasáshoz (shaderek)

/**
 * @class VulkanPipeline
 * @brief A Vulkan grafikus pipeline állapotait és erőforrásait összefogó osztály.
 */
class VulkanPipeline {
public:
    /**
     * @brief Konstruktor.
     */
    VulkanPipeline();

    /**
     * @brief Destruktor.
     */
    ~VulkanPipeline();

    /**
     * @brief Létrehozza az összes pipeline-hoz kapcsolódó Vulkan objektumot.
     *
     * Meghívja a belső createRenderPass, createGraphicsPipeline, createWireframePipeline
     * és createFramebuffers metódusokat.
     *
     * @param context A Vulkan környezet mutatója.
     * @param swapchain A swapchain mutatója (formátum és extent lekéréséhez).
     * @param depthImageView A mélységi kép nézete (framebufferhez).
     * @param depthFormat A mélységi kép formátuma (render pass-hoz).
     */
    void create(VulkanContext* context, VulkanSwapchain* swapchain, VkImageView depthImageView, VkFormat depthFormat);

    /**
     * @brief Felszabadítja az összes lefoglalt pipeline erőforrást (Pipeline-ok, Layout, RenderPass, Framebuffer-ek).
     */
    void cleanup();

    // --- Getter függvények a pipeline objektumok eléréséhez ---

    /** @return A Render Pass leírója. */
    VkRenderPass getRenderPass() const { return renderPass; }
    /** @return A Pipeline Layout leírója (Push Konstansokhoz/Descriptor Setekhez). */
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    /** @return A normál (kitöltött) grafikus pipeline leírója. */
    VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }
    /** @return A drótvázas (wireframe) grafikus pipeline leírója. */
    VkPipeline getWireframePipeline() const { return wireframePipeline; }
    /**
     * @brief Visszaadja a megadott indexű Framebuffer-t.
     * @param index A swapchain kép indexe.
     * @return A kért VkFramebuffer leíró.
     */
    VkFramebuffer getFramebuffer(uint32_t index) const { return swapChainFramebuffers[index]; }

private:
    /** @brief A Render Pass objektum (meghatározza a csatolmányokat és al-lépéseket). */
    VkRenderPass renderPass;
    /** @brief A Pipeline Layout (meghatározza a shaderek által használt erőforrásokat, pl. Push Konstansok). */
    VkPipelineLayout pipelineLayout;
    /** @brief A normál, kitöltött (solid) rendereléshez használt pipeline. */
    VkPipeline graphicsPipeline;
    /** @brief A drótvázas (wireframe) rendereléshez használt pipeline. */
    VkPipeline wireframePipeline;
    /** @brief A Framebuffer-ek vektora (egy minden swapchain képhez). */
    std::vector<VkFramebuffer> swapChainFramebuffers;

    /** @brief Mutató a központi Vulkan környezetre. */
    VulkanContext* context;

    // --- Privát inicializáló segédfüggvények ---

    /**
     * @brief Létrehozza a Render Pass-t a megadott szín- és mélységi formátumokkal.
     */
    void createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat);

    /**
     * @brief Létrehozza a normál (kitöltött) grafikus pipeline-t.
     */
    void createGraphicsPipeline();

    /**
     * @brief Létrehozza a drótvázas (wireframe) grafikus pipeline-t.
     */
    void createWireframePipeline();

    /**
     * @brief Létrehozza a Framebuffer-eket (egy-egy a swapchain képekhez).
     */
    void createFramebuffers(VulkanSwapchain* swapchain, VkImageView depthImageView);

    // --- Fájlkezelő segédfüggvények ---

    /**
     * @brief Beolvas egy bináris fájlt (pl. SPV shader) egy karakter vektorba.
     * @param filename A beolvasandó fájl neve.
     * @return A fájl tartalmát tartalmazó std::vector<char>.
     */
    static std::vector<char> readFile(const std::string& filename);

    /**
     * @brief Létrehoz egy VkShaderModule-t a beolvasott SPV bájtkódból.
     * @param code A SPV bájtkódot tartalmazó karakter vektor.
     * @return A létrehozott VkShaderModule leíró.
     */
    VkShaderModule createShaderModule(const std::vector<char>& code);
};