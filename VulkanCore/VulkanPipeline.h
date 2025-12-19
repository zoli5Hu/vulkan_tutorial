#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include <string>
#include <vector>

/**
 * @brief A VulkanPipeline osztály kezeli a grafikai csővezetéket,
 * a renderelési meneteket (RenderPass) és a Framebuffereket.
 */
class VulkanPipeline {
public:
    VulkanPipeline();
    ~VulkanPipeline();

    /**
     * @brief Inicializálja a teljes grafikai pipeline-t.
     * @param ctx Vulkan kontextus az eszköz eléréséhez.
     * @param swapchain A swapchain a felbontás és formátum lekérdezéséhez.
     * @param depthImageView A mélységi puffer nézete a framebuffer-ekhez.
     * @param depthFormat A használt mélységi képformátum.
     */
    void create(VulkanContext* ctx, VulkanSwapchain* swapchain, VkImageView depthImageView, VkFormat depthFormat);

    /**
     * @brief Felszabadítja a pipeline-hoz kapcsolódó összes Vulkan erőforrást.
     */
    void cleanup();

    // --- Getter függvények a renderelés vezérléséhez ---
    VkPipeline getGraphicsPipeline() { return graphicsPipeline; } // A tényleges csővezeték objektum
    VkPipelineLayout getPipelineLayout() { return pipelineLayout; } // Uniform/Push constant elrendezés
    VkRenderPass getRenderPass() { return renderPass; }           // A renderelési szakasz leírása
    VkDescriptorSetLayout getDescriptorSetLayout() { return descriptorSetLayout; } // Textúra binding struktúra

    /**
     * @brief Visszaadja a swapchain képekhez létrehozott framebuffer-ek listáját.
     * @return Konstans referencia a framebuffer vektorra.
     */
    const std::vector<VkFramebuffer>& getFramebuffers() const { return swapChainFramebuffers; }

private:
    VulkanContext* context; // Referencia a Vulkan környezetre

    // Alapvető Vulkan handle-ök
    VkRenderPass renderPass;                  // Meghatározza a szín/mélység csatolókat
    VkDescriptorSetLayout descriptorSetLayout; // Anyag textúrák elrendezése (Set 0)
    VkDescriptorSetLayout shadowSetLayout;     // Árnyéktérkép elrendezése (Set 1)
    VkPipelineLayout pipelineLayout;           // Összefogja a descriptorokat és push constantokat
    VkPipeline graphicsPipeline;               // A végleges grafikai állapotgép
    VkPipeline wireframePipeline;              // Opcionális drótvázas megjelenítés

    // Framebufferek: a render pass és a swapchain képek összekapcsolása
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Belső inicializáló lépések
    void createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat);
    void createGraphicsPipeline();
    void createWireframePipeline();
    void createFramebuffers(VulkanSwapchain* swapchain, VkImageView depthImageView);

    /**
     * @brief Segédfüggvény a bináris SPIR-V kód shader modullá alakításához.
     */
    VkShaderModule createShaderModule(const std::vector<char>& code);

    /**
     * @brief Statikus segédfüggvény a shader fájlok beolvasásához.
     */
    static std::vector<char> readFile(const std::string& filename);
};