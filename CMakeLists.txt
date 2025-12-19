cmake_minimum_required(VERSION 3.12)
project(vulkan)

set(CMAKE_CXX_STANDARD 17)

# Vulkan
find_package(Vulkan REQUIRED glslc)

# GLFW
include(FetchContent)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
)
FetchContent_MakeAvailable(glfw)

# GLM
include(FetchContent)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)


# --- SHADEREK BEÁLLÍTÁSA ---

set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

# 1. Vertex Shader (Main)
set(VERT_SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert)
set(VERT_SHADER_SPV ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vert.spv)

# 2. Fragment Shader (Main)
set(FRAG_SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag)
set(FRAG_SHADER_SPV ${CMAKE_CURRENT_SOURCE_DIR}/shaders/frag.spv)

# 3. Shadow Vertex Shader (Árnyék Pass) - EZ AZ ÚJ
set(SHADOW_VERT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_shader.vert)
set(SHADOW_VERT_SPV ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shadow_vert.spv)


# --- FORDÍTÁSI PARANCSOK ---

# shader.vert -> vert.spv
add_custom_command(
        OUTPUT ${VERT_SHADER_SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND Vulkan::glslc ${VERT_SHADER_SRC} -o ${VERT_SHADER_SPV}
        DEPENDS ${VERT_SHADER_SRC}
        COMMENT "Compiling vertex shader"
)

# shader.frag -> frag.spv
add_custom_command(
        OUTPUT ${FRAG_SHADER_SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND Vulkan::glslc ${FRAG_SHADER_SRC} -o ${FRAG_SHADER_SPV}
        DEPENDS ${FRAG_SHADER_SRC}
        COMMENT "Compiling fragment shader"
)

# shadow_shader.vert -> shadow_vert.spv (EZ AZ ÚJ BLOKK)
add_custom_command(
        OUTPUT ${SHADOW_VERT_SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND Vulkan::glslc ${SHADOW_VERT_SRC} -o ${SHADOW_VERT_SPV}
        DEPENDS ${SHADOW_VERT_SRC}
        COMMENT "Compiling shadow vertex shader"
)

# --- TARGET LÉTREHOZÁSA ---

# Itt adjuk hozzá a listához a ${SHADOW_VERT_SPV}-t is!
add_custom_target(
        CompileShaders
        DEPENDS ${VERT_SHADER_SPV} ${FRAG_SHADER_SPV} ${SHADOW_VERT_SPV}
)

add_executable(foobar
        main.cpp
        VulkanCore/VulkanContext.cpp
        VulkanCore/VulkanContext.h
        VulkanCore/VulkanSwapchain.cpp
        VulkanCore/VulkanSwapchain.h
        VulkanCore/VulkanPipeline.cpp
        VulkanCore/VulkanPipeline.h
        VulkanCore/VulkanRenderer.cpp
        VulkanCore/VulkanRenderer.h
        VulkanCore/MeshObject.cpp
        VulkanCore/MeshObject.h
        VulkanCore/Texture.h
        VulkanCore/Texture.cpp
)

# Ez biztosítja, hogy a shaderek leforduljanak az exe előtt
add_dependencies(foobar CompileShaders)

target_link_libraries(foobar PRIVATE Vulkan::Vulkan glfw)
target_link_libraries(foobar PRIVATE Vulkan::Vulkan glm)