cmake_minimum_required(VERSION 3.10)


add_executable(${PROJECT_NAME}
	main.cpp
	Context.hpp	Context.cpp
	Camera.hpp			Camera.cpp
	GameObject.hpp		GameObject.cpp
	Scene.hpp			Scene.cpp
	Engine.hpp			Engine.cpp

	meshes/TetMesh.hpp	meshes/TetMesh.cpp
	meshes/Plane.hpp	meshes/Plane.cpp
	meshes/TriangleMesh.hpp	meshes/TriangleMesh.cpp

	sim/IFEM.hpp		sim/IFEM.cpp
	sim/SimpleFEM.hpp	sim/SimpleFEM.cpp

	sim/solvers/ConjugateGradient.hpp	sim/solvers/ConjugateGradient.cpp

	physics/PhysicsSystem.hpp	physics/PhysicsSystem.cpp
	physics/RayIntersection.hpp	physics/RayIntersection.cpp
	physics/Primitive.hpp

	graphics/Shader.hpp	graphics/Shader.cpp
	graphics/ParticleManager.hpp	graphics/ParticleManager.cpp
	graphics/ShaderProgram.hpp	graphics/ShaderProgram.cpp

	utils/Timer.hpp	utils/Timer.cpp
	utils/Icosphere.hpp	utils/Icosphere.cpp
	utils/BBox.hpp	utils/BBox.cpp
	utils/CircularBuffer.hpp

	gameObject/Transform.hpp gameObject/Transform.cpp
	gameObject/ElasticSim.hpp gameObject/ElasticSim.cpp
	gameObject/extra/PrimitiveSelector.hpp gameObject/extra/PrimitiveSelector.cpp
	gameObject/Addon.hpp
 )

target_include_directories(${PROJECT_NAME} PRIVATE "./")

target_link_libraries(${PROJECT_NAME} PRIVATE
	cereal glfw glad ImGui ImGuiFileDialog ImGuizmo implot eigen glm ${CMAKE_DL_LIBS}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Add definition to get the compilation path
target_compile_definitions(${PROJECT_NAME} PUBLIC PROJECT_DIR=$<IF:$<BOOL:${USE_RELATIVE_PATH}>,"./","${CMAKE_BINARY_DIR}">)

# Set as default project for visual studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})