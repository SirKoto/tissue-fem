# Fast Tissue FEM Simulation

Master's Thesis - 2022

Simulation of deformable tetrahedron models and the Finite Elements Method

Supports collisions, friction, and user interaction.

Implemented different energy models:
- Bonet and Wood 2008 Neo-Hookean
- Smith 2019 Stable Neo-Hookean
- Corotational

## Build
Make sure you have downloaded all the submodules and dependencies.

If not, run:
```bash
git submodule update --init --recursive
```

This project can easily be built with CMake
```bash
mkdir build
cd build
cmake ..
```

## Dependencies
All dependencies are in the libs folder. They include:
- Cereal
- Eigen
- GLAD
- GLFW
- glm
- Dear ImGui
- ImGuiFileDialog
- ImGuizmo
- implot