/**
 * @file main.cpp
 * @brief Teljes Vulkan PBR (Diffuse + ARM/Roughness) megvalósítás.
 * Javítva: 8 float/vertex támogatás, procedurális Torus generálás.
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

const uint32_t WIDTH = 1024;
const uint32_t HEIGHT = 768;
using namespace std;

/**
 * @brief Torus generálása matematikailag (hogy a geometria és a normálok tökéletesek legyenek).
 * @param mainRadius A fánk fő sugara.
 * @param tubeRadius A cső vastagsága.
 * @param mainSegments A fánk körének felbontása.
 * @param tubeSegments A cső körének felbontása.
 * @return 8 float/vertex vektor (Pos3, Norm3, UV2).
 */
std::vector<float> generateTorus(float mainRadius, float tubeRadius, int mainSegments, int tubeSegments) {
    std::vector<float> vertices;
    vertices.reserve(mainSegments * tubeSegments * 6 * 8);

    for (int i = 0; i < mainSegments; ++i) {
        for (int j = 0; j < tubeSegments; ++j) {
            // Egy négyszög 2 háromszögből áll. A csúcsok indexei a rácson:
            // 0: (i, j), 1: (i+1, j), 2: (i, j+1), 3: (i+1, j+1)
            // Háromszög 1: 0 -> 1 -> 2
            // Háromszög 2: 1 -> 3 -> 2

            int indices[6][2] = {
                {i, j}, {i + 1, j}, {i, j + 1},      // Tri 1
                {i + 1, j}, {i + 1, j + 1}, {i, j + 1} // Tri 2
            };

            for (int k = 0; k < 6; ++k) {
                int i_curr = indices[k][0];
                int j_curr = indices[k][1];

                // Szögek radiánban
                float u = (float)i_curr / mainSegments * 2.0f * glm::pi<float>();
                float v = (float)j_curr / tubeSegments * 2.0f * glm::pi<float>();

                // 1. Pozíció (Position)
                float cosU = cos(u), sinU = sin(u);
                float cosV = cos(v), sinV = sin(v);

                float x = (mainRadius + tubeRadius * cosV) * cosU;
                float y = tubeRadius * sinV;
                float z = (mainRadius + tubeRadius * cosV) * sinU;

                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);

                // 2. Normálvektor (Normal)
                float nx = cosV * cosU;
                float ny = sinV;
                float nz = cosV * sinU;

                vertices.push_back(nx);
                vertices.push_back(ny);
                vertices.push_back(nz);

                // 3. Textúra koordináta (UV)
                float texU = (float)i_curr / mainSegments * 4.0f; // 4x ismétlődés
                float texV = (float)j_curr / tubeSegments * 1.0f;

                vertices.push_back(texU);
                vertices.push_back(texV);
            }
        }
    }
    return vertices;
}

/**
 * @namespace ModelData
 * @brief A Kocka és a Piramis adatait olvassuk be az .inc fájlokból.
 */
namespace ModelData {
    // 2. Kocka (Ico helyett most a javított kocka adatokat várjuk)
    static constexpr float icoVertices[] = {
        #include "./Modells/Datas/ico.inc"
    };

    // 3. Piramis (Cone helyett piramis)
    static constexpr float pyramidVertices[] = {
        #include "./Modells/Datas/cone.inc"
    };

    // 4. Még egy Piramis (n_v)
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

    // --- Descriptor Pool és Textúrák ---
    VkDescriptorPool descriptorPool;
    Texture rockTexture;
    Texture rustTexture;

    // --- Kamera Állapot ---
    glm::vec3 cameraPosition = glm::vec3(0, 2, -8.0f);
    const float cameraSpeed = 2.5f;

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
        // --- 1. Textúrák betöltése (Diffuse + ARM) ---
        // Az "ARM" textúra tartalmazza a Roughness-t a zöld (G) csatornán.

