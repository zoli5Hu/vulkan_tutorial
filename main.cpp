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
#include <iterator> // std::size-hoz

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
#include "VulkanCore/MeshObject.h" // A saját objektum osztályunk

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
using namespace std;

// Segédfüggvény: Ideiglenes vertex tömbök a modellek inc fájlokból
namespace ModelData {
    // 1. Torus (Mozgó, Kitöltött) - Ez az egyetlen, ami a kódodban jelenleg létező fájlra hivatkozik
    static constexpr float torusVertices[] = {
        #include "./Modells/Datas/torus.inc"
    };

    // 2. Kocka (Statikus, Wireframe)
    // IDEIGLENESEN: Torus adatok használata (amíg nincs cube.inc)
    static constexpr float icoVertices[] = {
        #include "./Modells/Datas/ico.inc"
    };

    // 3. Piramis (Mozgó, Kitöltött)
    // IDEIGLENESEN: Torus adatok használata (amíg nincs pyramid.inc)
    static constexpr float pyramidVertices[] = {
        #include "./Modells/Datas/cone.inc"
    };
}


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

    // --- 3D Erőforrások (Depth) ---
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkFormat depthFormat;

    // TÖRÖLVE a régi: VkBuffer vertexBuffer és VkDeviceMemory vertexBufferMemory

    // --- ÚJ: Objektumok ---
    MeshObject torus;
    MeshObject cube;    // Wireframe alakzat (objektum lista 1. indexe)
    MeshObject pyramid; // Másik mozgó alakzat

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

    // A régi createVertexBuffer() helyett
    void createObjects() {
        // 1. Torus (Mozgó, Kitöltött) - Ez volt az eredeti működő
        // A vektor inicializálása iteratorral
        std::vector<float> torusVector(std::begin(ModelData::torusVertices), std::end(ModelData::torusVertices));
        torus.create(&vulkanContext, torusVector);
        torus.position = glm::vec3(-3.0f, 0.0f, 0.0f);
        torus.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        torus.rotationSpeed = 40.0f;

        // 2. Kocka (Statikus, Wireframe)
        // JAVÍTVA: Explicit pointer aritmetika (stabilabb C-stílusú tömbökkel)
        std::vector<float> cubeVector(std::begin(ModelData::icoVertices), std::end(ModelData::icoVertices));
        cube.create(&vulkanContext, cubeVector);
        cube.position = glm::vec3(3.0f, 0.0f, 0.0f);
        cube.rotationSpeed = 0.0f;

        // 3. Piramis (Mozgó, Kitöltött)
        // JAVÍTVA: Explicit pointer aritmetika
        const size_t pyramidSize = sizeof(ModelData::pyramidVertices) / sizeof(ModelData::pyramidVertices[0]);
        std::vector<float> pyramidVector(std::begin(ModelData::pyramidVertices), std::end(ModelData::pyramidVertices));
        pyramid.create(&vulkanContext, pyramidVector);
        pyramid.position = glm::vec3(0.0f, 3.0f, 0.0f);
        pyramid.rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        pyramid.rotationSpeed = 60.0f;
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

        createObjects();

        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain, depthImageView, depthFormat);
        vulkanRenderer.create(&vulkanContext, &vulkanSwapchain);
    }

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Multi Object", nullptr, nullptr);
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

            // Objektumok listája: a sorrend fontos! (1. index a wireframe)
            std::vector<MeshObject*> objects = {&torus, &cube, &pyramid};

            // JAVÍTVA: drawFrame hívás frissítve
            vulkanRenderer.drawFrame(&vulkanSwapchain, &vulkanPipeline, cameraPosition, objects);
        }

        vkDeviceWaitIdle(vulkanContext.getDevice());
    }

    void cleanup()
    {
        vulkanRenderer.cleanup();
        vulkanPipeline.cleanup();
        vulkanSwapchain.cleanup();

        // Objektumok felszabadítása
        torus.cleanup(vulkanContext.getDevice());
        cube.cleanup(vulkanContext.getDevice());
        pyramid.cleanup(vulkanContext.getDevice());

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