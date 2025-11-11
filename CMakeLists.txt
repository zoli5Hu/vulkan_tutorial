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

include(FetchContent)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)



set(VERT_SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert)
set(FRAG_SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag)

set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

set(VERT_SHADER_SPV ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vert.spv)
set(FRAG_SHADER_SPV ${CMAKE_CURRENT_SOURCE_DIR}/shaders/frag.spv)

add_custom_command(
        OUTPUT ${VERT_SHADER_SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        # Második parancs a tényleges fordítás
        COMMAND Vulkan::glslc ${VERT_SHADER_SRC} -o ${VERT_SHADER_SPV}
        DEPENDS ${VERT_SHADER_SRC}
        COMMENT "Compiling vertex shader"
)

add_custom_command(
        OUTPUT ${FRAG_SHADER_SPV}
        # VÁLTOZTATÁS: Első parancsként hozzuk létre a mappát, ha nem létezik.
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        # Második parancs a tényleges fordítás
        COMMAND Vulkan::glslc ${FRAG_SHADER_SRC} -o ${FRAG_SHADER_SPV}
        DEPENDS ${FRAG_SHADER_SRC}
        COMMENT "Compiling fragment shader"
)

add_custom_target(
        CompileShaders
        DEPENDS ${VERT_SHADER_SPV} ${FRAG_SHADER_SPV}
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
)
add_dependencies(foobar CompileShaders)

target_link_libraries(foobar PRIVATE Vulkan::Vulkan glfw)
target_link_libraries(foobar PRIVATE Vulkan::Vulkan glm)