        try {
            std::cout << "Texturak betoltese..." << std::endl;
            rockTexture.create(&vulkanContext,
                               "Assets/rock/aerial_rocks_02_diff_4k.jpg",  // Szín
                               "Assets/rock/aerial_rocks_02_arm_4k.jpg",   // ARM (Ambient, Roughness, Metallic)
                               descriptorPool, vulkanPipeline.getDescriptorSetLayout());

            rustTexture.create(&vulkanContext,
                               "Assets/rust/rusty_metal_grid_diff_4k.jpg",  // Szín
                               "Assets/rust/rusty_metal_grid_arm_4k.jpg",   // ARM
                               descriptorPool, vulkanPipeline.getDescriptorSetLayout());
        } catch (const std::exception& e) {
            std::cerr << "HIBA: Textura betoltes sikertelen! " << e.what() << std::endl;
        }

        // --- 2. Objektumok létrehozása ---

        // TORUS (Procedurálisan generálva)
        // Sugár: 1.0, Vastagság: 0.4, Részletesség: 48x24
        std::vector<float> torusVector = generateTorus(1.0f, 0.4f, 48, 24);
        torus.create(&vulkanContext, torusVector);
        torus.position = glm::vec3(-2.5f, 0.0f, 0.0f);
        torus.rotationAxis = glm::vec3(0.3f, 1.0f, 0.0f); // Kicsit döntve forogjon
        torus.rotationSpeed = 40.0f;
        torus.setTexture(rockTexture.descriptorSet);

        // KOCKA (ico.inc fájlból)
        std::vector<float> cubeVector(std::begin(ModelData::icoVertices), std::end(ModelData::icoVertices));
        cube.create(&vulkanContext, cubeVector);
        cube.position = glm::vec3(2.5f, 0.0f, 0.0f);
        cube.rotationSpeed = 25.0f;
        cube.rotationAxis = glm::vec3(1.0f, 0.5f, 0.0f);
        cube.setTexture(rustTexture.descriptorSet);

        // PIRAMIS (cone.inc fájlból)
        std::vector<float> pyramidVector(std::begin(ModelData::pyramidVertices), std::end(ModelData::pyramidVertices));
        pyramid.create(&vulkanContext, pyramidVector);
        pyramid.position = glm::vec3(0.0f, 2.5f, 0.0f);
        pyramid.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        pyramid.rotationSpeed = 60.0f;
        pyramid.setTexture(rockTexture.descriptorSet);

        // ALSÓ PIRAMIS (n)
        std::vector<float> n_ve(std::begin(ModelData::n_v), std::end(ModelData::n_v));
        n.create(&vulkanContext, n_ve);
        n.position = glm::vec3(0.0f, -2.5f, 0.0f);
        n.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Fejre állítjuk forgatással majd a draw-ban vagy itt
        n.rotationSpeed = -60.0f;
        n.setTexture(rustTexture.descriptorSet);
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

        // Pipeline létrehozása (már az új elrendezéssel: Pos, Norm, UV)
        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain, depthImageView, depthFormat);

        // Descriptor Pool
        vulkanContext.createDescriptorPool(descriptorPool);

        // Objektumok inicializálása
        createObjects();

        // Renderer
        vulkanRenderer.create(&vulkanContext, &vulkanSwapchain);
    }

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan PBR - Torus & Shapes", nullptr, nullptr);
    }

    void mainLoop()
    {
        auto lastTime = std::chrono::high_resolution_clock::now();

        while (!glfwWindowShouldClose(window)) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
            lastTime = currentTime;

            glfwPollEvents();

            // Kamera mozgatás
            float velocity = cameraSpeed * deltaTime;
            if (keysPressed[GLFW_KEY_W]) cameraPosition.z += velocity;
            if (keysPressed[GLFW_KEY_S]) cameraPosition.z -= velocity;
            if (keysPressed[GLFW_KEY_A]) cameraPosition.x -= velocity;
            if (keysPressed[GLFW_KEY_D]) cameraPosition.x += velocity;
            if (keysPressed[GLFW_KEY_R]) cameraPosition.y += velocity;
            if (keysPressed[GLFW_KEY_F]) cameraPosition.y -= velocity;

            // Renderelés
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

        rockTexture.cleanup();
        rustTexture.cleanup();

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