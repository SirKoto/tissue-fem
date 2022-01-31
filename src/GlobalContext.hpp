#pragma once

#include <Eigen/Dense>
#include <list>

#include "meshes/TetMesh.hpp"
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

	float get_time() const { return m_time; }

	float delta_time() const { return m_delta_time; }

private:
	Camera m_camera;
	Eigen::Vector3f m_clear_color;

	ShaderProgram m_mesh_draw_program;

	float m_time = 0.0f;
	float m_delta_time = 0.0f;


	// Windows
	bool m_show_imgui_demo_window = true;
	bool m_show_camera_window = false;
	bool m_file_picker_open = false;

	// Models
	std::list<TetMesh> m_meshes;

	std::string m_file_picker_error;
	void handle_file_picker();

};