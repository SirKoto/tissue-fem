#include "Scene.hpp"

#include "Context.hpp"
#include "sim/SimpleFEM.hpp"

#include "meshes/Plane.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <implot.h>
#include <sstream>

Scene::Scene() :
	m_clear_color(0.45f, 0.55f, 0.60f)
{
	m_selected_object = m_gameObjects.end();
	this->physics().insert_primitive(std::make_shared<Plane>(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
}

void Scene::update(const Context& ctx)
{
	m_camera.update();

	for (const std::shared_ptr<GameObject>& obj : m_gameObjects) {
		obj->update(ctx);
	}

	for (const std::shared_ptr<GameObject>& obj : m_gameObjects) {
		obj->late_update(ctx);
	}
}

void Scene::update_ui(const Context& ctx)
{
	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("View"))
		{
			ImGui::Checkbox("GameObjects Window", &m_show_objects_window);
			ImGui::Checkbox("Inspector Window", &m_show_inspector_window);
			ImGui::Checkbox("Simulation Window", &m_show_simulation_window);
			ImGui::Checkbox("Simulation Metrics", &m_show_simulation_metrics);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (m_show_objects_window) {
		ImGui::SetNextWindowSize(ImVec2(250, 280), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("GameObjects", &m_show_objects_window)) {
			std::list<std::shared_ptr<GameObject>>::iterator it = m_gameObjects.begin();
			std::stringstream ss;
			while (it != m_gameObjects.end()) {
				ss.clear(); ss.str(std::string());
				ss << (*it)->get_name() << "####obj";
				ss << static_cast<const void*>(it->get());
				if (ImGui::Selectable(ss.str().c_str(), it == m_selected_object)) {
					m_selected_object = it;
					m_show_inspector_window = true;
				}
				if (ImGui::BeginPopupContextItem()) {
					
					ImGui::Text("Rename GameObject:");
					ImGui::InputText("####textRename", &(*it)->get_name());

					ImGui::Separator();
					if (ImGui::Button("Remove")) {
						if (m_selected_object == it) {
							m_selected_object = m_gameObjects.end();
						}
						it = m_gameObjects.erase(it);
						continue;
					}
					ImGui::EndPopup();
				}
				++it;
			}
		}
		ImGui::End();
	}

	if (m_show_inspector_window) {
		char buff[128];
		std::snprintf(buff, 128, "Inspector -- %s###InspectorWin", (m_selected_object != m_gameObjects.end()) ? (*m_selected_object)->get_name().c_str() : "");
		ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(
			main_viewport->WorkPos.x + main_viewport->WorkSize.x - 400,
			main_viewport->WorkPos.y + 40), ImGuiCond_FirstUseEver);
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

	for (const std::shared_ptr<GameObject>& obj : m_gameObjects) {
		obj->render(ctx);
	}
}

void Scene::add_gameObject(std::shared_ptr<GameObject>& obj)
{
	m_gameObjects.push_back(obj);
}
