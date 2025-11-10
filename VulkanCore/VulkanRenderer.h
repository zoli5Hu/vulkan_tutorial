// VulkanCore/VulkanRenderer.h
#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h" // Szükségünk van a pipeline-ra a parancs rögzítéséhez
#include <vector> // A std::vector használatához

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    // Maximálisan ennyi képkocka lehet feldolgozás alatt párhuzamosan
    // Ennek a bevezetésével javítjuk ki a szemafor-hibát!
    const int MAX_FRAMES_IN_FLIGHT = 2;

    // Fő inicializáló és törlő metódusok
    void create(VulkanContext* context, VulkanSwapchain* swapchain);
    void cleanup();

    // A fő rajzoló függvény (ez veszi át a drawFrame logikát a main.cpp-ből)
    void drawFrame(VulkanSwapchain* swapchain, VulkanPipeline* pipeline);

private:
    // --- Tagváltozók (áthelyezve a main.cpp-ből) ---

    // JAVÍTVA: Nem egy, hanem annyi parancspufferünk lesz,
    // ahány "frame in flight" van (MAX_FRAMES_IN_FLIGHT)
    std::vector<VkCommandBuffer> commandBuffers;

    // --- Szinkronizációs objektumok (MIND PRIVATE) ---

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    // Az eredeti fence vektor (CSAK EGYSZER, ITT)
    std::vector<VkFence> inFlightFences;

    // Az új, képeket követő fence vektor
    std::vector<VkFence> imagesInFlight;

    // Nyomon követi az aktuális frame indexet (0 vagy 1)
    uint32_t currentFrame = 0;

    // --- Függőségek ---
    VulkanContext* context; // Pointer a "motorra"

    // --- Privát segédfüggvények (áthelyezve a main.cpp-ből) ---

    // Létrehozza a parancspuffereket
    void createCommandBuffers();

    // Létrehozza a szinkronizációs objektumokat (JAVÍTOTT SZIGNATÚRA)
    void createSyncObjects(VulkanSwapchain* swapchain);

    // Rögzíti a rajzolási parancsokat az adott parancspufferbe
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VulkanSwapchain* swapchain, VulkanPipeline* pipeline);
};