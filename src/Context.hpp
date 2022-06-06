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

	Context(Engine* engine);

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;

	void update();

	void draw_ui();

	const glm::vec3& get_clear_color() const {
		return m_engine->m_scene->get_clear_color();
	}

	const Camera& camera() const { return m_engine->m_scene->camera(); }

	float get_time() const { return m_time; }
	float get_sim_time() const { return m_time - m_simulation_start_time; }

	float delta_time() const { return m_use_dynamic_dt ? m_dynamic_delta_time : m_static_dt; }

	float objective_dt() const { return m_objective_dt; }
	float objective_fps() const { return m_objective_fps; }

	const Scene& get_scene() const { return *m_engine->m_scene; }
	const ElasticSimulator& get_simulator() const { return m_engine->m_scene->elastic_sim(); }
	ElasticSimulator& get_simulator() { return m_engine->m_scene->elastic_sim(); }

	bool has_simulation_started() const { return m_engine->m_simulation_mode; }

	bool is_simulation_running() const { return m_engine->m_simulation_mode && m_engine->m_run_simulation; }

	void trigger_pause_simulation() const {
		if (has_simulation_started()) {
			m_engine->m_run_simulation = false;
		}
	}

	typedef std::function<bool(const Context&, 
		Scene& scene,
		const std::filesystem::path&, 
		std::string* err)> FilePickerCallback;
	void open_file_picker(
		const char* window_title,
		const char* filter,
		const FilePickerCallback& callback);

	// guizmos interaction
	enum class GuizmosInteraction {
		eTranslate = 0,
		eRotate = 1,
		eScale = 2
	};
	bool add_manipulation_guizmo(glm::mat4* transform, int32_t id) const;
	bool add_manipulation_guizmo(glm::mat4* transform, glm::mat4* delta_transform, int32_t id) const;

	bool add_manipulation_guizmo(glm::mat4* transform, Context::GuizmosInteraction op, glm::mat4* delta_transform, int32_t id) const;


	PhysicsSystem::PhysicEntityId insert_static_physics_primitive(const PhysicsSystem::PrimitivePtr& primitive) {
		return m_engine->m_scene->physics().insert_primitive(primitive);
	}
	PhysicsSystem::PhysicEntityId insert_static_physics_primitive(PhysicsSystem::PrimitivePtr&& primitive) {
		return m_engine->m_scene->physics().insert_primitive(primitive);
	}

	void write_file_csv(const std::function<void(std::ostream&)>& callback) const;

private:

	Engine* m_engine;

	float m_time = 0.0f;
	float m_simulation_start_time = 0.0f;
	float m_dynamic_delta_time = 0.0f;
	std::array<float, 4> m_delta_times_buffer;

	float m_objective_fps = 59.0f;
	float m_objective_dt = 1.0f / 59.0f;
	float m_static_dt = 1.0f / 60.0f;
	bool m_use_dynamic_dt = true;

	// Windows
	bool m_show_imgui_demo_window = false;
	bool m_show_implot_demo_window = false;
	bool m_show_camera_window = false;
	bool m_show_profiling_window = false;

	mutable bool m_file_picker_open = false;
	bool m_show_world_grid = false;
	bool m_show_guizmos = true;

	bool m_run_simulation = false;

	
	GuizmosInteraction m_guizmos_mode = GuizmosInteraction::eTranslate;
	bool m_use_local_space_interaction = true;


	std::string m_file_picker_error;
	FilePickerCallback m_file_picker_callback;

	
	CircularBuffer<Engine::EngineTimings> m_engine_timings;
	float m_metrics_past_seconds = 30.0f;

	mutable std::function<void(std::ostream&)> m_file_callback_write;

	void handle_file_picker();

	void ui_draw_profiling();

	friend class Engine;
};