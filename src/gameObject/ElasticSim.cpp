#include "ElasticSim.hpp"

#include <imgui.h>
#include "Context.hpp"
#include "sim/SimpleFEM.hpp"

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

	ImGui::Checkbox("Keep running", &m_run_simulation);
	if (ImGui::Button("Step simulator")) {
		m_step_once = true;
	}
	if (ImGui::Button("Pancake")) {
		reinterpret_cast<sim::SimpleFem*>(m_sim.get())->pancake();
	}

	ImGui::Checkbox("Simulation Metrics", &m_show_simulation_metrics);

	if (m_show_simulation_metrics) {
		ImGui::SetNextWindowSize(ImVec2(450, 280), ImGuiCond_FirstUseEver);
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Simulation Metrics", &m_show_simulation_metrics)) {
			if (m_sim && m_run_simulation) {
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


	if(m_step_once || m_run_simulation) {
		float dt = std::min(ctx.delta_time(), 1.0f / 60.0f);
		const std::map<uint32_t, PrimitiveSelector::Delta>& movements = m_selector.get_movements();
		for (const auto& m : movements) {
			m_sim->add_constraint(m.first, glm::vec3(0.0f));
			m_sim->add_position_alteration(m.first, m.second.delta);
		}
		
		// Solve system
		m_sim->step((sim::Float)dt);

		// Clear constraints after using them
		m_sim->clear_constraints();

		// Add constraints for surface faces with static objects
		for (const auto& surface_vert : parent->get_mesh()->global_to_local_surface_vertices()) {
			uint32_t node_idx = surface_vert.first;
			Ray ray;
			ray.origin = parent->get_mesh()->nodes_glm()[node_idx];
			const glm::vec3 sim_pos = sim::cast_vec3(m_sim->get_node(node_idx));
			ray.direction = sim_pos - ray.origin;
			/*if (glm::dot(ray.direction, ray.direction) > 1e-8) {
				std::optional<SurfaceIntersection> intersection = ctx.get_scene().physics().intersect(ray, 1.0f);

				if (intersection.has_value()) {
					m_sim->add_constraint(node_idx, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					m_sim->add_position_alteration(node_idx, 1.01f * (intersection->point - sim_pos));
				}
			}*/
			if (sim_pos.y < 0.0f) {
				glm::vec3 v(0.0f, -m_sim->get_velocity(node_idx).y(), 0.0f);
				v *= 0.1f;
				float dp = -dt * v.y - sim_pos.y;
				m_sim->add_constraint(node_idx, v, glm::vec3(0.0f, 1.0f, 0.0f));
				m_sim->add_position_alteration(node_idx, glm::vec3(0.0f, dp, 0.0f));
			}
		}

		m_sim->update_objects();

		m_step_once = false;
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

} // namespace gobj