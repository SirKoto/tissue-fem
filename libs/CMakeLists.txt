cmake_minimum_required(VERSION 3.10)


add_subdirectory(glad)
add_subdirectory(imgui)
add_subdirectory(ImGuizmo)
add_subdirectory(ImGuiFileDialog)

add_subdirectory(glm)

add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE eigen/)

add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE cereal/)

add_library(implot
	implot/implot.cpp implot/implot.h
	implot/implot_internal.h
	implot/implot_items.cpp
	implot/implot_demo.cpp
)
target_include_directories(implot PUBLIC implot/)
target_link_libraries(implot ImGui)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)