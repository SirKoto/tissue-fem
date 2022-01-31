cmake_minimum_required(VERSION 3.10)


add_subdirectory(glad)
add_subdirectory(imgui)
add_subdirectory(ImGuiFileDialog)

add_subdirectory(glm)

add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE eigen/)


set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)