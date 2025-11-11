/**
 * @file VulkanPipeline.cpp
 * @brief Megvalósítja a VulkanPipeline osztályt.
 *
 * Ez az osztály felelős a Render Pass, a Pipeline Layout (uniform/push constant definíciók),
 * a grafikus pipeline-ok (kitöltött és wireframe) és a Framebuffer-ek létrehozásáért.
 * Lényegében ez definiálja a GPU renderelési állapotát.
 */
// VulkanCore/VulkanPipeline.cpp
#include "VulkanPipeline.h"

#include <array>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
using namespace std;

/**
 * @brief Konstruktor.
 * Inicializálja a Vulkan leírókat VK_NULL_HANDLE-re.
 */
VulkanPipeline::VulkanPipeline() : context(nullptr), renderPass(VK_NULL_HANDLE),
                                   pipelineLayout(VK_NULL_HANDLE), graphicsPipeline(VK_NULL_HANDLE), wireframePipeline(VK_NULL_HANDLE) {
}

/**
 * @brief Destruktor.
 */
VulkanPipeline::~VulkanPipeline() {
}

/**
 * @brief Létrehozza az összes pipeline-hoz kapcsolódó Vulkan objektumot.
 *
 * @param ctx A Vulkan környezet mutatója.
 * @param swapchain A swapchain mutatója (formátum és extent lekéréséhez).
 * @param depthImageView A mélységi kép nézete (framebufferhez).
 * @param depthFormat A mélységi kép formátuma (render pass-hoz).
 */
void VulkanPipeline::create(VulkanContext* ctx, VulkanSwapchain* swapchain, VkImageView depthImageView, VkFormat depthFormat) {
    this->context = ctx;

    createRenderPass(swapchain->getImageFormat(), depthFormat);
    createGraphicsPipeline(); // Kitöltött (Solid) pipeline
    createWireframePipeline(); // Drótváz (Wireframe) pipeline
    createFramebuffers(swapchain, depthImageView);
}

/**
 * @brief Felszabadítja az összes lefoglalt pipeline erőforrást.
 * A felszabadítás a létrehozással ellentétes sorrendben történik.
 */
void VulkanPipeline::cleanup() {
    // Framebuffer-ek felszabadítása
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
    }
    // Pipeline-ok felszabadítása
    vkDestroyPipeline(context->getDevice(), wireframePipeline, nullptr);
    vkDestroyPipeline(context->getDevice(), graphicsPipeline, nullptr);
    // Pipeline elrendezés felszabadítása
    vkDestroyPipelineLayout(context->getDevice(), pipelineLayout, nullptr);
    // Render Pass felszabadítása
    vkDestroyRenderPass(context->getDevice(), renderPass, nullptr);
}

/**
 * @brief Létrehozza a Render Pass-t.
 * A Render Pass határozza meg, hogy a renderelés során milyen csatolmányokat (attachments)
 * használunk (pl. színes, mélységi), hogyan kezeljük őket (törlés, tárolás),
 * és hogyan függnek egymástól az al-lépések (subpasses).
 *
 * @param swapchainFormat A Swapchain képek formátuma (ez lesz a szín csatolmány).
 * @param depthFormat A mélységi puffer formátuma.
 */
