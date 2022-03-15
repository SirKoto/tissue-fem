#pragma once

#include <Eigen/Dense>
#include <list>
#include <memory>
#include <functional>

#include "GameObject.hpp"
#include "Camera.hpp"
#include "graphics/ShaderProgram.hpp"
#include "sim/IFEM.hpp"
#include "utils/CircularBuffer.hpp"
#include "Scene.hpp"
#include "Engine.hpp"


class Context
{
public:

	Context(const Engine* engine);

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;

	void update();

	void update_ui();

	const glm::vec3& get_clear_color() const {
		return m_engine->m_scene->get_clear_color();
	}

	const Camera& camera() const { return m_engine->m_scene->camera(); }

	float get_time() const { return m_time; }

	float delta_time() const { return m_delta_time; }

	void add_manipulation_guizmo(glm::mat4* transform) const;

	typedef std::function<bool(const Context&, const std::filesystem::path&, std::string* err)> FilePickerCallback;
	void open_file_picker(const FilePickerCallback& callback);

private:

	const Engine* m_engine;

	float m_time = 0.0f;
	float m_delta_time = 0.0f;


	// Windows
	bool m_show_imgui_demo_window = false;
	bool m_show_implot_demo_window = false;
	bool m_show_camera_window = false;

	bool m_file_picker_open = false;
	bool m_show_world_grid = false;
	bool m_show_guizmos = true;

	bool m_run_simulation = false;

	// guizmos interaction
	enum class GuizmosInteraction {
		eTranslate = 0,
		eRotate = 1,
		eScale = 2
	};
	GuizmosInteraction m_guizmos_mode = GuizmosInteraction::eTranslate;
	bool m_use_local_space_interaction = true;


	std::string m_file_picker_error;
	FilePickerCallback m_file_picker_callback;

	void handle_file_picker();

};