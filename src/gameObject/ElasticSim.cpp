#include "ElasticSim.hpp"

#include <imgui.h>
#include "Context.hpp"
#include "sim/SimpleFEM.hpp"

#undef NDEBUG
#include <assert.h>

namespace gobj {

ElasticSim::ElasticSim()
{

}

void ElasticSim::render_ui(const Context& ctx, GameObject* parent)
{
	if (!m_sim) {
		return;
	}

	ImGui::PushID(this);
	m_sim->draw_ui();


	if (ImGui::Button("Pancake")) {
		reinterpret_cast<sim::SimpleFem*>(m_sim.get())->pancake();
	}

	ImGui::Checkbox("Simulation Metrics", &m_show_simulation_metrics);

	if (m_show_simulation_metrics) {
		ImGui::SetNextWindowSize(ImVec2(450, 280), ImGuiCond_FirstUseEver);
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Simulation Metrics", &m_show_simulation_metrics)) {
			if (m_sim && ctx.is_simulation_running()) {
				m_metric_times_buffer.push({ ctx.get_time(), m_sim->get_metric_times() });
			}

			ImGui::SliderFloat("Past seconds", &m_metrics_past_seconds, 0.1f, 30.0f, "%.1f");

			if (ImPlot::BeginPlot("##SolverMetrics", ImVec2(-1, -1))) {
				ImPlot::SetupAxes("time (s)", "dt (s)");
				float x = m_metric_times_buffer.size() > 0 ? m_metric_times_buffer.back().first : 0.0f;
				ImPlot::SetupAxisLimits(ImAxis_X1, x - m_metrics_past_seconds, x, ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 0.0055);
				ImPlot::PlotLine("Step time",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.step,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("Set Zero System",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.set_zero,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("Blocks Assign",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.blocks_assign,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("System finish",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.system_finish,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::PlotLine("Solve",
					&m_metric_times_buffer.data()->first,
					&m_metric_times_buffer.data()->second.solve,
					(int)m_metric_times_buffer.size(),
					(int)m_metric_times_buffer.offset(),
					sizeof(*m_metric_times_buffer.data()));
				ImPlot::EndPlot();
			}
		}
		ImGui::End();
	}

	if (ImGui::TreeNode("Primitive Selector")) {
		m_selector.render_ui(ctx, parent);
		ImGui::Separator();
		ImGui::TreePop();
	}
	

	ImGui::PopID();
}

void ElasticSim::update(const Context& ctx, GameObject* parent)
{
	if (!m_sim) {
		m_sim = std::make_unique<sim::SimpleFem>(parent->get_mesh(), 100000.0f, 0.2f);
	}
	m_selector.update(ctx, parent);


	if(ctx.is_simulation_running()) {
		float dt = std::min(ctx.delta_time(), 1.0f / 60.0f);

		const std::map<uint32_t, PrimitiveSelector::Delta>& movements = m_selector.get_movements();
		for (const auto& m : movements) {
			const uint32_t node_idx = m.first;
			if (m_constrained_nodes.count(node_idx) == 0) {
				// Do not add the constraint if it traverses some kinematic collider
				Ray ray;
				ray.origin = parent->get_mesh()->nodes_glm()[node_idx];
				const glm::vec3 next_pos = ray.origin + m.second.delta;
				ray.direction = next_pos - ray.origin;
				std::optional<SurfaceIntersection> intersection = ctx.get_scene().physics().intersect(ray, 1.0f);

				if (m.second.delta == glm::vec3(0.0f) || !intersection.has_value()) {
					m_sim->add_constraint(node_idx, glm::vec3(0.0f));
					m_sim->add_position_alteration(node_idx, m.second.delta);
					m_constrained_nodes.emplace(node_idx, Constraint(glm::vec3(0.0f), true));
				}
			}
		}
		
		// Solve system
		m_sim->step((sim::Float)dt);

		// Clear constraints after using them
		m_sim->clear_frame_alterations();

		// Remove constraints that are applying negatve constraint forces or marked as to_delete
		for (std::map<uint32_t, Constraint>::const_iterator it = m_constrained_nodes.begin(); it != m_constrained_nodes.end();) {
			const uint32_t node_idx = it->first;
			glm::vec3 constraint_force = sim::cast_vec3(m_sim->get_force_constraint(node_idx));
			const float dot = glm::dot(constraint_force, it->second.normal);
			if (it->second.to_delete || dot < -std::numeric_limits<float>::epsilon()) {
				m_sim->erase_constraint(node_idx);
				it = m_constrained_nodes.erase(it);
			}
			else {
				++it;
			}
		}

		// Add constraints for surface faces with static objects
		for (const auto& surface_vert : parent->get_mesh()->global_to_local_surface_vertices()) {
			uint32_t node_idx = surface_vert.first;
			if (m_constrained_nodes.count(node_idx)) {
				continue;
			}

			Ray ray;
			ray.origin = parent->get_mesh()->nodes_glm()[node_idx];
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
						m_constrained_nodes.emplace(node_idx, intersection->normal);
					}
				}
			}
		}

		m_sim->update_objects(true);
	}
}

void ElasticSim::late_update(const Context& ctx, GameObject* parent)
{
	m_selector.late_update(ctx, parent);
}

void ElasticSim::render(const Context& ctx, const GameObject& parent) const
{
	m_selector.render(ctx, parent);
}

const char* ElasticSim::get_name() const
{
	return "Elastic Simulator"; 
}

template<class Archive>
void ElasticSim::serialize(Archive& archive)
{
	archive(TF_SERIALIZE_NVP_MEMBER(m_selector));
}

TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(ElasticSim)

} // namespace gobj