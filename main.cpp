/**
 * @file main.cpp
 * @brief Fő alkalmazásfájl a Vulkan grafikus programhoz.
 */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <chrono>
#include <cmath>
#include <iterator>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

// Include-ok
#include "VulkanCore/VulkanContext.h"
#include "VulkanCore/VulkanSwapchain.h"
#include "VulkanCore/VulkanPipeline.h"
#include "VulkanCore/VulkanRenderer.h"
#include "VulkanCore/MeshObject.h"
#include "VulkanCore/Texture.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
using namespace std;

/**
 * @namespace ModelData
 * @brief Modellek adatai (vertex tömbök).
 */
namespace ModelData {
    // 1. Torus
    static constexpr float torusVertices[] = {
        #include "./Modells/Datas/torus.inc"
    };

    // 2. Kocka (Ico)
    static constexpr float icoVertices[] = {
        #include "./Modells/Datas/ico.inc"
    };

    // 3. Piramis (Cone)
    static constexpr float pyramidVertices[] = {
        #include "./Modells/Datas/cone.inc"
    };

    // 4. Piramis (n_v)
    static constexpr float n_v[] = {
        #include "./Modells/Datas/cone.inc"
    };
}

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
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

    // --- ÚJ: Descriptor Pool és Textúrák ---
    VkDescriptorPool descriptorPool;
    Texture rockTexture;
    Texture rustTexture;

    // --- Kamera Állapot ---
    glm::vec3 cameraPosition = glm::vec3(0, 2, -10.0f);
    const float cameraSpeed = 1.5f;

    // --- Input ---
    std::map<int, bool> keysPressed;

    VkSurfaceKHR surface;

    // --- Depth Resources ---
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkFormat depthFormat;

    // --- Objektumok ---
    MeshObject torus;
    MeshObject cube;
    MeshObject pyramid;
    MeshObject n;

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

    void createObjects() {
        // --- 1. Textúrák betöltése ---
        // A képed alapján ezeket a fájlokat használjuk.
        // FONTOS: Győződj meg róla, hogy az "Assets" mappa ott van az .exe mellett!

        // Szikla textúra betöltése
        try {
            rockTexture.create(&vulkanContext, "Assets/rock/aerial_rocks_02_diff_4k.jpg",
                               descriptorPool, vulkanPipeline.getDescriptorSetLayout());
        } catch (const std::exception& e) {
            std::cerr << "HIBA: Szikla textúra betöltése sikertelen! " << e.what() << std::endl;
        }

        // Rozsda textúra betöltése
        try {
            rustTexture.create(&vulkanContext, "Assets/rust/rusty_metal_grid_diff_4k.jpg",
                               descriptorPool, vulkanPipeline.getDescriptorSetLayout());
        } catch (const std::exception& e) {
            std::cerr << "HIBA: Rozsda textúra betöltése sikertelen! " << e.what() << std::endl;
        }

        // --- 2. Objektumok létrehozása és textúra hozzárendelés ---

        // Torus - Szikla textúra
        std::vector<float> torusVector(std::begin(ModelData::torusVertices), std::end(ModelData::torusVertices));
        torus.create(&vulkanContext, torusVector);
        torus.position = glm::vec3(-3.0f, 0.0f, 0.0f);
        torus.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        torus.rotationSpeed = 40.0f;
        torus.setTexture(rockTexture.descriptorSet); // Textúra beállítása

        // Kocka (Ico) - Rozsda textúra
        std::vector<float> cubeVector(std::begin(ModelData::icoVertices), std::end(ModelData::icoVertices));
        cube.create(&vulkanContext, cubeVector);
        cube.position = glm::vec3(3.0f, 0.0f, 0.0f);
        cube.rotationSpeed = 15.0f; // Most már forogjon ez is kicsit
        cube.rotationAxis = glm::vec3(1.0f, 1.0f, 0.0f);
        cube.setTexture(rustTexture.descriptorSet); // Textúra beállítása

        // Piramis - Szikla textúra
        std::vector<float> pyramidVector(std::begin(ModelData::pyramidVertices), std::end(ModelData::pyramidVertices));
        pyramid.create(&vulkanContext, pyramidVector);
        pyramid.position = glm::vec3(0.0f, 3.0f, 0.0f);
        pyramid.rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        pyramid.rotationSpeed = 60.0f;
        pyramid.setTexture(rockTexture.descriptorSet); // Textúra beállítása

        // Piramis (n) - Rozsda textúra
        std::vector<float> n_ve(std::begin(ModelData::n_v), std::end(ModelData::n_v));
        n.create(&vulkanContext, n_ve);
        n.position = glm::vec3(0.0f, -3.0f, 0.0f);
        n.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        n.rotationSpeed = 90.0f;
        n.setTexture(rustTexture.descriptorSet); // Textúra beállítása
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

        // 1. Pipeline Létrehozása (Layout miatt kell előbb!)
        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain, depthImageView, depthFormat);

        // 2. Descriptor Pool Létrehozása (Textúrákhoz)
        vulkanContext.createDescriptorPool(descriptorPool);

        // 3. Objektumok Létrehozása (Textúrák betöltése)
        // Most már biztonságos, mert van Pipeline Layout és Descriptor Pool
        createObjects();

        // 4. Renderer Létrehozása
        vulkanRenderer.create(&vulkanContext, &vulkanSwapchain);
    }

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Textured Objects", nullptr, nullptr);
    }

    void mainLoop()
    {
        auto lastTime = std::chrono::high_resolution_clock::now();

        while (!glfwWindowShouldClose(window)) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
            lastTime = currentTime;

            glfwPollEvents();

            float velocity = cameraSpeed * deltaTime;

            if (keysPressed[GLFW_KEY_W]) cameraPosition.z += velocity;
            if (keysPressed[GLFW_KEY_S]) cameraPosition.z -= velocity;
            if (keysPressed[GLFW_KEY_A]) cameraPosition.x -= velocity;
            if (keysPressed[GLFW_KEY_D]) cameraPosition.x += velocity;
            if (keysPressed[GLFW_KEY_R]) cameraPosition.y += velocity;
            if (keysPressed[GLFW_KEY_F]) cameraPosition.y -= velocity;

            // Most már minden objektum textúrázott, nincs wireframe pipeline váltogatás
            std::vector<MeshObject*> objects = {&torus, &cube, &pyramid, &n};

            vulkanRenderer.drawFrame(&vulkanSwapchain, &vulkanPipeline, cameraPosition, objects);
        }

        vkDeviceWaitIdle(vulkanContext.getDevice());
    }

    void cleanup()
    {
        vulkanRenderer.cleanup();
        vulkanPipeline.cleanup();
        vulkanSwapchain.cleanup();

        // Textúrák felszabadítása
        rockTexture.cleanup();
        rustTexture.cleanup();

        // Descriptor Pool felszabadítása
        vkDestroyDescriptorPool(vulkanContext.getDevice(), descriptorPool, nullptr);

        torus.cleanup(vulkanContext.getDevice());
        cube.cleanup(vulkanContext.getDevice());
        pyramid.cleanup(vulkanContext.getDevice());
        n.cleanup(vulkanContext.getDevice());

        vkDestroyImageView(vulkanContext.getDevice(), depthImageView, nullptr);
        vkDestroyImage(vulkanContext.getDevice(), depthImage, nullptr);
        vkFreeMemory(vulkanContext.getDevice(), depthImageMemory, nullptr);

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