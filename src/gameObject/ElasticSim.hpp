#pragma once

#include <memory>
#include <map>

#include "sim/IFEM.hpp"
#include "utils/CircularBuffer.hpp"
#include "physics/RayIntersection.hpp"
#include "extra/PrimitiveSelector.hpp"
#include "utils/serialization.hpp"

class SimulatedGameObject;
class Context;
namespace gobj {

class ElasticSim final{
public:

	ElasticSim();

	void render_ui(const Context& ctx, SimulatedGameObject* parent);
	void update(const Context& ctx, SimulatedGameObject* parent);
	void late_update(const Context& ctx, SimulatedGameObject* parent);
	void render(const Context& ctx, const SimulatedGameObject& parent) const;
	void start_simulation(const Context& ctx, const SimulatedGameObject& parent);

private:
	sim::Parameters m_params;
	std::unique_ptr<sim::IFEM> m_sim;
	struct Constraint;
	std::map<uint32_t, Constraint> m_constrained_nodes;
	gobj::PrimitiveSelector m_selector;

	bool m_show_simulation_metrics = false;

	// Metrics
	CircularBuffer<std::pair<float, sim::IFEM::MetricTimes>> m_metric_times_buffer;
	float m_metrics_past_seconds = 20.0f;

	typedef physics::Primitive Primitive;
	// Constraint
	struct Constraint {
		Constraint(const glm::vec3& n, const Primitive* primitive) : normal(n), primitive(primitive){}
		Constraint(const glm::vec3& n, const Primitive* primitive, bool delete_next) : normal(n), primitive(primitive), to_delete(delete_next) {}
		glm::vec3 normal;
		const Primitive* primitive;
		bool to_delete = false;
	};

	// Serialization
	template<typename Archive>
	void serialize(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS
};

} // namespace gobj