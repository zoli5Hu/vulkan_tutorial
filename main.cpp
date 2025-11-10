#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector> // ÚJ: a vertex adatokhoz
#include <cstring> // ÚJ: a memcpy-hez

// 1. BEILLESZTJÜK AZ ÖSSZES OSZTÁLYUNKAT
#include "VulkanCore/VulkanContext.h"
#include "VulkanCore/VulkanSwapchain.h"
#include "VulkanCore/VulkanPipeline.h"
#include "VulkanCore/VulkanRenderer.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
using namespace std;

// A kocka adatai a fájlból (36 csúcspont)
//
static constexpr float g_cubeVertices[] = {
    #include "./Modells/Datas/torus.inc"

};


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

    // --- Core Komponensek ---
    VulkanContext vulkanContext;
    VulkanSwapchain vulkanSwapchain;
    VulkanPipeline vulkanPipeline;
    VulkanRenderer vulkanRenderer;

    // --- Alkalmazás-specifikus objektumok (MARADNAK) ---
    VkSurfaceKHR surface;

    // --- ÚJ: 3D Erőforrások ---
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkFormat depthFormat;

    // --- ÚJ segédfüggvények ---

    // Létrehozza a Vertex Buffert (a kocka adataival)
    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(g_cubeVertices);

        // 1. Staging Buffer (CPU oldali) létrehozása
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vulkanContext.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // Forrás
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        // 2. Adatok másolása a Staging Bufferbe
        void* data;
        vkMapMemory(vulkanContext.getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, g_cubeVertices, (size_t)bufferSize);
        vkUnmapMemory(vulkanContext.getDevice(), stagingBufferMemory);

        // 3. Vertex Buffer (GPU oldali) létrehozása
        vulkanContext.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // Cél és Vertex Buffer
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // Csak a GPU látja
            vertexBuffer,
            vertexBufferMemory
        );

        // 4. Adatok átmásolása (Staging -> Vertex Buffer) a GPU-n
        vulkanContext.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        // 5. Staging Buffer törlése
        vkDestroyBuffer(vulkanContext.getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(vulkanContext.getDevice(), stagingBufferMemory, nullptr);
    }

    // Létrehozza a Depth Buffert (mélységi kép)
    void createDepthResources() {
        depthFormat = VK_FORMAT_D32_SFLOAT; // Standard mélység formátum
        VkExtent2D swapChainExtent = vulkanSwapchain.getExtent();

        // 1. Kép létrehozása
        vulkanContext.createImage(
            swapChainExtent.width,
            swapChainExtent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // Mélységi pufferként használjuk
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImage,
            depthImageMemory
        );

        // 2. Képnézet (ImageView) létrehozása
        depthImageView = vulkanContext.createImageView(
            depthImage,
            depthFormat,
            VK_IMAGE_ASPECT_DEPTH_BIT // Mélységi képként kezeljük
        );

        // (Megjegyzés: Itt még kellene egy layout transition,
        // de a RenderPass-unk kezeli az 'UNDEFINED' layout-ot)
    }

    void initVulkan()
    {
        // 1. Instance
        vulkanContext.initInstance(window);

        // 2. Surface
        if (glfwCreateWindowSurface(vulkanContext.getInstance(), window, nullptr, &surface) != VK_SUCCESS)
        {
            throw runtime_error("failed to create window surface!");
        }

        // 3. Device
        vulkanContext.initDevice(surface);

        // 4. Swapchain
        vulkanSwapchain.create(&vulkanContext, surface, window);

        // --- MÓDOSÍTOTT SORREND ---

        // 5. ÚJ: Depth Buffer létrehozása (a pipeline-nak kell)
        createDepthResources();

        // 6. ÚJ: Vertex Buffer létrehozása (a renderelőnek kell)
        createVertexBuffer();

        // 7. MÓDOSÍTOTT: Pipeline létrehozása
        // Átadjuk az új mélységi adatokat, amiket a pipeline elvár
        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain, depthImageView, depthFormat);

        // 8. Renderer létrehozása
        vulkanRenderer.create(&vulkanContext, &vulkanSwapchain);
    }


    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Kocka", nullptr, nullptr);
    }

    void mainLoop()
    {
        std::cout << "kacsa a Main Loop (Javított Kód)" << std::endl;
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // MÓDOSÍTVA: Átadjuk a vertex buffert a renderelőnek
            // EZ HIBA LESZ, amíg a VulkanRenderer-t nem frissítjük!
            vulkanRenderer.drawFrame(&vulkanSwapchain, &vulkanPipeline, vertexBuffer);
        }

        // Várjuk meg, amíg a GPU befejezi az utolsó munkát is
        vkDeviceWaitIdle(vulkanContext.getDevice());
    }

    void cleanup()
    {
        // A létrehozással ellentétes sorrendben takarítunk

        vulkanRenderer.cleanup();
        vulkanPipeline.cleanup();
        vulkanSwapchain.cleanup();

        // --- ÚJ: Erőforrások törlése ---
        vkDestroyImageView(vulkanContext.getDevice(), depthImageView, nullptr);
        vkDestroyImage(vulkanContext.getDevice(), depthImage, nullptr);
        vkFreeMemory(vulkanContext.getDevice(), depthImageMemory, nullptr);

        vkDestroyBuffer(vulkanContext.getDevice(), vertexBuffer, nullptr);
        vkFreeMemory(vulkanContext.getDevice(), vertexBufferMemory, nullptr);
        // --- ---

        vkDestroySurfaceKHR(vulkanContext.getInstance(), surface, nullptr);
        vulkanContext.cleanup();

        glfwDestroyWindow(window);
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