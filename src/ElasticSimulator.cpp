#include "ElasticSimulator.hpp"

#include "Context.hpp"
#include "sim/SimpleFEM.hpp"
#include "sim/ParallelFEM.hpp"

ElasticSimulator::ElasticSimulator() : 
	m_params(1000.0f, 0.3f), 
	m_last_step_time_cost(1.0/60.0)
{
}

void ElasticSimulator::add_simulated_object(SimulatedGameObject* obj)
{
	uint32_t offset = 0;
	if (!m_simulated_objects.empty()) {
		offset = m_simulated_objects.back().offset +
			(uint32_t)m_simulated_objects.back().obj->get_mesh()->nodes().size();
	}
	m_simulated_objects.push_back(SimulatedEntity(offset, obj));
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

	std::vector<const TetMesh*> objs;
	uint32_t surface_verts = 0;
	objs.reserve(m_simulated_objects.size());
	for (const SimulatedEntity& e : m_simulated_objects) {
		objs.push_back(e.obj->get_mesh().get());
		surface_verts += (uint32_t)e.obj->get_mesh()->global_to_local_surface_vertices().size();
	}

	m_constrained_nodes.reserve(surface_verts / 6);


	switch (m_simulator_type) {
	case SimulatorType::SimpleFEM:
		m_sim = std::make_unique<sim::SimpleFem>();
		break;
	case SimulatorType::ParallelFEM:
		m_sim = std::make_unique<sim::ParallelFEM>();
		break;
	default:
		assert(false);
	}

	m_sim->initialize(objs);

}

