#pragma once

#include <Eigen/Dense>

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
	Eigen::Vector3f m_clear_color;

	float m_time = 0.0f;
	float m_delta_time = 0.0f;


	// Windows
	bool m_show_imgui_demo_window = true;
};