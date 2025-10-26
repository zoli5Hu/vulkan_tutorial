cmake_minimum_required(VERSION 4.0)
project(vulkan)

set(CMAKE_CXX_STANDARD 17)

# Vulkan
find_package(Vulkan REQUIRED)

# GLFW
include(FetchContent)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
)
FetchContent_MakeAvailable(glfw)


# Executable
add_executable(foobar main.cpp)

# Include és linkelés
target_link_libraries(foobar PRIVATE Vulkan::Vulkan glfw)
