#pragma once

#include <iostream>
#include <memory>
#include <filesystem>

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

	bool m_simulation_mode = false;
	bool m_run_simulation = true;
	bool m_first_simulation_frame = false;

	uint32_t m_num_threads;

	GLFWwindow* m_window;

	std::unique_ptr<Scene> m_scene;
	std::filesystem::path m_scene_path;
	std::filesystem::path m_scene_path_tmp;

	std::unique_ptr<Context> m_ctx;

	double m_min_delta_time = 1 / 60.0f;

	bool reload_scene(const std::filesystem::path& path, std::string* error);
	bool save_scene(const std::filesystem::path& path, std::string* error);

	void signal_start_simulation();
	void stop_simulation();

	void set_num_threads(uint32_t num);

	struct EngineTimings {
		float time;
		float window_poll;
		float imgui_new_frame;
		float context_update;
		float scene_draw_ui;
		float scene_update;
		float imgui_render_cpu;
		float imgui_render_gpu;
		float scene_render;
		Scene::SceneTimeUpdate scene_times;
	};

	friend class Context;
};

