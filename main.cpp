/**
 * @file main.cpp
 * @brief Vulkan PBR - Torus, Kocka, Piramis és Padló.
 * Készítette: Multimaster AI
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

// --- GEOMETRIA GENERÁLÓK ---

// Torus generálása (8 float / vertex)
std::vector<float> generateTorus(float mainRadius, float tubeRadius, int mainSegments, int tubeSegments) {
    std::vector<float> vertices;
    vertices.reserve(mainSegments * tubeSegments * 6 * 8);

    for (int i = 0; i < mainSegments; ++i) {
        for (int j = 0; j < tubeSegments; ++j) {
            int indices[6][2] = {
                {i, j}, {i + 1, j}, {i, j + 1},
                {i + 1, j}, {i + 1, j + 1}, {i, j + 1}
            };

            for (int k = 0; k < 6; ++k) {
                int i_curr = indices[k][0];
                int j_curr = indices[k][1];

                float u = (float)i_curr / mainSegments * 2.0f * glm::pi<float>();
                float v = (float)j_curr / tubeSegments * 2.0f * glm::pi<float>();

                float cosU = cos(u), sinU = sin(u);
                float cosV = cos(v), sinV = sin(v);

                // Pozíció
                float x = (mainRadius + tubeRadius * cosV) * cosU;
                float y = tubeRadius * sinV;
                float z = (mainRadius + tubeRadius * cosV) * sinU;
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);

                // Normál
                float nx = cosV * cosU;
                float ny = sinV;
                float nz = cosV * sinU;
                vertices.push_back(nx); vertices.push_back(ny); vertices.push_back(nz);

                // UV
                float texU = (float)i_curr / mainSegments * 4.0f;
                float texV = (float)j_curr / tubeSegments * 1.0f;
                vertices.push_back(texU); vertices.push_back(texV);
            }
        }
    }
    return vertices;
}

// Padló (Sík) generálása
std::vector<float> generateFloor(float size, float uvScale) {
    std::vector<float> vertices;
    float half = size / 2.0f;
    // Format: Pos(3), Norm(3), UV(2)
    // Normálvektor felfelé mutat: (0, 1, 0)

    // Háromszög 1
    vertices.insert(vertices.end(), {-half, 0.0f, -half,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f});
    vertices.insert(vertices.end(), {-half, 0.0f,  half,  0.0f, 1.0f, 0.0f,  0.0f, uvScale});
    vertices.insert(vertices.end(), { half, 0.0f, -half,  0.0f, 1.0f, 0.0f,  uvScale, 0.0f});

    // Háromszög 2
    vertices.insert(vertices.end(), { half, 0.0f, -half,  0.0f, 1.0f, 0.0f,  uvScale, 0.0f});
    vertices.insert(vertices.end(), {-half, 0.0f,  half,  0.0f, 1.0f, 0.0f,  0.0f, uvScale});
    vertices.insert(vertices.end(), { half, 0.0f,  half,  0.0f, 1.0f, 0.0f,  uvScale, uvScale});

    return vertices;
}

namespace ModelData {
    static constexpr float icoVertices[] = {
        #include "./Modells/Datas/ico.inc"
    };
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
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, staticKeyCallback);
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VulkanContext vulkanContext;
    VulkanSwapchain vulkanSwapchain;
    VulkanPipeline vulkanPipeline;
    VulkanRenderer vulkanRenderer;

    VkDescriptorPool descriptorPool;
    Texture rockTexture;
    Texture rustTexture;

    glm::vec3 cameraPosition = glm::vec3(0, 3, -10.0f); // Kicsit magasabbról nézzük
    const float cameraSpeed = 2.5f;
    std::map<int, bool> keysPressed;

    VkSurfaceKHR surface;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkFormat depthFormat;

    // --- OBJEKTUMOK ---
    MeshObject torus;
    MeshObject cube;
    MeshObject pyramid;
    MeshObject n;
    MeshObject floor; // A padló

    static void staticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(userPtr);
            if (action == GLFW_PRESS) app->keysPressed[key] = true;
            else if (action == GLFW_RELEASE) app->keysPressed[key] = false;
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    void createObjects() {
        try {
            rockTexture.create(&vulkanContext, "Assets/rock/aerial_rocks_02_diff_4k.jpg", "Assets/rock/aerial_rocks_02_arm_4k.jpg", descriptorPool, vulkanPipeline.getDescriptorSetLayout());
            rustTexture.create(&vulkanContext, "Assets/rust/rusty_metal_grid_diff_4k.jpg", "Assets/rust/rusty_metal_grid_arm_4k.jpg", descriptorPool, vulkanPipeline.getDescriptorSetLayout());
        } catch (const std::exception& e) {
            std::cerr << "Textura hiba: " << e.what() << std::endl;
        }

        // 1. Torus
        std::vector<float> torusVector = generateTorus(1.0f, 0.4f, 48, 24);
        torus.create(&vulkanContext, torusVector);
        torus.position = glm::vec3(-3.0f, 0.5f, 0.0f);
        torus.rotationAxis = glm::vec3(0.3f, 1.0f, 0.0f);
        torus.rotationSpeed = 40.0f;
        torus.setTexture(rockTexture.descriptorSet);

        // 2. Kocka
        std::vector<float> cubeVector(std::begin(ModelData::icoVertices), std::end(ModelData::icoVertices));
        cube.create(&vulkanContext, cubeVector);
        cube.position = glm::vec3(3.0f, 0.5f, 0.0f);
        cube.rotationSpeed = 25.0f;
        cube.rotationAxis = glm::vec3(1.0f, 0.5f, 0.0f);
        cube.setTexture(rustTexture.descriptorSet);

        // 3. Piramis (Felső)
        std::vector<float> pyramidVector(std::begin(ModelData::pyramidVertices), std::end(ModelData::pyramidVertices));
        pyramid.create(&vulkanContext, pyramidVector);
        pyramid.position = glm::vec3(0.0f, 3.0f, 0.0f);
        pyramid.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        pyramid.rotationSpeed = 60.0f;
        pyramid.setTexture(rockTexture.descriptorSet);

        // 4. Piramis (Alsó)
        n.create(&vulkanContext, pyramidVector); // Ugyanaz a geometria
        n.position = glm::vec3(0.0f, -2.0f, 0.0f);
        n.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        n.rotationSpeed = -60.0f;
        n.setTexture(rustTexture.descriptorSet);

        // 5. PADLÓ (ÚJ)
        std::vector<float> floorVec = generateFloor(20.0f, 4.0f); // 20x20-as méret, 4x tiling
        floor.create(&vulkanContext, floorVec);
        floor.position = glm::vec3(0.0f, -3.0f, 0.0f); // A többi objektum alá tesszük
        floor.rotationSpeed = 0.0f;
        floor.setTexture(rustTexture.descriptorSet); // Legyen fémrácsos padló
    }

    void createDepthResources() {
        depthFormat = VK_FORMAT_D32_SFLOAT;
        VkExtent2D swapChainExtent = vulkanSwapchain.getExtent();
        vulkanContext.createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = vulkanContext.createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        vulkanContext.executeSingleTimeCommands([&](VkCommandBuffer commandBuffer) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = depthImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            barrier.subresourceRange.baseMipLevel = 0; barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0; barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        });
    }

    void initVulkan() {
        vulkanContext.initInstance(window);
        if (glfwCreateWindowSurface(vulkanContext.getInstance(), window, nullptr, &surface) != VK_SUCCESS) throw runtime_error("failed to create window surface!");
        vulkanContext.initDevice(surface);
        vulkanSwapchain.create(&vulkanContext, surface, window);
        createDepthResources();
        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain, depthImageView, depthFormat);
        vulkanContext.createDescriptorPool(descriptorPool);
        createObjects();
        vulkanRenderer.create(&vulkanContext, &vulkanSwapchain);
    }

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan - Floor & Lights", nullptr, nullptr);
    }

    void mainLoop() {
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

            // Renderelés (a padlót is hozzáadtuk a listához)
            std::vector<MeshObject*> objects = {&torus, &cube, &pyramid, &n, &floor};
            vulkanRenderer.drawFrame(&vulkanSwapchain, &vulkanPipeline, cameraPosition, objects);
        }
        vkDeviceWaitIdle(vulkanContext.getDevice());
    }

    void cleanup() {
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
        floor.cleanup(vulkanContext.getDevice()); // Padló takarítása

        vkDestroyImageView(vulkanContext.getDevice(), depthImageView, nullptr);
        vkDestroyImage(vulkanContext.getDevice(), depthImage, nullptr);
        vkFreeMemory(vulkanContext.getDevice(), depthImageMemory, nullptr);
        vkDestroySurfaceKHR(vulkanContext.getInstance(), surface, nullptr);
        vulkanContext.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;
    try { app.run(); }
    catch (const exception& e) { cerr << e.what() << endl; return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}