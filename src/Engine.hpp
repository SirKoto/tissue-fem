#pragma once

#include <iostream>
#include <memory>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>
#include <implot.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Scene.hpp"

class Context;
class Engine
{
public:
	Engine();
	~Engine();

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void run();

private:
	bool m_error = false;

	GLFWwindow* m_window;

	std::unique_ptr<Scene> m_scene;

	friend class Context;
};

