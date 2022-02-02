cmake_minimum_required(VERSION 3.10)


add_executable(${PROJECT_NAME}
	main.cpp
	GlobalContext.hpp	GlobalContext.cpp
	Camera.hpp			Camera.cpp
	GameObject.hpp		GameObject.cpp

	meshes/TetMesh.hpp	meshes/TetMesh.cpp

	graphics/Shader.hpp	graphics/Shader.cpp
	graphics/ShaderProgram.hpp	graphics/ShaderProgram.cpp
 )

target_include_directories(${PROJECT_NAME} PRIVATE "./")

target_link_libraries(${PROJECT_NAME} PRIVATE
	glfw glad ImGui ImGuiFileDialog ImGuizmo eigen glm ${CMAKE_DL_LIBS}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Add definition to get the compilation path
target_compile_definitions(${PROJECT_NAME} PUBLIC PROJECT_DIR=$<IF:$<BOOL:${USE_RELATIVE_PATH}>,"./","${CMAKE_BINARY_DIR}">)

# Set as default project for visual studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})