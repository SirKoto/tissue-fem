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

	for (const std::shared_ptr<GameObject>& obj : m_gameObjects) {
		obj->update(ctx);
	}
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
		obj->render();
	}
}

void Scene::add_gameObject(std::shared_ptr<GameObject>& obj)
{
	m_gameObjects.push_back(obj);
}