void ElasticSimulator::update(const Context& ctx)
{
	assert(ctx.is_simulation_running());

	const float dt = std::min(ctx.delta_time(), 1.0f / 30.0f);

	// Dynamic update of sub-steps
	uint32_t num_substeps;
	if (m_last_step_time_cost.count() > ctx.objective_dt() && m_last_frame_iterations > 1) {
		num_substeps = m_last_frame_iterations - 1;
	}
	else {
		num_substeps = m_last_frame_iterations;

		const double avg_time = m_last_step_time_cost.count() / (double)m_last_frame_iterations;
		if ((m_last_step_time_cost.count() + avg_time) < ctx.objective_dt()) {
			num_substeps += 1;
		}
	}
	num_substeps = std::min(m_max_substeps, num_substeps);
	m_metric_substeps_buffer.push(std::make_pair(ctx.get_sim_time(), (float) num_substeps));

	m_update_meshes_time = 0.0f;
	m_remove_constraints_time = 0.0f;
	m_physics_time = 0.0f;

	// Full-step timer
	const auto init_step_timer = std::chrono::high_resolution_clock::now();

	for (uint32_t substep = 0; substep < num_substeps; ++substep) {
		for (const SimulatedEntity& e : m_simulated_objects) {
			// Add constraints for interaction
			const std::map<uint32_t, gobj::PrimitiveSelector::Delta>& movements = 
				e.obj->get_selector().get_movements();
			for (const auto& m : movements) {
				const uint32_t node_idx = m.first;
				const uint32_t sim_node_idx = e.offset + m.first;
				if (m_constrained_nodes.count(sim_node_idx) == 0) {
					// Do not add the constraint if it traverses some kinematic collider
					Ray ray;
					ray.origin = e.obj->get_mesh()->nodes_glm()[node_idx];
					const glm::vec3 next_pos = ray.origin + m.second.delta;
					ray.direction = next_pos - ray.origin;
					std::optional<SurfaceIntersection> intersection = ctx.get_scene().physics().intersect(ray, 1.0f);

					if (m.second.delta == glm::vec3(0.0f) || !intersection.has_value()) {
						/*glm::vec3 vel = m.second.delta / dt / (float)num_substeps;
						m_sim->add_constraint(sim_node_idx, vel);
						m_constrained_nodes.emplace(sim_node_idx, Constraint(vel, nullptr, true));*/

						m_sim->add_constraint(sim_node_idx, glm::vec3(0.0f));
						m_sim->add_position_alteration(sim_node_idx, m.second.delta / (float)num_substeps);
						m_constrained_nodes.emplace(sim_node_idx, Constraint(glm::vec3(0.0f), nullptr, true));
					}
				}
			}
		}

		float step_dt = dt / (float)num_substeps;

		// Solve system
		m_sim->step((sim::Float)step_dt, m_params);

		// Clear constraints after using them
		m_sim->clear_frame_alterations();

		const auto remove_constraints_timer = std::chrono::high_resolution_clock::now();
		// Remove constraints that are applying negative constraint forces or marked as to_delete
		for (std::unordered_map<uint32_t, Constraint>::const_iterator it = m_constrained_nodes.begin(); it != m_constrained_nodes.end();) {
			const uint32_t sim_node_idx = it->first;
			const glm::vec3 p = sim::cast_vec3(m_sim->get_node(sim_node_idx));
			// Move node ontop of its face if it has "moved"
			if (it->second.primitive != nullptr) {
				physics::Projection pr = it->second.primitive->plane_project(p);
				if (pr.z < 0.0f) {
					glm::vec3 delta_x = it->second.normal *
						std::nextafter(-pr.z, std::numeric_limits<float>::infinity());
					m_sim->add_position_alteration(
						sim_node_idx, delta_x);
				}
			}

			const glm::vec3 constraint_force = sim::cast_vec3(m_sim->get_force_constraint(sim_node_idx));
			const float dot = glm::dot(constraint_force, it->second.normal);
			const float distance = it->second.primitive != nullptr ? it->second.primitive->distance(p) : 0.0f;
			if (it->second.to_delete || dot < -std::numeric_limits<float>::epsilon() ||
				distance > 1e-3f) {
				m_sim->erase_constraint(sim_node_idx);
				it = m_constrained_nodes.erase(it);
			}
			else {
				++it;
			}
		}
		const auto physics_timer = std::chrono::high_resolution_clock::now();
		for (const SimulatedEntity& e : m_simulated_objects) {
			// Add constraints for surface faces with static objects
			/*for (const auto& surface_vert : e.obj->get_mesh()->global_to_local_surface_vertices()) {
				uint32_t node_idx = surface_vert.first;
			*/	
			for(uint32_t node_idx = 0; node_idx < e.obj->get_mesh()->nodes().size(); ++node_idx) {
				uint32_t sim_node_idx = e.offset + node_idx;

				if (m_constrained_nodes.count(sim_node_idx)) {
					continue;
				}

				Ray ray;
				ray.origin = e.obj->get_mesh()->nodes_glm()[node_idx];
				const glm::vec3 sim_pos = sim::cast_vec3(m_sim->get_node(sim_node_idx));
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
							sim_node_idx, delta_x);

						// Only add constraint if node's velocity goes against static surface
						glm::vec3 v = sim::cast_vec3(m_sim->get_velocity(sim_node_idx));
						const float perp_velocity = glm::dot(intersection->normal, v);
						if (perp_velocity < 0.0f) {
							m_sim->add_constraint(sim_node_idx,
								glm::vec3(0.0f),
								intersection->normal,
								m_tangential_friction);
							// Cache the constrained node
							m_constrained_nodes.emplace(sim_node_idx, Constraint(intersection->normal, intersection->primitive));
						}
					}
				}
			}
		}

		const auto update_meshes_timer = std::chrono::high_resolution_clock::now();

		for (const SimulatedEntity& e : m_simulated_objects) {
			m_sim->update_objects(e.obj->get_mesh().get(),
				e.offset, e.offset + (uint32_t)e.obj->get_mesh()->nodes().size(),
				true);
		}
		const auto end_sub_step_timer = std::chrono::high_resolution_clock::now();

		m_update_meshes_time = std::chrono::duration<float>(end_sub_step_timer - update_meshes_timer).count();
		m_remove_constraints_time = std::chrono::duration<float>(physics_timer - remove_constraints_timer).count();
		m_physics_time = std::chrono::duration<float>(update_meshes_timer - physics_timer).count();

		if (!m_sim->simulation_converged()) {
			ctx.trigger_pause_simulation();
			break;
		}
	}

	const auto end_step_timer = std::chrono::high_resolution_clock::now();

	m_last_step_time_cost = end_step_timer - init_step_timer;
	m_last_frame_iterations = num_substeps;

	// Update metrics
	m_metric_times_buffer.push({ ctx.get_sim_time(),
					{m_sim->get_metric_times(),
					(float)m_sim->compute_volume(),
					(float)m_last_step_time_cost.count() / m_last_frame_iterations,
					m_update_meshes_time / m_last_frame_iterations,
					m_remove_constraints_time / m_last_frame_iterations,
					m_physics_time / m_last_frame_iterations
					} });
}

