#pragma once

#include <glm/glm.hpp>

#include "GameObject.hpp"
#include "Camera.hpp"
#include "graphics/ShaderProgram.hpp"
#include "ElasticSimulator.hpp"
#include "physics/PhysicsSystem.hpp"
#include "utils/serialization.hpp"

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

	void start_simulation(Context& ctx);

	const glm::vec3& get_clear_color() const {
		return m_clear_color;
	}

	const Camera& camera() const { return m_camera; }
	Camera& camera() { return m_camera; }

	void add_gameObject(const std::shared_ptr<GameObject>& obj);
	void add_gameObject(std::shared_ptr<GameObject>&& obj);

	PhysicsSystem& physics() { return m_physic_sys; }
	const PhysicsSystem& physics() const { return m_physic_sys; }

	ElasticSimulator& elastic_sim() { return m_elastic_simulator; }
	const ElasticSimulator& elastic_sim() const { return m_elastic_simulator; }

	struct SceneTimeUpdate {
		float update;
		float simulation_update;
		float late_update;
	};
	const SceneTimeUpdate& get_scene_time_update() const { return m_scene_time_update; }

private:
	Camera m_camera;
	glm::vec3 m_clear_color;

	// Models
	std::list<std::shared_ptr<GameObject>> m_gameObjects;
	std::list<std::shared_ptr<GameObject>>::iterator m_selected_object;

	// UI
	bool m_show_objects_window = true;
	bool m_show_inspector_window = false;
	bool m_show_simulation_window = true;

	// Physics
	PhysicsSystem m_physic_sys;

	// Elastic Simulation
	ElasticSimulator m_elastic_simulator;

	
	SceneTimeUpdate m_scene_time_update;

	// Serialization
	template<typename Archive>
	void save(Archive& archive) const;

	template<typename Archive>
	void load(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS
};

