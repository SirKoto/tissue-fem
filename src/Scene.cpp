#include "Scene.hpp"

#include "Context.hpp"
#include "sim/SimpleFEM.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <implot.h>

Scene::Scene() :
	m_clear_color(0.45f, 0.55f, 0.60f)
{
	const std::filesystem::path proj_dir(PROJECT_DIR);
	const std::filesystem::path shad_dir = proj_dir / "resources/shaders";

	/*std::array<Shader, 2> mesh_shaders = {
		Shader((shad_dir / "mesh.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "mesh.frag"), Shader::Type::Fragment)
	};*/
	std::array<Shader, 3> mesh_shaders = {
		Shader((shad_dir / "simple_mesh.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "generate_face_normal.geom"), Shader::Type::Geometry),
		Shader((shad_dir / "mesh.frag"), Shader::Type::Fragment)
	};
	m_mesh_draw_program = ShaderProgram(mesh_shaders.data(), (uint32_t)mesh_shaders.size());


	m_selected_object = m_gameObjects.end();
}

void Scene::update(const Context& ctx)
{
	m_camera.update();
}

void Scene::update_ui(const Context& ctx)
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("GameObjects"))
		{
			for (auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ++it) {
				if (ImGui::MenuItem((*it)->get_name().c_str())) {
					m_selected_object = it;
					m_show_inspector_window = true;
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::Checkbox("Inspector Window", &m_show_inspector_window);
			ImGui::Checkbox("Simulation Window", &m_show_simulation_window);
			ImGui::Checkbox("Simulation Metrics", &m_show_simulation_metrics);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (m_show_inspector_window) {
		char buff[128];
		std::snprintf(buff, 128, "Inspector -- %s###InspectorWin", (m_selected_object != m_gameObjects.end()) ? (*m_selected_object)->get_name().c_str() : "");
		ImGui::SetNextWindowSize(ImVec2(350, 280), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(buff, &m_show_inspector_window)) {
			if (m_selected_object != m_gameObjects.end()) {
				(*m_selected_object)->render_ui(ctx);
			}
		}
		ImGui::End();
	}

	if (m_show_simulation_window) {
		ImGui::SetNextWindowSize(ImVec2(150, 180), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Simulation", &m_show_simulation_window)) {
			if (m_selected_object != m_gameObjects.end()) {
				ImGui::Text("Selected: %s", (*m_selected_object)->get_name().c_str());

				if (ImGui::Button("Build simulator")) {
					m_sim = std::make_unique<sim::SimpleFem>(*m_selected_object, 100000.0, 0.2);
				}
				if (m_sim) {
					ImGui::Checkbox("Keep running", &m_run_simulation);
					if (ImGui::Button("Step simulator") || m_run_simulation) {
						float dt = ctx.delta_time();
						glm::vec3 dir(std::cos(ctx.get_time()), 0.0f, 0.0f);
						for (uint32_t i = 0; i < 3; ++i)
							m_sim->add_constraint(i, dir);
						m_sim->step((sim::Float)dt);

						m_sim->update_objects();
					}
					if (ImGui::Button("Pancake")) {
						reinterpret_cast<sim::SimpleFem*>(m_sim.get())->pancake();
					}

					ImGui::Separator();
					m_sim->draw_ui();
				}


			}
		}
		ImGui::End();
	}

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

	Context::FilePickerCallback callback =
		[&](const Context& ctx, const std::filesystem::path& file, std::string* err) -> bool
	{
		GameObject obj;
		if (!obj.load_tetgen(file, err)) {
			return false;
		}
		else {
			m_gameObjects.push_back(std::make_shared<GameObject>(std::move(obj)));
			return true;
		}
	};
}

void Scene::render(const Context& ctx)
{
	const glm::mat4 view_proj_mat = m_camera.getProjView();

	m_mesh_draw_program.use_program();
	glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(view_proj_mat));

	for (const std::shared_ptr<GameObject>& obj : m_gameObjects) {
		obj->draw();
	}
}

void Scene::add_gameObject(std::shared_ptr<GameObject>& obj)
{
	m_gameObjects.push_back(obj);
}
