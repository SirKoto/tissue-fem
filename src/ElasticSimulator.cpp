#include "ElasticSimulator.hpp"

#include "Context.hpp"
#include "sim/SimpleFEM.hpp"

ElasticSimulator::ElasticSimulator() : m_params(1000.0f, 0.3f)
{
	m_sim = std::make_unique<sim::SimpleFem>();
}

void ElasticSimulator::add_simulated_object(SimulatedGameObject* obj)
{
	m_simulated_objects.push_back(obj);
}

void ElasticSimulator::reset()
{
	m_constrained_nodes.clear();
	m_simulated_objects.clear();
	m_last_frame_iterations = 1;
}

void ElasticSimulator::start_simulation(const Context& ctx)
{
	m_metric_times_buffer.clear();

	for (const SimulatedGameObject* obj : m_simulated_objects) {
		m_sim->set_tetmesh(obj->get_mesh());
	}
}

void ElasticSimulator::update(const Context& ctx)
{
	assert(ctx.is_simulation_running());

	const float dt = std::min(ctx.delta_time(), 1.0f / 30.0f);

	// Dynamic update of sub-steps
	uint32_t max_repetitions;
	if (ctx.delta_time() > ctx.objective_dt() && m_last_frame_iterations > 1) {
		max_repetitions = m_last_frame_iterations - 1;
	}
	else {
		max_repetitions = m_last_frame_iterations;

		const double avg_time = m_last_step_time_cost.count() / m_last_frame_iterations;

		if (m_last_step_time_cost.count() + avg_time < ctx.objective_dt()) {
			max_repetitions += 1;
		}
	}

	// Full-step timer
	const auto init_step_timer = std::chrono::high_resolution_clock::now();

	for (uint32_t repetitions = 0; repetitions < max_repetitions; ++repetitions) {
		for (const SimulatedGameObject* obj : m_simulated_objects) {
			// Add constraints for interaction
			const std::map<uint32_t, gobj::PrimitiveSelector::Delta>& movements = obj->get_selector().get_movements();
			for (const auto& m : movements) {
				const uint32_t node_idx = m.first;
				if (m_constrained_nodes.count(node_idx) == 0) {
					// Do not add the constraint if it traverses some kinematic collider
					Ray ray;
					ray.origin = obj->get_mesh()->nodes_glm()[node_idx];
					const glm::vec3 next_pos = ray.origin + m.second.delta;
					ray.direction = next_pos - ray.origin;
					std::optional<SurfaceIntersection> intersection = ctx.get_scene().physics().intersect(ray, 1.0f);

					if (m.second.delta == glm::vec3(0.0f) || !intersection.has_value()) {
						m_sim->add_constraint(node_idx, glm::vec3(0.0f));
						m_sim->add_position_alteration(node_idx, m.second.delta / (float)max_repetitions);
						m_constrained_nodes.emplace(node_idx, Constraint(glm::vec3(0.0f), nullptr, true));
					}
				}
			}
		}

		// Solve system
		m_sim->step((sim::Float)dt / (sim::Float)max_repetitions, m_params);

		// Clear constraints after using them
		m_sim->clear_frame_alterations();

		// Remove constraints that are applying negative constraint forces or marked as to_delete
		for (std::map<uint32_t, Constraint>::const_iterator it = m_constrained_nodes.begin(); it != m_constrained_nodes.end();) {
			const uint32_t node_idx = it->first;
			const glm::vec3 constraint_force = sim::cast_vec3(m_sim->get_force_constraint(node_idx));
			const float dot = glm::dot(constraint_force, it->second.normal);
			const float distance = it->second.primitive != nullptr ? it->second.primitive->distance(sim::cast_vec3(m_sim->get_node(node_idx))) : 0.0f;
			if (it->second.to_delete || dot < -std::numeric_limits<float>::epsilon() ||
				distance > 1e-3f) {
				m_sim->erase_constraint(node_idx);
				it = m_constrained_nodes.erase(it);
			}
			else {
				++it;
			}
		}

		for (const SimulatedGameObject* obj : m_simulated_objects) {
			// Add constraints for surface faces with static objects
			for (const auto& surface_vert : obj->get_mesh()->global_to_local_surface_vertices()) {
				uint32_t node_idx = surface_vert.first;
				if (m_constrained_nodes.count(node_idx)) {
					continue;
				}

				Ray ray;
				ray.origin = obj->get_mesh()->nodes_glm()[node_idx];
				const glm::vec3 sim_pos = sim::cast_vec3(m_sim->get_node(node_idx));
				ray.direction = sim_pos - ray.origin;

				if (glm::dot(ray.direction, ray.direction) >= 2.0f * std::numeric_limits<float>::epsilon()) {
					std::optional<SurfaceIntersection> intersection = ctx.get_scene().physics().intersect(ray, 1.0f);

					if (intersection.has_value()) {

						float dist_to_intersection = glm::dot(intersection->normal, intersection->point - sim_pos);
						if (dist_to_intersection < 0.0f) {
							continue;
						}

						glm::vec3 delta_x = intersection->normal *
							std::nextafter(dist_to_intersection, std::numeric_limits<float>::infinity());
						m_sim->add_position_alteration(
							node_idx, delta_x);

						// Only add constraint if node's velocity goes against static surface
						glm::vec3 v = sim::cast_vec3(m_sim->get_velocity(node_idx));
						const float perp_velocity = glm::dot(intersection->normal, v);
						if (perp_velocity < 0.0f) {
							m_sim->add_constraint(node_idx,
								glm::vec3(0.0f),
								intersection->normal);
							// Cache the constrained node
							m_constrained_nodes.emplace(node_idx, Constraint(intersection->normal, intersection->primitive));
						}
					}
				}
			}
		}

		m_sim->update_objects(true);
	}

	const auto end_step_timer = std::chrono::high_resolution_clock::now();

	m_last_step_time_cost = end_step_timer - init_step_timer;
	m_last_frame_iterations = max_repetitions;

}

