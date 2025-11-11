/**
 * @file main.cpp
 * @brief Fő alkalmazásfájl a Vulkan grafikus programhoz.
 *
 * Ez a fájl tartalmazza a HelloTriangleApplication osztályt, amely felelős a Vulkan
 * inicializálásáért, a GLFW ablak kezeléséért, a 3D objektumok betöltéséért
 * és a fő renderelési ciklus (mainLoop) futtatásáért.
 */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
/**
 * @brief Standard C++ fejlécek a bemenet/kimenet, kivételkezelés, memóriakezelés és konténerek számára.
 */
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <chrono> // <- IDŐMÉRÉSHEZ (DeltaTime számításhoz)
#include <cmath> // A math.h függvényekhez
#include <iterator> // std::begin és std::end használatához (ModelData inicializálás)

// GLM include-ok
/**
 * @brief GLM kényszerítés: a GLM vektor/mátrix függvényei radiánban várják a szögeket.
 */
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

// 1. BEILLESZTJÜK AZ ÖSSZES OSZTÁLYUNKAT
/**
 * @brief Az alkalmazás Vulkan komponensei: Context (eszközkezelés), Swapchain (képcserélő lánc),
 * Pipeline (állapotgépek és shaderek), Renderer (rajzolási logika) és MeshObject (3D modellek).
 */
#include "VulkanCore/VulkanContext.h"
#include "VulkanCore/VulkanSwapchain.h"
#include "VulkanCore/VulkanPipeline.h"
#include "VulkanCore/VulkanRenderer.h"
#include "VulkanCore/MeshObject.h" // A saját objektum osztályunk

/**
 * @brief Az ablak fix szélessége és magassága.
 */
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
using namespace std;

/**
 * @namespace ModelData
 * @brief Segédnévtér a betöltött modell-adatok tárolására.
 *
 * A modellek adatai (float tömbök) a cmake fájlban fordított '.inc' fájlokból kerülnek beillesztésre.
 * Az adatstruktúra: (X, Y, Z, U, V) floatok listája.
 */
namespace ModelData {
    // 1. Torus (Mozgó, Kitöltött)
    /** @brief A torus vertex adatai. */
    static constexpr float torusVertices[] = {
        #include "./Modells/Datas/torus.inc"
    };

    // 2. Kocka (Statikus, Wireframe)
    // IDEIGLENESEN: Az Icosphere (ico.inc) adatai vannak használva a Wireframe kockához.
    /** @brief Az icosphere (ideiglenes kocka) vertex adatai. */
    static constexpr float icoVertices[] = {
        #include "./Modells/Datas/ico.inc"
    };

    // 3. Piramis (Mozgó, Kitöltött)
    // IDEIGLENESEN: A Cone (cone.inc) adatai vannak használva a Piramishoz.
    /** @brief A kúp (cone, ideiglenes piramis) vertex adatai. */
    static constexpr float pyramidVertices[] = {
        #include "./Modells/Datas/cone.inc"
    };
}

/**
 * @class HelloTriangleApplication
 * @brief A fő alkalmazás osztály, amely a Vulkan inicializálását, futtatását és leállítását kezeli.
 */
