#pragma once

#include <Eigen/Dense>
#include <list>
#include <memory>

#include "GameObject.hpp"
#include "Camera.hpp"
#include "graphics/ShaderProgram.hpp"

class GlobalContext
{
public:

	GlobalContext();

	GlobalContext(const GlobalContext&) = delete;
	GlobalContext& operator=(const GlobalContext&) = delete;

	void update();

	void update_ui();

	void render();

	const Eigen::Vector3f& get_clear_color() const {
		return m_clear_color;
	}

	const Camera& camera() const { return m_camera; }

	float get_time() const { return m_time; }

	float delta_time() const { return m_delta_time; }

	void add_manipulation_guizmo(glm::mat4* transform) const;

private:
	Camera m_camera;
	Eigen::Vector3f m_clear_color;

	ShaderProgram m_mesh_draw_program;

	float m_time = 0.0f;
	float m_delta_time = 0.0f;


	// Windows
	bool m_show_imgui_demo_window = true;
	bool m_show_camera_window = false;
	bool m_show_inspector_window = false;
	bool m_show_simulation_window = false;

	bool m_file_picker_open = false;
	bool m_show_world_grid = false;
	bool m_show_guizmos = true;

	// guizmos interaction
	enum class GuizmosInteraction {
		eTranslate = 0,
		eRotate = 1,
		eScale = 2
	};
	GuizmosInteraction m_guizmos_mode = GuizmosInteraction::eTranslate;
	bool m_use_local_space_interaction = true;

	// Models
	std::list<std::shared_ptr<GameObject>> m_gameObjects;
	std::list<std::shared_ptr<GameObject>>::iterator m_selected_object;

	std::string m_file_picker_error;
	void handle_file_picker();

};