void ElasticSimulator::render_ui(const Context& ctx)
{
	m_params.draw_ui();

	ImGui::Checkbox("Simulation Metrics", &m_show_simulation_metrics);

	ImGui::Text("Iterations in step: %i", m_last_frame_iterations);

	if (m_show_simulation_metrics) {
		ImGui::SetNextWindowSize(ImVec2(450, 380), ImGuiCond_FirstUseEver);
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Simulation Metrics", &m_show_simulation_metrics)) {
			if (m_sim && ctx.is_simulation_running()) {
				m_metric_times_buffer.push({ ctx.get_time(),
					{m_sim->get_metric_times(), (float)m_sim->compute_volume()} });
			}

			ImGui::SliderFloat("Past seconds", &m_metrics_past_seconds, 0.1f, 30.0f, "%.1f");


			if (ImPlot::BeginPlot("##SolverMetrics", ImVec2(-1, 160))) {
				ImPlot::SetupAxes("time (s)", "dt (s)");
				float x = m_metric_times_buffer.size() > 0 ? m_metric_times_buffer.back().first : 0.0f;
				ImPlot::SetupAxisLimits(ImAxis_X1, x - m_metrics_past_seconds, x, ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 0.0055);
				ImPlot::PlotLine("Step time",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.times.step,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("Set Zero System",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.times.set_zero,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("Blocks Assign",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.times.blocks_assign,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("System finish",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.times.system_finish,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("Solve",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.times.solve,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::EndPlot();
			}

			if (ImPlot::BeginPlot("Volume##Volume", ImVec2(-1, 160))) {
				ImPlot::SetupAxes("time (s)", "vol (m^3)");
				float x = m_metric_times_buffer.size() > 0 ? m_metric_times_buffer.back().first : 0.0f;
				ImPlot::SetupAxisLimits(ImAxis_X1, x - m_metrics_past_seconds, x, ImGuiCond_Always);

				ImPlot::PlotLine("##volume",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.volume,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::EndPlot();
			}
		}
		ImGui::End();
	}
}


template<class Archive>
void ElasticSimulator::serialize(Archive& archive)
{
	archive(TF_SERIALIZE_NVP_MEMBER(m_params));
}

TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(ElasticSimulator)