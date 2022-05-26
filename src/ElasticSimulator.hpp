#pragma once

#include <memory>
#include <map>
#include <chrono>

#include "sim/IFEM.hpp"
#include "utils/CircularBuffer.hpp"
#include "physics/RayIntersection.hpp"
#include "gameObject/SimulatedGameObject.hpp"

class Context;
class ElasticSimulator {
public:

	ElasticSimulator();

	void add_simulated_object(SimulatedGameObject* obj);

	void reset();

	void start_simulation(const Context& ctx);

	void update(const Context& ctx);

	void render_ui(const Context& ctx);

private:
	sim::Parameters m_params;
	std::unique_ptr<sim::IFEM> m_sim;
	struct Constraint;
	std::map<uint32_t, Constraint> m_constrained_nodes;

	struct SimulatedEntity;
	std::vector<SimulatedEntity> m_simulated_objects;

	uint32_t m_last_frame_iterations = 1;
	std::chrono::duration<double> m_last_step_time_cost;
	float m_update_meshes_time = 0.0f;
	float m_remove_constraints_time = 0.0f;
	float m_physics_time = 0.0f;

	enum class SimulatorType {
		SimpleFEM = 0,
		ParallelFEM = 1
	};

	SimulatorType m_simulator_type = SimulatorType::ParallelFEM;
	uint32_t m_max_substeps = 6;
	float m_tangential_friction = .1f;
	bool m_show_simulation_metrics = false;

	// Metrics
	struct Metrics {
		sim::IFEM::MetricTimes times;
		float volume;
		float step_time;
		float update_meshes;
		float remove_constraints;
		float physics;
	};
	CircularBuffer<std::pair<float, Metrics>> m_metric_times_buffer;
	CircularBuffer<std::pair<float, float>> m_metric_substeps_buffer;

	float m_metrics_past_seconds = 20.0f;

	typedef physics::Primitive Primitive;
	// Constraint
	struct Constraint {
		Constraint(const glm::vec3& n, const Primitive* primitive) : normal(n), primitive(primitive) {}
		Constraint(const glm::vec3& n, const Primitive* primitive, bool delete_next) : normal(n), primitive(primitive), to_delete(delete_next) {}
		glm::vec3 normal;
		const Primitive* primitive;
		bool to_delete = false;
	};

	struct SimulatedEntity {
		SimulatedEntity(uint32_t offset, SimulatedGameObject* obj) : offset(offset), obj(obj) {}
		uint32_t offset;
		SimulatedGameObject* obj;
	};

	// TODO: remove
	// Serialization
	template<typename Archive>
	void serialize(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS
};