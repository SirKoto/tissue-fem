#include "GlobalContext.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <iostream>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>


#include "meshes/TetMesh.hpp"


const char* STRING_IGFD_LOAD_MODEL = "FilePickerModel";

GlobalContext::GlobalContext() :
	m_clear_color(0.45f, 0.55f, 0.60f)
{
	const std::filesystem::path proj_dir(PROJECT_DIR);
	const std::filesystem::path shad_dir = proj_dir / "resources/shaders";

	std::array<Shader, 2> mesh_shaders = {
		Shader((shad_dir / "mesh.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "mesh.frag"), Shader::Type::Fragment)
	};
	m_mesh_draw_program = ShaderProgram(mesh_shaders.data(), (uint32_t)mesh_shaders.size());


	m_selected_object = m_gameObjects.end();
}

void GlobalContext::update()
{
	m_time = (float)glfwGetTime();

	m_delta_time = ImGui::GetIO().DeltaTime;

	m_camera.update();
}

void GlobalContext::update_ui()
{
	// Add more information to menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Load Mesh", nullptr, nullptr, !m_file_picker_open))
			{
				m_file_picker_open = true;
				ImGuiFileDialog::Instance()->OpenDialog(
					STRING_IGFD_LOAD_MODEL,
					"Pick a model to load",
					"Model Files (*.ele *.face *.node){.ele,.face,.node}",
					"."
				);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{			
			ImGui::Checkbox("ImGui Demo Window", &m_show_imgui_demo_window);
			ImGui::Checkbox("Camera Window", &m_show_camera_window);
			ImGui::Checkbox("Inspector Window", &m_show_inspector_window);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("GameObjects"))
		{
			for (auto it = m_gameObjects.begin(); it != m_gameObjects.end(); ++it) {
				if (ImGui::MenuItem(it->get_name().c_str())) {
					m_selected_object = it;
					m_show_inspector_window = true;
				}
			}
			ImGui::EndMenu();
		}

		ImGui::Text("Framerate %.1f", ImGui::GetIO().Framerate);
		ImGui::EndMainMenuBar();
	}

	if (m_show_imgui_demo_window) {
		ImGui::ShowDemoWindow(&m_show_imgui_demo_window);
	}

	if (m_show_camera_window) {
		if (ImGui::Begin("Camera", &m_show_camera_window)) {
			m_camera.render_ui();
		}
		ImGui::End();
	}

	if (m_show_inspector_window) {
		if (ImGui::Begin("Inspector", &m_show_inspector_window)) {
			m_selected_object->update_ui();
		}
		ImGui::End();
	}

	this->handle_file_picker();
}

void GlobalContext::render()
{
	const glm::mat4 view_proj_mat = m_camera.getProjView();

	m_mesh_draw_program.use_program();
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view_proj_mat));

	for (const GameObject& obj : m_gameObjects) {
		obj.draw();
	}
}

void GlobalContext::handle_file_picker()
{

	if (ImGui::BeginPopup("PopupErrorFileDialog")) {
		ImGui::Text(m_file_picker_error.c_str());
		ImGui::EndPopup();
	}

	if (!m_file_picker_open) {
		return;
	}

	IGFD::FileDialog *filedialog = ImGuiFileDialog::Instance();

	if (filedialog->Display(STRING_IGFD_LOAD_MODEL)) {
		
		m_file_picker_open = false;

		if (filedialog->IsOk()) {

			std::filesystem::path file = filedialog->GetFilePathName();

			GameObject obj;
			if (!obj.load_tetgen(file, &m_file_picker_error)) {
				ImGui::OpenPopup("PopupErrorFileDialog");
			}
			else {
				m_gameObjects.push_back(std::move(obj));
			}
		}

		filedialog->Close();
	}


}

