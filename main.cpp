#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

// 1. BEILLESZTJÜK AZ ÖSSZES OSZTÁLYUNKAT
#include "VulkanCore/VulkanContext.h"
#include "VulkanCore/VulkanSwapchain.h"
#include "VulkanCore/VulkanPipeline.h"
#include "VulkanCore/VulkanRenderer.h" // <-- ÚJ

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
using namespace std;

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
    // A motorunk most már 4 fő részből áll:
    VulkanContext vulkanContext;
    VulkanSwapchain vulkanSwapchain;
    VulkanPipeline vulkanPipeline;
    VulkanRenderer vulkanRenderer; // <-- ÚJ OBJEKTUM

    // --- Alkalmazás-specifikus objektumok (MARADNAK) ---
    // A Surface az egyetlen Vulkan objektum, ami az alkalmazáshoz
    // (az ablakhoz) kötődik, nem a motor belső működéséhez.
    VkSurfaceKHR surface;

    // TÖRÖLVE: commandBuffers, Szinkronizációs objektumok (semaphores, fences)
    // Ezek mind átkerültek a VulkanRenderer-be.

    void initVulkan()
    {
        // 1. A "MAG" INICIALIZÁLÁSA
        vulkanContext.initVulkan(window);

        // 2. LÉTREHOZZUK A SURFACE-T
        if (glfwCreateWindowSurface(vulkanContext.getInstance(), window, nullptr, &surface) != VK_SUCCESS)
        {
            throw runtime_error("failed to create window surface!");
        }

        // 3. LÉTREHOZZUK A SWAPCHAIN-T
        vulkanSwapchain.create(&vulkanContext, surface, window);

        // 4. LÉTREHOZZUK A PIPELINE-T
        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain);

        // 5. LÉTREHOZZUK A RENDERELŐT
        // Ez hozza létre a CommandBuffer-eket és a Szinkronizációt
        vulkanRenderer.create(&vulkanContext, &vulkanSwapchain);
    }

    // TÖRÖLVE: createCommandBuffer, recordCommandBuffer, createSyncObjects, drawFrame
    // Ezek mind átkerültek a VulkanRenderer-be.

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void mainLoop()
    {
        std::cout << "Indul a Main Loop (Javított Kód)" << std::endl;
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // JAVÍTVA: A drawFrame hívás átkerült a renderer-be
            vulkanRenderer.drawFrame(&vulkanSwapchain, &vulkanPipeline);
        }

        // Várjuk meg, amíg a GPU befejezi az utolsó munkát is
        vkDeviceWaitIdle(vulkanContext.getDevice());
    }

    void cleanup()
    {
        // 1. ÚJ: A VulkanRenderer takarítása
        // (ez törli: Semaphores, Fences)
        // (A CommandBuffer-eket a CommandPool törlése kezeli)
        vulkanRenderer.cleanup();

        // 2. A VulkanPipeline takarítása
        // (ez törli: Framebuffer, Pipeline, Layout, RenderPass)
        vulkanPipeline.cleanup();

        // 3. A VulkanSwapchain takarítása
        // (ez törli: Swapchain, ImageViews)
        vulkanSwapchain.cleanup();

        // 4. FONTOS: A Surface törlése
        vkDestroySurfaceKHR(vulkanContext.getInstance(), surface, nullptr);

        // 5. A VÉGÉN hívjuk meg a Context saját takarítóját
        // (ez törli: Instance, Device, CommandPool, Debugger)
        vulkanContext.cleanup();

        // 6. Végül a GLFW takarítása
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