void ElasticSimulator::render_ui(const Context& ctx)
{
	m_params.draw_ui();

	ImGui::BeginDisabled(ctx.has_simulation_started());
	ImGui::Combo("Simulator type", reinterpret_cast<int*>(&m_simulator_type),
		"SimpleFEM\0ParallelFEM");
	ImGui::EndDisabled();

	ImGui::Checkbox("Simulation Metrics", &m_show_simulation_metrics);

	ImGui::InputFloat("Friction", &m_tangential_friction, 0.01f);

	uint32_t step = 1;
	ImGui::InputScalar("Max substeps", ImGuiDataType_U32, &m_max_substeps, &step);
	m_max_substeps = std::max(m_max_substeps, 1u);
	

	ImGui::Text("Iterations in step: %u", m_last_frame_iterations);

	if (m_show_simulation_metrics) {
		ImGui::SetNextWindowSize(ImVec2(450, 380), ImGuiCond_FirstUseEver);
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Simulation Metrics", &m_show_simulation_metrics)) {

			ImGui::SliderFloat("Past seconds", &m_metrics_past_seconds, 0.1f, 30.0f, "%.1f");


			if (ImPlot::BeginPlot("Solver##SolverMetrics", ImVec2(-1, 160))) {
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
				ImPlot::PlotLine("System build",
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
				ImPlot::PlotLine("Constraints build",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.times.constraints,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::EndPlot();
			}

			if (ImPlot::BeginPlot("General Sim##General", ImVec2(-1, 160))) {
				ImPlot::SetupAxes("time (s)", "dt (s)");
				float x = m_metric_times_buffer.size() > 0 ? m_metric_times_buffer.back().first : 0.0f;
				ImPlot::SetupAxisLimits(ImAxis_X1, x - m_metrics_past_seconds, x, ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 0.0055);

				ImPlot::PlotLine("Total",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.step_time,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("FEM time",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.times.step,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("Update Meshes",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.update_meshes,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("Remove constraints",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.remove_constraints,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("Physics",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.physics,
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

			if (ImPlot::BeginPlot("Substeps", ImVec2(-1, 160))) {
				ImPlot::SetupAxes("time (s)", "substeps");
				float x = m_metric_times_buffer.size() > 0 ? m_metric_times_buffer.back().first : 0.0f;
				ImPlot::SetupAxisLimits(ImAxis_X1, x - m_metrics_past_seconds, x, ImGuiCond_Always);

				ImPlot::PlotLine("##substeps",
					&m_metric_substeps_buffer.data()->first,
					&m_metric_substeps_buffer.data()->second,
					(int)m_metric_substeps_buffer.size(),
					(int)m_metric_substeps_buffer.offset(),
					sizeof(*m_metric_substeps_buffer.data()));
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
	archive(TF_SERIALIZE_NVP_MEMBER(m_show_simulation_metrics));
	archive(TF_SERIALIZE_NVP_MEMBER(m_simulator_type));
	archive(TF_SERIALIZE_NVP_MEMBER(m_max_substeps));
	archive(TF_SERIALIZE_NVP_MEMBER(m_tangential_friction));
}

TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(ElasticSimulator)