void VulkanPipeline::createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat) {

    // 1. Szín Csatolmány (Color Attachment) leírása
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat; // A swapchain-től kapott formátum
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Nincs multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Törölje a képet (háttérszínre) a render pass elején
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Tárolja az eredményt a render pass végén
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Nem érdekel minket a korábbi elrendezés
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // A kép készen áll a megjelenítésre

    // 2. Mélységi Csatolmány (Depth Attachment) leírása
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Törölje a mélységi puffert minden keret elején
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Nem kell tárolni a mélységi adatot a pass végén
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Optimális elrendezés a mélységi teszthez

    // 3. Csatolmány Referenciák (melyik csatolmányt használjuk a subpass-ban)
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // A 0. indexű csatolmány (colorAttachment)
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Optimális elrendezés a szín írásához

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1; // Az 1. indexű csatolmány (depthAttachment)
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Optimális elrendezés a mélységi teszthez/íráshoz

    // 4. Al-lépés (Subpass) leírása
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Grafikus pipeline-hoz kötve
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // Szín csatolmány hozzárendelése
    subpass.pDepthStencilAttachment = &depthAttachmentRef; // Mélységi csatolmány hozzárendelése

    // 5. Subpass Függőség (Dependency)
    // Ez biztosítja, hogy a render pass megvárja, amíg a kép elérhetővé válik,
    // mielőtt elkezdené írni a szín- és mélységi csatolmányokat.
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // A render pass előtti műveletek
    dependency.dstSubpass = 0; // A mi (első) subpass-unk
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    // 6. Render Pass Létrehozása
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
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

/**
 * @brief Létrehozza a normál, kitöltött (Solid Fill) grafikus pipeline-t.
 */
void VulkanPipeline::createGraphicsPipeline()
{
    // 1. Shaderek betöltése
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // 2. Shader Fázisok (Stages) beállítása
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main"; // A shader belépési pontja

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // 3. Dinamikus Állapotok (Dynamic States)
    // Ezeket az állapotokat (Viewport, Scissor) nem sütjük bele a pipeline-ba,
    // hanem minden keretben dinamikusan állítjuk be a parancspufferben.
    vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 4. Viewport és Scissor (üresen hagyva, mert dinamikusak)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 5. Vertex Bemenet (Vertex Input)
    // Meghatározza, hogyan néz ki egy vertex a memóriában.
    const size_t g_cubeVertexSize = sizeof(float) * 5; // X,Y,Z,U,V

    // Kötés (Binding): A teljes vertex adat (5 float)
    const VkVertexInputBindingDescription bindingInfo = {
        .binding   = 0, // 0. kötési pont
        .stride    = (uint32_t)g_cubeVertexSize, // Mekkora egy vertex
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, // Vertexenként lépjen
    };

    // Attribútum (Attribute): A pozíció (3 float)
    const VkVertexInputAttributeDescription attributeInfo = {
        .location = 0, // 'layout(location = 0)' a shaderben
        .binding  = 0, // A 0. kötésből jön
        .format   = VK_FORMAT_R32G32B32_SFLOAT, // 3 db 32 bites float
        .offset   = 0u, // 0 eltolás a vertex kezdetétől
    };
    // MEGJEGYZÉS: Az UV (U,V) attribútumok nincsenek bekötve, mert a jelenlegi shader nem használja őket.

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions    = &attributeInfo;

    // 6. Input Assembly (Primitívek összerakása)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Háromszög listaként rajzolunk
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 7. Raszterizáló (Rasterizer)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Kitöltött háromszögek
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Hátlap eldobása
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Óramutatóval ellentétes a háromszög iránya
    rasterizer.depthBiasEnable = VK_FALSE;

    // 8. Multisampling (Élsimítás) - Nincs használatban
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 9. Színkeverés (Color Blending) - Nincs használatban (Opaque renderelés)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 10. Pipeline Layout (Uniformok, Push Konstansok)
    // Definiál egy Push Konstans tartományt (az MVP mátrixnak) a Vertex Shader számára.
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4); // MVP mátrix mérete

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Nincsenek Descriptor Set-ek
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    // A Pipeline Layout létrehozása (ez mindkét pipeline-hoz (solid/wireframe) közös lesz)
    if (vkCreatePipelineLayout(context->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // 11. Mélységi/Stencil Állapot (Depth/Stencil State)
    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE; // Mélységi teszt engedélyezése
    depthStencilState.depthWriteEnable = VK_TRUE; // Mélységi puffer írásának engedélyezése
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS; // A közelebbi objektumok (kisebb mélység) nyernek
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE; // Stencil teszt nincs használatban

    // 12. Grafikus Pipeline Létrehozása
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2; // Vertex + Fragment shader
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout; // A korábban létrehozott elrendezés
    pipelineInfo.renderPass = renderPass; // A korábban létrehozott render pass
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;


    if (vkCreateGraphicsPipelines(context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // A shader modulokra már nincs szükség a pipeline létrehozása után.
    vkDestroyShaderModule(context->getDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(context->getDevice(), vertShaderModule, nullptr);
}

/**
 * @brief Létrehozza a drótváz (Wireframe) grafikus pipeline-t.
 *
 * Ez a függvény szinte teljesen megegyezik a `createGraphicsPipeline`-nel,
 * az EGYETLEN különbség a raszterizáló állapotában van:
 * `rasterizer.polygonMode = VK_POLYGON_MODE_LINE;`
 */
void VulkanPipeline::createWireframePipeline()
{
    // 1. Shaderek betöltése (Ugyanazok, mint a solid pipeline-nál)
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // 2. Shader Fázisok (Ugyanazok)
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // 3. Dinamikus Állapotok (Ugyanazok)
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 4. Viewport és Scissor (Ugyanazok)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 5. Vertex Bemenet (Ugyanazok)
    const size_t g_cubeVertexSize = sizeof(float) * 5;
    const VkVertexInputBindingDescription bindingInfo = {
        .binding   = 0,
        .stride    = (uint32_t)g_cubeVertexSize,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    const VkVertexInputAttributeDescription attributeInfo = {
        .location = 0,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32_SFLOAT,
        .offset   = 0u,
    };
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions    = &attributeInfo;

    // 6. Input Assembly (Ugyanaz)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 7. Raszterizáló (A KÜLÖNBSÉG)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    /**************************************************************************/
    /* @brief KÜLÖNBSÉG: A poligon mód vonalakra (LINE) van állítva kitöltés (FILL) helyett. */
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    /**************************************************************************/
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // 8. Multisampling (Ugyanaz)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 9. Színkeverés (Ugyanaz)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 10. Mélységi/Stencil Állapot (Ugyanaz)
    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE;

    // 11. Grafikus Pipeline Létrehozása
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer; // A módosított raszterizáló használata
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout; // Ugyanazt a layoutot használja, mint a solid pipeline!
    pipelineInfo.renderPass = renderPass; // Ugyanazt a render pass-t használja
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    // A wireframe pipeline létrehozása
    if (vkCreateGraphicsPipelines(context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &wireframePipeline) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create wireframe graphics pipeline!");
    }

    // Shader modulok felszabadítása
    vkDestroyShaderModule(context->getDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(context->getDevice(), vertShaderModule, nullptr);
}

/**
 * @brief Létrehozza a Framebuffer-eket.
 *
 * Egy Framebuffer-t hoz létre a Swapchain minden egyes képéhez.
 * A Framebuffer összeköti a Render Pass-t a konkrét képnézetekkel (ImageViews),
 * amelyeket a renderelés során használni fogunk (szín és mélység).
 *
 * @param swapchain A swapchain objektum (a képnézetek és a méret lekéréséhez).
 * @param depthImageView A mélységi kép nézete (minden framebufferhez ugyanazt használjuk).
 */
void VulkanPipeline::createFramebuffers(VulkanSwapchain* swapchain, VkImageView depthImageView) {
    const auto& imageViews = swapchain->getImageViews();
    VkExtent2D extent = swapchain->getExtent();

    swapChainFramebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {

        // A csatolmányok listája: [0] = Szín, [1] = Mélység
        std::array<VkImageView, 2> attachments = {
            imageViews[i], // A swapchain aktuális képnézete
            depthImageView // A mélységi képnézet
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass; // Megadja, hogy melyik render pass-szal kompatibilis
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

/**
 * @brief Beolvas egy fájlt (pl. SPV shader) binárisan egy karakter vektorba.
 *
 * @param filename A beolvasandó fájl neve.
 * @return A fájl tartalmát tartalmazó std::vector<char>.
 */
vector<char> VulkanPipeline::readFile(const string& filename)
{
    // Megnyitás bináris módban (ios::binary) és a fájl végére ugrás (ios::ate).
    ifstream file(filename, ios::ate | ios::binary);
    if (!file.is_open())
    {
        throw runtime_error("failed to open file!");
    }
    // A fájl méretének lekérése (mivel a végén állunk).
    size_t fileSize = (size_t)file.tellg();
    vector<char> buffer(fileSize);
    // Visszaugrás a fájl elejére.
    file.seekg(0);
    // Adatok beolvasása a bufferbe.
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

/**
 * @brief Létrehoz egy VkShaderModule-t a beolvasott SPV kódból.
 *
 * @param code A SPV bájtkódot tartalmazó karakter vektor.
 * @return A létrehozott VkShaderModule leíró.
 */
VkShaderModule VulkanPipeline::createShaderModule(const vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    // A Vulkan uint32_t pointert vár, ezért a char pointert át kell kasztolni.
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(context->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}