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

	std::vector<SimulatedGameObject*> m_simulated_objects;

	uint32_t m_last_frame_iterations = 1;
	std::chrono::duration<double> m_last_step_time_cost;

	bool m_show_simulation_metrics = false;

	// Metrics
	struct Metrics {
		sim::IFEM::MetricTimes times;
		float volume;
	};
	CircularBuffer<std::pair<float, Metrics>> m_metric_times_buffer;
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

	// TODO: remove
	// Serialization
	template<typename Archive>
	void serialize(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS
};