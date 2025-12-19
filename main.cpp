/**
 * @file main.cpp
 * @brief Vulkan PBR - Normal Mapping (Javított kamera + Assets útvonalak).
 * Ez a fájl a belépési pont, itt történik az ablakkezelés (GLFW), a bemenet kezelése,
 * és a fő renderelési ciklus futtatása.
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

// Saját Vulkan keretrendszer modulok
#include "VulkanCore/VulkanContext.h"
#include "VulkanCore/VulkanSwapchain.h"
#include "VulkanCore/VulkanPipeline.h"
#include "VulkanCore/VulkanRenderer.h"
#include "VulkanCore/MeshObject.h"
#include "VulkanCore/Texture.h"

const uint32_t WIDTH = 1024;
const uint32_t HEIGHT = 768;
using namespace std;

// --- GEOMETRIA GENERÁLÓ FÜGGVÉNYEK ---
// Ezek a függvények nyers vertex adatokat állítanak elő:
// Stride: 8 float (X, Y, Z, NormX, NormY, NormZ, U, V)
// A MeshObject osztály fogja ezt később kiegészíteni a Tangens adatokkal (11 floatra).

// Tórusz (fánk) generálása
std::vector<float> generateTorus(float mainRadius, float tubeRadius, int mainSegments, int tubeSegments) {
    std::vector<float> vertices;
    for (int i = 0; i <= mainSegments; ++i) {
        float u = (float)i / mainSegments * glm::two_pi<float>();
        float cosU = cos(u);
        float sinU = sin(u);
        for (int j = 0; j <= tubeSegments; ++j) {
            float v = (float)j / tubeSegments * glm::two_pi<float>();
            float cosV = cos(v);
            float sinV = sin(v);

            // Pozíció számítása
            float x = (mainRadius + tubeRadius * cosV) * cosU;
            float y = (mainRadius + tubeRadius * cosV) * sinU;
            float z = tubeRadius * sinV;

            // Normálvektor számítása
            float nx = cosV * cosU;
            float ny = cosV * sinU;
            float nz = sinV;

            // Adatok hozzáadása (Pos + Norm + UV)
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
            vertices.push_back(nx); vertices.push_back(ny); vertices.push_back(nz);
            vertices.push_back((float)i / mainSegments * 4.0f); // UV ismétlődés
            vertices.push_back((float)j / tubeSegments * 1.0f);
        }
    }
    // Indexelés helyett duplikált vertexekkel ("Triangle List") töltjük fel a kimenetet
    std::vector<float> finalVertices;
    for (int i = 0; i < mainSegments; ++i) {
        for (int j = 0; j < tubeSegments; ++j) {
            int p1 = i * (tubeSegments + 1) + j;
            int p2 = ((i + 1)) * (tubeSegments + 1) + j;
            int p3 = p1 + 1;
            int p4 = p2 + 1;

            auto pushVert = [&](int idx) {
                for (int k = 0; k < 8; k++) finalVertices.push_back(vertices[idx * 8 + k]);
            };
            // Két háromszög alkot egy négyszöget (quad)
            pushVert(p1); pushVert(p2); pushVert(p3);
            pushVert(p2); pushVert(p4); pushVert(p3);
        }
    }
    return finalVertices;
}

// Egységkocka generálása
std::vector<float> generateCube(float size) {
    float h = size / 2.0f;
    // Kézzel definiált vertexek: Pos(3), Norm(3), UV(2)
    std::vector<float> v = {
        // Hátlap
        -h, -h,  h,  0, 0, 1,  0, 0,  h, -h,  h,  0, 0, 1,  1, 0,  h,  h,  h,  0, 0, 1,  1, 1,
        h,  h,  h,  0, 0, 1,  1, 1, -h,  h,  h,  0, 0, 1,  0, 1, -h, -h,  h,  0, 0, 1,  0, 0,
        // Előlap
        h, -h, -h,  0, 0, -1, 0, 0, -h, -h, -h,  0, 0, -1, 1, 0, -h,  h, -h,  0, 0, -1, 1, 1,
        -h,  h, -h,  0, 0, -1, 1, 1,  h,  h, -h,  0, 0, -1, 0, 1,  h, -h, -h,  0, 0, -1, 0, 0,
        // Bal oldal
        -h, -h, -h, -1, 0, 0,  0, 0, -h, -h,  h, -1, 0, 0,  1, 0, -h,  h,  h, -1, 0, 0,  1, 1,
        -h,  h,  h, -1, 0, 0,  1, 1, -h,  h, -h, -1, 0, 0,  0, 1, -h, -h, -h, -1, 0, 0,  0, 0,
        // Jobb oldal
         h, -h,  h,  1, 0, 0,  0, 0,  h, -h, -h,  1, 0, 0,  1, 0,  h,  h, -h,  1, 0, 0,  1, 1,
         h,  h, -h,  1, 0, 0,  1, 1,  h,  h,  h,  1, 0, 0,  0, 1,  h, -h,  h,  1, 0, 0,  0, 0,
        // Teteje
        -h,  h,  h,  0, 1, 0,  0, 0,  h,  h,  h,  0, 1, 0,  1, 0,  h,  h, -h,  0, 1, 0,  1, 1,
         h,  h, -h,  0, 1, 0,  1, 1, -h,  h, -h,  0, 1, 0,  0, 1, -h,  h,  h,  0, 1, 0,  0, 0,
        // Alja
        -h, -h, -h,  0, -1, 0, 0, 0,  h, -h, -h,  0, -1, 0, 1, 0,  h, -h,  h,  0, -1, 0, 1, 1,
         h, -h,  h,  0, -1, 0, 1, 1, -h, -h,  h,  0, -1, 0, 0, 1, -h, -h, -h,  0, -1, 0, 0, 0
    };
    return v;
}

// Piramis generálása
std::vector<float> generatePyramid(float size, float height) {
    float h = size / 2.0f;
    // Normálvektorok kissé trükkösek az oldallapoknál (dőlésszög miatt)
    std::vector<float> v = {
        // Alap
        -h, 0,  h,  0, 0.5f, 0.8f,  0, 0,  h, 0,  h,  0, 0.5f, 0.8f,  1, 0,  0, height, 0, 0, 0.5f, 0.8f, 0.5f, 1,
         h, 0,  h,  0.8f, 0.5f, 0,  0, 0,  h, 0, -h,  0.8f, 0.5f, 0,  1, 0,  0, height, 0, 0.8f, 0.5f, 0, 0.5f, 1,
         h, 0, -h,  0, 0.5f, -0.8f, 0, 0, -h, 0, -h,  0, 0.5f, -0.8f, 1, 0,  0, height, 0, 0, 0.5f, -0.8f, 0.5f, 1,
        -h, 0, -h, -0.8f, 0.5f, 0,  0, 0, -h, 0,  h, -0.8f, 0.5f, 0,  1, 0,  0, height, 0, -0.8f, 0.5f, 0, 0.5f, 1,
        // Alja (két háromszög)
        -h, 0, -h,  0, -1, 0,  0, 1,  h, 0, -h,  0, -1, 0,  1, 1,  h, 0,  h,  0, -1, 0,  1, 0,
         h, 0,  h,  0, -1, 0,  1, 0, -h, 0,  h,  0, -1, 0,  0, 0, -h, 0, -h,  0, -1, 0,  0, 1
    };
    return v;
}

// Padló sík generálása (nagy kiterjedésű quad)
std::vector<float> generateFloor(float size, float textureRepeat) {
    float h = size / 2.0f;
    std::vector<float> v = {
        -h, 0,  h,  0, 1, 0,  0, 0,
         h, 0,  h,  0, 1, 0,  textureRepeat, 0,
         h, 0, -h,  0, 1, 0,  textureRepeat, textureRepeat,
         h, 0, -h,  0, 1, 0,  textureRepeat, textureRepeat,
        -h, 0, -h,  0, 1, 0,  0, textureRepeat,
        -h, 0,  h,  0, 1, 0,  0, 0
    };
    return v;
}

// --- FŐ ALKALMAZÁS OSZTÁLY ---
class HelloTriangleApplication {
public:
    void run() {
        initWindow();

        // Callback beállítása a billentyűzethez a folyamatos mozgáshoz
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, staticKeyCallback);

        initVulkan(); // Vulkan rendszer inicializálása
        mainLoop();   // Render ciklus
        cleanup();    // Takarítás
    }

private:
    GLFWwindow* window;

    // Vulkan komponensek
    VulkanContext vulkanContext;
    VulkanSwapchain vulkanSwapchain;
    VulkanPipeline vulkanPipeline;
    VulkanRenderer vulkanRenderer;

    VkDescriptorPool descriptorPool; // Memória a shader resource-oknak

    // Anyagok (Textúrák)
    Texture rockTexture;
    Texture rustTexture;

    // 3D Objektumok
    MeshObject torus;
    MeshObject cube;
    MeshObject pyramid;
    MeshObject n;
    MeshObject floor;

    // Mélység puffer erőforrások (Z-Buffering)
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkSurfaceKHR surface;

    // --- KAMERA VEZÉRLÉS ---
    // Kezdőpozíció: Kicsit hátrébb és feljebb
    glm::vec3 cameraPosition = glm::vec3(0.0f, 3.0f, -10.0f);
    float cameraSpeed = 5.0f; // Mozgási sebesség

    // Gombok állapota a sima mozgáshoz (nem "darabos" event alapú)
    std::map<int, bool> keysPressed;

    /**
     * @brief Statikus callback, ami elkapja a billentyűzet eseményeket és beállítja a `keysPressed` map-et.
     */
    static void staticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(userPtr);
            if (action == GLFW_PRESS) app->keysPressed[key] = true;
            else if (action == GLFW_RELEASE) app->keysPressed[key] = false;
        }
    }

    void initWindow() {
        glfwInit();
        // Azt mondjuk a GLFW-nak, hogy ne hozzon létre OpenGL kontextust (mert Vulkan-t használunk)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Normal Mapping", nullptr, nullptr);
    }

    void initVulkan() {
        // Inicializálási sorrend kritikus!
        vulkanContext.initInstance(window); // 1. Instance
        if (glfwCreateWindowSurface(vulkanContext.getInstance(), window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        vulkanContext.initDevice(surface); // 2. Fizikai és Logikai eszköz
        vulkanSwapchain.create(&vulkanContext, surface, window); // 3. Swapchain
        createDepthResources(); // 4. Mélység puffer
        // 5. Pipeline létrehozása (Shader betöltés, Vertex layout, stb.)
        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain, depthImageView, findDepthFormat());
        vulkanRenderer.create(&vulkanContext, &vulkanSwapchain); // 6. Renderer (Sync objects, Cmd Buffers)
        createDescriptorPool(); // 7. Descriptor Pool
        createAssets();         // 8. Textúrák betöltése
        createObjects();        // 9. Geometria létrehozása
    }

    // Mélység puffer létrehozása a helyes takarás érdekében
    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();
        VkExtent2D extent = vulkanSwapchain.getExtent();

        vulkanContext.createImage(
            extent.width, extent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // Csak a GPU látja
            depthImage,
            depthImageMemory
        );

        depthImageView = vulkanContext.createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    VkFormat findDepthFormat() {
        return VK_FORMAT_D32_SFLOAT; // 32 bites lebegőpontos mélység
    }

    void createDescriptorPool() {
        // Helyet foglalunk a Uniform Buffereknek és a Textúráknak (Combined Image Sampler)
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 10;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 50;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 50;

        if (vkCreateDescriptorPool(vulkanContext.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createAssets() {
        // --- TEXTÚRÁK BETÖLTÉSE (Assets mappából) ---
        // PBR készlet: Diffuse (Szín), Roughness (Érdesség), Normal (Domborzat)

        // Szikla textúra
        rockTexture.create(
            &vulkanContext,
            "Assets/rock/rock_diffuse.jpg",
            "Assets/rust/aerial_rocks_02_rough_4k.png",
            "Assets/rock/aerial_rocks_02_nor_gl_4k.png",
            descriptorPool,
            vulkanPipeline.getDescriptorSetLayout()
        );

        // Rozsdás fém textúra
        rustTexture.create(
            &vulkanContext,
            "Assets/rust/rusty_metal_grid_diff_4k.jpg",
            "Assets/rust/rusty_metal_grid_rough_4k.png",
            "Assets/rust/rusty_metal_grid_nor_gl_4k.png",
            descriptorPool,
            vulkanPipeline.getDescriptorSetLayout()
        );
    }

    void createObjects() {
        // 1. Torus létrehozása és beállítása
        std::vector<float> torusVec = generateTorus(1.0f, 0.4f, 32, 16);
        torus.create(&vulkanContext, torusVec); // Tangens számítás itt történik automatikusan
        torus.position = glm::vec3(2.0f, 0.0f, 0.0f);
        torus.rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        torus.rotationSpeed = 20.0f;
        torus.setTexture(rockTexture.descriptorSet);

        // 2. Kocka
        std::vector<float> cubeVec = generateCube(1.5f);
        cube.create(&vulkanContext, cubeVec);
        cube.position = glm::vec3(-2.0f, 0.0f, 0.0f);
        cube.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        cube.rotationSpeed = -30.0f;
        cube.setTexture(rustTexture.descriptorSet);

        // 3. Piramis
        std::vector<float> pyrVec = generatePyramid(1.5f, 2.0f);
        pyramid.create(&vulkanContext, pyrVec);
        pyramid.position = glm::vec3(0.0f, 2.0f, -2.0f);
        pyramid.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        pyramid.rotationSpeed = 45.0f;
        pyramid.setTexture(rockTexture.descriptorSet);

        // 4. "N" betű (kockából)
        std::vector<float> nVec = generateCube(1.0f);
        n.create(&vulkanContext, nVec);
        n.position = glm::vec3(0.0f, 0.0f, 2.0f);
        n.setTexture(rustTexture.descriptorSet);

        // 5. Padló
        std::vector<float> floorVec = generateFloor(20.0f, 4.0f);
        floor.create(&vulkanContext, floorVec);
        floor.position = glm::vec3(0.0f, -3.0f, 0.0f);
        floor.setTexture(rustTexture.descriptorSet);
    }

    void mainLoop() {
        auto lastTime = std::chrono::high_resolution_clock::now();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents(); // Ablak események (pl. bezárás, gombnyomás)

            // Delta time számítása a sima mozgáshoz (független az FPS-től)
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
            lastTime = currentTime;

            // --- KAMERA IRÁNYÍTÁS (FOLYAMATOS MOZGÁS) ---
            float velocity = cameraSpeed * deltaTime;

            // W: Előre (Z tengelyen, a 0 felé)
            if (keysPressed[GLFW_KEY_W]) cameraPosition.z += velocity;
            // S: Hátra
            if (keysPressed[GLFW_KEY_S]) cameraPosition.z -= velocity;
            // A: Balra
            if (keysPressed[GLFW_KEY_A]) cameraPosition.x -= velocity;
            // D: Jobbra
            if (keysPressed[GLFW_KEY_D]) cameraPosition.x += velocity;
            // R: Fel (Lift)
            if (keysPressed[GLFW_KEY_R]) cameraPosition.y += velocity;
            // F: Le (Lift)
            if (keysPressed[GLFW_KEY_F]) cameraPosition.y -= velocity;

            // Renderelés indítása
            std::vector<MeshObject*> objects = {&torus, &cube, &pyramid, &n, &floor};
            vulkanRenderer.drawFrame(&vulkanSwapchain, &vulkanPipeline, cameraPosition, objects);
        }
        // Kilépés előtt megvárjuk, amíg a GPU befejez mindent
        vkDeviceWaitIdle(vulkanContext.getDevice());
    }

    void cleanup() {
        // Fordított sorrendben szabadítjuk fel az erőforrásokat
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
        floor.cleanup(vulkanContext.getDevice());

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
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}