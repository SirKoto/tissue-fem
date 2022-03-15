#pragma once

#include <glm/glm.hpp>

#include "GameObject.hpp"
#include "Camera.hpp"
#include "graphics/ShaderProgram.hpp"
#include "sim/IFEM.hpp"
#include "utils/CircularBuffer.hpp"

class Context;
class Scene
{
public:
	Scene();

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void update(const Context& ctx);

	void update_ui(const Context& ctx);

	void render(const Context& ctx);

	const glm::vec3& get_clear_color() const {
		return m_clear_color;
	}

	const Camera& camera() const { return m_camera; }

	void add_gameObject(std::shared_ptr<GameObject>& obj);

private:
	Camera m_camera;
	glm::vec3 m_clear_color;

	ShaderProgram m_mesh_draw_program;

	// Models
	std::list<std::shared_ptr<GameObject>> m_gameObjects;
	std::list<std::shared_ptr<GameObject>>::iterator m_selected_object;

	// Simulatior
	std::unique_ptr<sim::IFEM> m_sim;
	bool m_run_simulation = false;
	// UI
	bool m_show_inspector_window = false;
	bool m_show_simulation_window = true;
	bool m_show_simulation_metrics = true;

	// Metrics
	CircularBuffer<std::pair<float, sim::IFEM::MetricTimes>> m_metric_times_buffer;
	float m_metrics_past_seconds = 20.0f;
};