class HelloTriangleApplication
{
public:
    /**
     * @brief A teljes alkalmazási folyamatot elindító függvény.
     * Elindítja az ablakot, inicializálja a Vulkan-t, futtatja a fő ciklust, majd tisztít.
     */
    void run()
    {
        initWindow();
        // 2. Callback regisztrálása a window objektummal (ez a 'this')
        // Elmenti az osztály (this) pointerét a GLFW ablak felhasználói adatává.
        glfwSetWindowUserPointer(window, this);
        // Regisztrálja a statikus billentyűzetkezelő callback függvényt.
        glfwSetKeyCallback(window, staticKeyCallback);

        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    /** @brief A GLFW ablak objektum. */
    GLFWwindow* window;

    // --- Core Komponensek ---
    /** @brief A Vulkan példányt és logikai eszközt kezelő osztály. */
    VulkanContext vulkanContext;
    /** @brief A képcserélő láncot (swapchain) kezelő osztály. */
    VulkanSwapchain vulkanSwapchain;
    /** @brief A render pass-t és a grafikus pipeline-okat (kitöltött, wireframe) kezelő osztály. */
    VulkanPipeline vulkanPipeline;
    /** @brief A parancspuffereket, szinkronizációt és drawFrame logikát kezelő osztály. */
    VulkanRenderer vulkanRenderer;

    // --- Kamera Állapot ---
    /** @brief A kamera világkoordinátás pozíciója. */
    glm::vec3 cameraPosition = glm::vec3(0, 2, -10.0f);
    /**
     * @brief A kamera mozgási sebessége (méter/másodperc).
     * Használva van a deltaTime-mal a sebességfüggetlen mozgáshoz.
     */
    const float cameraSpeed = 1.5f; // Pl. 1.5 méter/másodperc

    // --- Input Állapot ---
    /** @brief Térkép (map) a lenyomott billentyűk állapotának tárolására (key code -> lenyomva/felengedve). */
    std::map<int, bool> keysPressed;

    // --- Alkalmazás-specifikus objektumok ---
    /** @brief A Vulkan renderelési felület (összekapcsolja a Vulkan-t a GLFW ablakkal). */
    VkSurfaceKHR surface;

    // --- 3D Erőforrások (Depth) ---
    /** @brief A mélységi teszthez használt kép (image) erőforrás. */
    VkImage depthImage;
    /** @brief A depthImage-hez allokált memória. */
    VkDeviceMemory depthImageMemory;
    /** @brief A depthImage-hez létrehozott képnézet (image view). */
    VkImageView depthImageView;
    /** @brief A mélységi kép formátuma (D32_SFLOAT). */
    VkFormat depthFormat;

    // TÖRÖLVE a régi: VkBuffer vertexBuffer és VkDeviceMemory vertexBufferMemory

    // --- ÚJ: Objektumok ---
    /** @brief Az első mozgó MeshObject (Torus). */
    MeshObject torus;
    /** @brief A második statikus MeshObject (Icosphere/Kocka), amit wireframe-ként rajzolunk. */
    MeshObject cube;    // Wireframe alakzat (objektum lista 1. indexe)
    /** @brief A harmadik mozgó MeshObject (Cone/Piramis). */
    MeshObject pyramid; // Másik mozgó alakzat

    // --- Static Callback a GLFW-hez (a C++ osztályon BELÜL) ---
    /**
     * @brief Statikus callback függvény a billentyűzet események kezelésére.
     * Ezt a GLFW hívja meg. Használja a window user pointert az alkalmazáspéldány eléréséhez.
     */
    static void staticKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            // A user pointer visszakonvertálása a HelloTriangleApplication példányra.
            HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(userPtr);

            // Billentyű lenyomás/felengedés állapotának frissítése a keysPressed térképen.
            if (action == GLFW_PRESS) {
                app->keysPressed[key] = true;
            } else if (action == GLFW_RELEASE) {
                app->keysPressed[key] = false;
            }

            // ESC lenyomásakor az ablak bezárásának kérése.
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }
    }


    // --- Core segédfüggvények ---

    /**
     * @brief Létrehozza és inicializálja a 3D MeshObject-eket.
     *
     * Átkopírozza a statikus modell adatokat (ModelData::*) a dinamikus MeshObject-ekbe,
     * majd beállítja a pozíciót és a forgási paramétereket.
     */
    void createObjects() {
        // 1. Torus inicializálása
        // Létrehoz egy dinamikus float vektort a C-stílusú tömbből (torus.inc tartalmából).
        std::vector<float> torusVector(std::begin(ModelData::torusVertices), std::end(ModelData::torusVertices));
        torus.create(&vulkanContext, torusVector);
        torus.position = glm::vec3(-3.0f, 0.0f, 0.0f);
        torus.rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        torus.rotationSpeed = 40.0f; // Mozgás engedélyezése

        // 2. Kocka (Icosphere) inicializálása (Wireframe-ként használandó)
        // A MeshObject a vertex buffer memóriáját a Vulkan eszközre másolja.
        std::vector<float> cubeVector(std::begin(ModelData::icoVertices), std::end(ModelData::icoVertices));
        cube.create(&vulkanContext, cubeVector);
        cube.position = glm::vec3(3.0f, 0.0f, 0.0f);
        cube.rotationSpeed = 0.0f; // Statikus

        // 3. Piramis (Cone) inicializálása
        // Méret ellenőrzés (a kódban maradt, de az iteratoros inicializálás mellett opcionális)
        const size_t pyramidSize = sizeof(ModelData::pyramidVertices) / sizeof(ModelData::pyramidVertices[0]);
        std::vector<float> pyramidVector(std::begin(ModelData::pyramidVertices), std::end(ModelData::pyramidVertices));
        pyramid.create(&vulkanContext, pyramidVector);
        pyramid.position = glm::vec3(0.0f, 3.0f, 0.0f);
        pyramid.rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        pyramid.rotationSpeed = 60.0f; // Mozgás engedélyezése
    }

    /**
     * @brief Létrehozza a mélységi (Depth) pufferhez szükséges Vulkan erőforrásokat.
     *
     * Létrehozza a VkImage-et, allokálja a memóriát, létrehozza a VkImageView-t,
     * és végrehajt egy egyszeri parancsot a kép elrendezésének (layout) optimalizálására.
     */
    void createDepthResources() {
        // A mélységi formátum beállítása. VK_FORMAT_D32_SFLOAT = 32 bites float mélységi puffer.
        depthFormat = VK_FORMAT_D32_SFLOAT;
        VkExtent2D swapChainExtent = vulkanSwapchain.getExtent();

        // Mélységi kép létrehozása
        vulkanContext.createImage(
            swapChainExtent.width,
            swapChainExtent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL, // Optimális elrendezés a hardveres gyorsításhoz
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, // Mélységi/Stencil csatolmányként való használat
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // GPU-specifikus gyors memória
            depthImage,
            depthImageMemory
        );
        // Képnézet létrehozása a mélységi képhez
        depthImageView = vulkanContext.createImageView(
            depthImage,
            depthFormat,
            VK_IMAGE_ASPECT_DEPTH_BIT // Mélységi bit használata
        );

        // Kép elrendezésének átváltása a kezdeti UNDEFINED-ből a végleges DEPTH_STENCIL_ATTACHMENT_OPTIMAL-ra.
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


    /**
     * @brief Inicializálja az összes Vulkan komponenst.
     *
     * Létrehozza a példányt, felületet, logikai eszközt, swapchain-t, mélységi erőforrásokat,
     * 3D objektumokat, pipeline-t és a renderelőt.
     */
    void initVulkan()
    {
        vulkanContext.initInstance(window);
        // GLFW segítségével Vulkan felület létrehozása (összekapcsolás a Vulkan és a platform ablakrendszere között).
        if (glfwCreateWindowSurface(vulkanContext.getInstance(), window, nullptr, &surface) != VK_SUCCESS)
        {
            throw runtime_error("failed to create window surface!");
        }
        vulkanContext.initDevice(surface);
        vulkanSwapchain.create(&vulkanContext, surface, window);
        createDepthResources(); // Mélységi puffer létrehozása

        createObjects(); // 3D modellek létrehozása

        // A pipeline-hoz kell a swapchain és a depth infó (formátumok, nézetek).
        vulkanPipeline.create(&vulkanContext, &vulkanSwapchain, depthImageView, depthFormat);
        // A renderelőhöz kell a VulkanContext és a Swapchain infó (parancspufferek, szinkronizáció).
        vulkanRenderer.create(&vulkanContext, &vulkanSwapchain);
    }

    /**
     * @brief Inicializálja a GLFW ablakot.
     * Beállítja a Vulkan-kompatibilis, nem átméretezhető ablakot.
     */
    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Kikényszeríti a Vulkan (nem OpenGL) használatát.
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Megakadályozza az ablak átméretezését.
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Multi Object", nullptr, nullptr);
    }

    /**
     * @brief A program fő ciklusa.
     *
     * Kezeli az időmérést (deltaTime), az inputokat és a renderelési hívást minden kereten.
     */
    void mainLoop()
    {
        // Az időmérés inicializálása.
        auto lastTime = std::chrono::high_resolution_clock::now();

        std::cout << "kacsa a Main Loop (Javított Kód)" << std::endl;
        while (!glfwWindowShouldClose(window)) {

            // Valós deltaTime kiszámítása: az utolsó és a jelenlegi képkocka közötti eltelt idő.
            // Ez biztosítja az FPS-től független mozgást.
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
            lastTime = currentTime;

            glfwPollEvents(); // GLFW események (pl. billentyűzet) kezelése.

            // --- INPUT KEZELÉS (FPS FÜGGETLEN) ---
            float velocity = cameraSpeed * deltaTime; // Az elmozdulás kiszámítása

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
            // A Wireframe objektum a 1. indexen van (cube), a renderer ezt használja a pipeline váltására.
            std::vector<MeshObject*> objects = {&torus, &cube, &pyramid};

            // JAVÍTVA: drawFrame hívás frissítve
            // Kezdeményezi az aktuális keret kirajzolását.
            vulkanRenderer.drawFrame(&vulkanSwapchain, &vulkanPipeline, cameraPosition, objects);
        }

        // Megvárja, amíg a GPU befejezi az összes függőben lévő műveletet, mielőtt a tisztítás elindulna.
        vkDeviceWaitIdle(vulkanContext.getDevice());
    }

    /**
     * @brief Felszabadítja az összes lefoglalt Vulkan és GLFW erőforrást.
     *
     * A felszabadítás fordított sorrendben történik, mint a létrehozás.
     */
    void cleanup()
    {
        // Renderelési réteg tisztítása (szemaforok, kerítések)
        vulkanRenderer.cleanup();
        // Pipeline réteg tisztítása (render pass, pipeline-ok, framebuffer-ek)
        vulkanPipeline.cleanup();
        // Swapchain tisztítása (image view-k, swapchain objektum)
        vulkanSwapchain.cleanup();

        // Objektumok felszabadítása (vertex pufferek, memóriák)
        torus.cleanup(vulkanContext.getDevice());
        cube.cleanup(vulkanContext.getDevice());
        pyramid.cleanup(vulkanContext.getDevice());

        // Mélységi erőforrások felszabadítása
        vkDestroyImageView(vulkanContext.getDevice(), depthImageView, nullptr);
        vkDestroyImage(vulkanContext.getDevice(), depthImage, nullptr);
        vkFreeMemory(vulkanContext.getDevice(), depthImageMemory, nullptr);

        // Vulkan felület felszabadítása
        vkDestroySurfaceKHR(vulkanContext.getInstance(), surface, nullptr);
        // Vulkan környezet felszabadítása (logikai eszköz, parancskészlet, debug messenger, példány)
        vulkanContext.cleanup();

        // GLFW ablak bezárása és könyvtár leállítása
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

/**
 * @brief A program belépési pontja.
 *
 * Létrehozza az alkalmazás példányát, futtatja, és kezeli a futásidejű kivételeket.
 */
int main()
{
    HelloTriangleApplication app;

    try
    {
        // Az alkalmazás futtatása
        app.run();
    }
    catch (const exception& e)
    {
        // Kivétel elkapása és hibaüzenet kiírása a szabványos hibakimenetre.
        cerr << e.what() << endl;
        return EXIT_FAILURE; // Hibakódos visszatérés
    }

    return EXIT_SUCCESS; // Sikeres visszatérés
}