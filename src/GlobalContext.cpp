#include "GlobalContext.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <ImGuiFileDialog.h>
#include <iostream>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>


#include "meshes/TetMesh.hpp"
#include "sim/SimpleFEM.hpp"

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

	ImGuiIO& io = ImGui::GetIO();

	m_delta_time = io.DeltaTime;

	m_camera.update();

	if (!io.WantCaptureKeyboard) {
		if (io.KeysDown[GLFW_KEY_E]) {
			m_guizmos_mode = GuizmosInteraction::eScale;
		}
		else if (io.KeysDown[GLFW_KEY_R]) {
			m_guizmos_mode = GuizmosInteraction::eRotate;
		}
		else if (io.KeysDown[GLFW_KEY_T]) {
			m_guizmos_mode = GuizmosInteraction::eTranslate;
		}
	}
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

			if (ImGui::BeginMenu("Example models")) {

				if (ImGui::MenuItem("Armadillo Simple")) {
					const std::filesystem::path proj_dir(PROJECT_DIR);
					const std::filesystem::path file = proj_dir / "resources/models/armadilloSimp/armadSimp.ele";
					GameObject obj;
					bool loaded = obj.load_tetgen(file);
					assert(loaded);
					obj.scale_model(0.01f);
					obj.rotate_model(glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(180.0f));
					obj.get_mesh().flip_face_orientation();
					m_gameObjects.push_back(std::make_shared<GameObject>(std::move(obj)));
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{			
			ImGui::Checkbox("ImGui Demo Window", &m_show_imgui_demo_window);
			ImGui::Checkbox("Camera Window", &m_show_camera_window);
			ImGui::Checkbox("Inspector Window", &m_show_inspector_window);
			ImGui::Checkbox("Simulation Window", &m_show_simulation_window);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Guizmos")) {
			ImGui::Checkbox("Enable World Grid", &m_show_world_grid);
			ImGui::Checkbox("Enable Guizmos", &m_show_guizmos);
			ImGui::TextDisabled("Interaction Space");
			if (ImGui::RadioButton("Local", m_use_local_space_interaction)) {
				m_use_local_space_interaction = true;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("World", !m_use_local_space_interaction)) {
				m_use_local_space_interaction = false;
			}
			ImGui::TextDisabled("Interaction Type"); ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted("You can also use 'e', 'r' and 't' keys to change between different interaction types.");
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
			if (ImGui::RadioButton("Translate", m_guizmos_mode == GuizmosInteraction::eTranslate)) {
				m_guizmos_mode = GuizmosInteraction::eTranslate;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Rotate", m_guizmos_mode == GuizmosInteraction::eRotate)) {
				m_guizmos_mode = GuizmosInteraction::eRotate;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Scale", m_guizmos_mode == GuizmosInteraction::eScale)) {
				m_guizmos_mode = GuizmosInteraction::eScale;
			}

			ImGui::EndMenu();
		}

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
		char buff[128];
		std::snprintf(buff, 128, "Inspector -- %s###InspectorWin", (m_selected_object != m_gameObjects.end()) ? (*m_selected_object)->get_name().c_str() : "");
		ImGui::SetNextWindowSize(ImVec2(350, 280), ImGuiCond_FirstUseEver);
		if (ImGui::Begin(buff, &m_show_inspector_window)) {
			if (m_selected_object != m_gameObjects.end()) {
				(*m_selected_object)->update_ui(*this);
			}
		}
		ImGui::End();
	}

	if (m_show_simulation_window) {
		ImGui::SetNextWindowSize(ImVec2(150, 80), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Simulation")) {
			if (m_selected_object != m_gameObjects.end()) {
				ImGui::Text("Selected: %s", (*m_selected_object)->get_name().c_str());
				if (ImGui::Button("TEST")) {
					sim::SimpleFem fem(*m_selected_object);
					std::cout << "Loaded" << std::endl;
					fem.step(1.0f / 60.0f);
					std::cout << "Stepped" << std::endl;
				}
			}
		}
		ImGui::End();
	}

	if (m_show_world_grid) {
		ImGuizmo::DrawGrid(glm::value_ptr(camera().getView()), glm::value_ptr(camera().getProj()), glm::value_ptr(glm::mat4(1.0f)), 100.f);
	}

	this->handle_file_picker();
}

void GlobalContext::render()
{
	const glm::mat4 view_proj_mat = m_camera.getProjView();

	m_mesh_draw_program.use_program();
	glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(view_proj_mat));

	for (const std::shared_ptr<GameObject>& obj : m_gameObjects) {
		obj->draw();
	}
}

void GlobalContext::add_manipulation_guizmo(glm::mat4* transform) const
{
	assert(transform != nullptr);

	if (!m_show_guizmos) {
		return;
	}

	ImGuizmo::OPERATION op;
	switch (m_guizmos_mode)
	{
	case GlobalContext::GuizmosInteraction::eTranslate:
		op = ImGuizmo::OPERATION::TRANSLATE;
		break;
	case GlobalContext::GuizmosInteraction::eRotate:
		op = ImGuizmo::OPERATION::ROTATE;
		break;
	case GlobalContext::GuizmosInteraction::eScale:
		op = ImGuizmo::OPERATION::SCALE;
		break;
	default:
		assert(false);
		return;
	}

	ImGuizmo::Manipulate(
		glm::value_ptr(camera().getView()),
		glm::value_ptr(camera().getProj()),
		op,
		m_use_local_space_interaction ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD,
		glm::value_ptr(*transform));
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

	if (filedialog->Display(STRING_IGFD_LOAD_MODEL, ImGuiWindowFlags_NoCollapse, ImVec2(0, 280))) {
		
		m_file_picker_open = false;

		if (filedialog->IsOk()) {

			std::filesystem::path file = filedialog->GetFilePathName();

			GameObject obj;
			if (!obj.load_tetgen(file, &m_file_picker_error)) {
				ImGui::OpenPopup("PopupErrorFileDialog");
			}
			else {
				m_gameObjects.push_back(std::make_shared<GameObject>(std::move(obj)));
			}
		}

		filedialog->Close();
	}


}

