#pragma once

#include <memory>

#include "Addon.hpp"
#include "sim/IFEM.hpp"
#include "utils/CircularBuffer.hpp"

namespace gobj {

class ElasticSim : public Addon {
public:

	ElasticSim();

	void render_ui(const Context& ctx, GameObject* parent) override final;
	void update(const Context& ctx, GameObject* parent) override final;
	const char* get_name() const { return "Elastic Simulator"; };

private:
	std::unique_ptr<sim::IFEM> m_sim;

	bool m_step_once = false;
	bool m_run_simulation = false;
	bool m_show_simulation_metrics = false;

	// Metrics
	CircularBuffer<std::pair<float, sim::IFEM::MetricTimes>> m_metric_times_buffer;
	float m_metrics_past_seconds = 20.0f;
};

} // namespace gobj