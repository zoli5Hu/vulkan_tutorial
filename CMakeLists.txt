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


# ========= SHADER FORDÍTÁS KEZDETE =========

# 1. Definiáljuk a forrás (GLSL) és a cél (SPIR-V) fájlokat.
set(VERT_SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert)
set(FRAG_SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag)

#A kimeneti mappa a build dir-en belüli "shaders" mappa
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

set(VERT_SHADER_SPV ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vert.spv)
set(FRAG_SHADER_SPV ${CMAKE_CURRENT_SOURCE_DIR}/shaders/frag.spv)

# 2. Létrehozzuk a fordítási szabályokat (parancsokat).
add_custom_command(
        OUTPUT ${VERT_SHADER_SPV}
        # VÁLTOZTATÁS: Első parancsként hozzuk létre a mappát, ha nem létezik.
        # A -E make_directory platformfüggetlen mappa létrehozás.
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

# 3. Létrehozunk egy egyéni célt (target), ami összefogja a kimeneteket.
add_custom_target(
        CompileShaders
        DEPENDS ${VERT_SHADER_SPV} ${FRAG_SHADER_SPV}
)

# ========= SHADER FORDÍTÁS VÉGE =========


# Executable
add_executable(foobar main.cpp)

# 4. Hozzáadjuk a függőséget.
add_dependencies(foobar CompileShaders)

# Include és linkelés
target_link_libraries(foobar PRIVATE Vulkan::Vulkan glfw)