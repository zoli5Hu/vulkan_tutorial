/**
 * @file VulkanPipeline.cpp
 * @brief Grafikai pipeline megvalósítása: 11 float/vertex (Tangent támogatás) és 3 textúra binding (Normal Map).
 */
#include "VulkanPipeline.h"

#include <array>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <iostream>

using namespace std;

VulkanPipeline::VulkanPipeline() : context(nullptr), renderPass(VK_NULL_HANDLE),
                                   pipelineLayout(VK_NULL_HANDLE), graphicsPipeline(VK_NULL_HANDLE),
                                   wireframePipeline(VK_NULL_HANDLE), descriptorSetLayout(VK_NULL_HANDLE),
                                   shadowSetLayout(VK_NULL_HANDLE) {
}

VulkanPipeline::~VulkanPipeline() {
}

void VulkanPipeline::create(VulkanContext* ctx, VulkanSwapchain* swapchain, VkImageView depthImageView, VkFormat depthFormat) {
    this->context = ctx;

    // A renderelési folyamat szakaszainak felépítése
    createRenderPass(swapchain->getImageFormat(), depthFormat);
    createGraphicsPipeline();
    createFramebuffers(swapchain, depthImageView);
}

void VulkanPipeline::cleanup() {
    // GPU erőforrások felszabadítása a függőségek figyelembevételével
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
    }
    vkDestroyPipeline(context->getDevice(), wireframePipeline, nullptr);
    vkDestroyPipeline(context->getDevice(), graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(context->getDevice(), pipelineLayout, nullptr);
    vkDestroyRenderPass(context->getDevice(), renderPass, nullptr);

    vkDestroyDescriptorSetLayout(context->getDevice(), descriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(context->getDevice(), shadowSetLayout, nullptr);
}

void VulkanPipeline::createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat) {
    // 1. Szín attachment (Color Buffer): Hogyan kezeljük a pixeleket a rajzolás során
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;    // Keret elején töröljük a tartalmat
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Eredmény mentése a képernyőre
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Megjelenítésre optimalizált formátum

    // 2. Mélység attachment (Depth Buffer): A 3D objektumok helyes takarásáért
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthAttachmentRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    // Subpass: A pipeline egyetlen fázisa, ami a fenti pufferre dolgozik
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Szinkronizáció: Megvárjuk, amíg az előző képkocka megjelenítése befejeződik az írás előtt
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(context->getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void VulkanPipeline::createGraphicsPipeline() {
    // Sharderek betöltése és modulok létrehozása
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // Vertex és Fragment shader szakaszok definiálása
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // --- VERTEX INPUT KONFIGURÁCIÓ (11 float stride) ---
    // Adatszerkezet: Pozíció(3) + Normál(3) + UV(2) + Tangens(3)
    VkVertexInputBindingDescription bindingInfo = { 0, sizeof(float) * 11, VK_VERTEX_INPUT_RATE_VERTEX };

    // 4 attribútum leírása a shader bemenetekhez (layout locations)
    std::array<VkVertexInputAttributeDescription, 4> attributeInfos;
    // 0: Pozíció (vec3)
    attributeInfos[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
    // 1: Normál (vec3)
    attributeInfos[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)};
    // 2: UV koordináta (vec2)
    attributeInfos[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT,    6 * sizeof(float)};
    // 3: Tangens (vec3) - Normal Mapping-hez kritikus
    attributeInfos[3] = {3, 0, VK_FORMAT_R32G32B32_SFLOAT, 8 * sizeof(float)};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeInfos.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeInfos.data();

    // Geometria típusának beállítása (háromszög lista)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Viewport állapot (Dinamikusan lesz kezelve a rajzoláskor)
    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Raszterizáló beállítások
    VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // Minden oldal látszódik (pl. padlóhoz hasznos)
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // Élsimítás (Multisampling) - kikapcsolva
    VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Mélységteszt (Z-Buffer) aktiválása
    VkPipelineDepthStencilStateCreateInfo depthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;

    // Színkeverés (Blending) - kikapcsolva az átlátszatlan tárgyakhoz
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Dinamikus állapotok engedélyezése az ablakméretezés támogatásához
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // --- DESCRIPTOR LAYOUTS KONFIGURÁCIÓJA ---

    // Set 0: Anyag textúrák (Diffuse, Roughness, Normal Map)
    std::vector<VkDescriptorSetLayoutBinding> bindings(3);
    bindings[0] = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    bindings[1] = {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    bindings[2] = {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(context->getDevice(), &layoutInfo, nullptr, &descriptorSetLayout);

    // Set 1: Árnyéktérkép (Shadow Map)
    std::vector<VkDescriptorSetLayoutBinding> shadowBindings = { {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} };
    VkDescriptorSetLayoutCreateInfo shadowLayoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    shadowLayoutInfo.bindingCount = 1;
    shadowLayoutInfo.pBindings = shadowBindings.data();

    vkCreateDescriptorSetLayout(context->getDevice(), &shadowLayoutInfo, nullptr, &shadowSetLayout);

    // Pipeline Layout: Meghatározza, hogyan férnek hozzá a shaderek az adatokhoz
    std::array<VkDescriptorSetLayout, 2> setLayouts = {descriptorSetLayout, shadowSetLayout};

    // Push Constants: Gyors adatátvitel mátrixokhoz (128 byte: Model + MVP)
    VkPushConstantRange pushConstantRange{VK_SHADER_STAGE_VERTEX_BIT, 0, 2 * sizeof(glm::mat4)};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    vkCreatePipelineLayout(context->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout);

    // A grafikai csővezeték (Pipeline) összeállítása a beállítások alapján
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;

    if (vkCreateGraphicsPipelines(context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // A shader modulokra nincs szükség a pipeline létrejötte után
    vkDestroyShaderModule(context->getDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(context->getDevice(), vertShaderModule, nullptr);
}

void VulkanPipeline::createWireframePipeline() {
    // Drótvázas megjelenítéshez külön pipeline definiálható VK_POLYGON_MODE_LINE-nal
}

void VulkanPipeline::createFramebuffers(VulkanSwapchain* swapchain, VkImageView depthImageView) {
    const auto& imageViews = swapchain->getImageViews();
    VkExtent2D extent = swapchain->getExtent();

    swapChainFramebuffers.resize(imageViews.size());

    // Framebuffer létrehozása minden egyes swapchain képhez (Szín + Mélység)
    for (size_t i = 0; i < imageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = { imageViews[i], depthImageView };

        VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

vector<char> VulkanPipeline::readFile(const string& filename) {
    // Bináris SPIR-V fájl beolvasása
    ifstream file(filename, ios::ate | ios::binary);
    if (!file.is_open()) throw runtime_error("failed to open file: " + filename);
    size_t fileSize = (size_t)file.tellg();
    vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

VkShaderModule VulkanPipeline::createShaderModule(const vector<char>& code) {
    // Nyers bájtokból Vulkan shader modul objektum létrehozása
    VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    vkCreateShaderModule(context->getDevice(), &createInfo, nullptr, &shaderModule);
    return shaderModule;
}