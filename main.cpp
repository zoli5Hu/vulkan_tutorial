#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <chrono> // <- IDŐMÉRÉSHEZ
#include <cmath> // A math.h függvényekhez

// GLM include-ok
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

// 1. BEILLESZTJÜK AZ ÖSSZES OSZTÁLYUNKAT
#include "VulkanCore/VulkanContext.h"
#include "VulkanCore/VulkanSwapchain.h"
#include "VulkanCore/VulkanPipeline.h"
#include "VulkanCore/VulkanRenderer.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
using namespace std;

// A torus adatai
static constexpr float g_cubeVertices[] = {
    #include "./Modells/Datas/torus.inc"
};


class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        // 2. Callback regisztrálása a window objektummal (ez a 'this')
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, staticKeyCallback);

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

    // --- Kamera Állapot ---
    // A kamera pozíciója a (0,0,0) pontra néz
    glm::vec3 cameraPosition = glm::vec3(2.0f, 2.0f, -4.0f);
    // Visszaállítjuk a sebességet, hogy a deltaTime-mal sima mozgást adjon
    const float cameraSpeed = 1.5f; // Pl. 1.5 méter/másodperc

    // --- Input Állapot ---
    std::map<int, bool> keysPressed;

    // --- Alkalmazás-specifikus objektumok ---
    VkSurfaceKHR surface;

    // --- 3D Erőforrások ---
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkFormat depthFormat;

    // --- Static Callback a GLFW-hez (a C++ osztályon BELÜL) ---
    static void staticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(userPtr);

            if (action == GLFW_PRESS) {
                app->keysPressed[key] = true;
            } else if (action == GLFW_RELEASE) {
                app->keysPressed[key] = false;
            }

            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }
    }


    // --- Core segédfüggvények ---

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(g_cubeVertices);

        // A többi Vulkan kód a VulkanContext.cpp-ben van.
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vulkanContext.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data;
        vkMapMemory(vulkanContext.getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, g_cubeVertices, (size_t)bufferSize);
        vkUnmapMemory(vulkanContext.getDevice(), stagingBufferMemory);

        vulkanContext.createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBuffer,
            vertexBufferMemory
        );
        vulkanContext.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
        vkDestroyBuffer(vulkanContext.getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(vulkanContext.getDevice(), stagingBufferMemory, nullptr);
    }

    void createDepthResources() {
        depthFormat = VK_FORMAT_D32_SFLOAT;
        VkExtent2D swapChainExtent = vulkanSwapchain.getExtent();

        vulkanContext.createImage(
            swapChainExtent.width,
            swapChainExtent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImage,
            depthImageMemory
        );
        depthImageView = vulkanContext.createImageView(
            depthImage,
            depthFormat,
            VK_IMAGE_ASPECT_DEPTH_BIT
        );

        vulkanContext.executeSingleTimeCommands([&](VkCommandBuffer commandBuffer) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = depthImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        });
    }


    void initVulkan()
    {
        vulkanContext.initInstance(window);
        if (glfwCreateWindowSurface(vulkanContext.getInstance(), window, nullptr, &surface) != VK_SUCCESS)
        {
            throw runtime_error("failed to create window surface!");
        }
        vulkanContext.initDevice(surface);
        vulkanSwapchain.create(&vulkanContext, surface, window);
        createDepthResources();
        createVertexBuffer();

        // JAVÍTVA: A VulkanPipeline create hívása helyes
        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain, depthImageView, depthFormat);
        // JAVÍTVA: A VulkanRenderer create hívása helyes
        vulkanRenderer.create(&vulkanContext, &vulkanSwapchain);
    }

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Torus", nullptr, nullptr);
    }

    void mainLoop()
    {
        auto lastTime = std::chrono::high_resolution_clock::now();

        std::cout << "kacsa a Main Loop (Javított Kód)" << std::endl;
        while (!glfwWindowShouldClose(window)) {

            // Valós deltaTime kiszámítása
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
            lastTime = currentTime;

            glfwPollEvents();

            // --- INPUT KEZELÉS (FPS FÜGGETLEN) ---
            float velocity = cameraSpeed * deltaTime;

            // FWD/BACK (W/S) -> Z tengely
            if (keysPressed[GLFW_KEY_W])
                cameraPosition.z += velocity;
            if (keysPressed[GLFW_KEY_S])
                cameraPosition.z -= velocity;

            // STRAFE (A/D) -> X tengely
            if (keysPressed[GLFW_KEY_A])
                cameraPosition.x -= velocity;
            if (keysPressed[GLFW_KEY_D])
                cameraPosition.x += velocity;

            // FEL/LE (R/F) -> Y tengely (EZ AZ ÚJ BLOKK)
            if (keysPressed[GLFW_KEY_R])
                cameraPosition.y += velocity;
            if (keysPressed[GLFW_KEY_F])
                cameraPosition.y -= velocity;


            vulkanRenderer.drawFrame(&vulkanSwapchain, &vulkanPipeline, vertexBuffer, cameraPosition);
        }

        vkDeviceWaitIdle(vulkanContext.getDevice());
    }

    void cleanup()
    {
        vulkanRenderer.cleanup();
        vulkanPipeline.cleanup();
        vulkanSwapchain.cleanup();

        vkDestroyImageView(vulkanContext.getDevice(), depthImageView, nullptr);
        vkDestroyImage(vulkanContext.getDevice(), depthImage, nullptr);
        vkFreeMemory(vulkanContext.getDevice(), depthImageMemory, nullptr);

        vkDestroyBuffer(vulkanContext.getDevice(), vertexBuffer, nullptr);
        vkFreeMemory(vulkanContext.getDevice(), vertexBufferMemory, nullptr);

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