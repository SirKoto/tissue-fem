#include "GlobalContext.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <iostream>
#include <filesystem>

#include "meshes/TetMesh.hpp"


const char* STRING_IGFD_LOAD_MODEL = "FilePickerModel";

GlobalContext::GlobalContext() :
	m_clear_color(0.45f, 0.55f, 0.60f)
{

}

void GlobalContext::update()
{
	m_time = (float)glfwGetTime();

	m_delta_time = ImGui::GetIO().DeltaTime;
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

			ImGui::EndMenu();
		}

		ImGui::Text("Framerate %.1f", ImGui::GetIO().Framerate);
		ImGui::EndMainMenuBar();
	}

	if (m_show_imgui_demo_window) {
		ImGui::ShowDemoWindow(&m_show_imgui_demo_window);
	}

	this->handle_file_picker();
}

void GlobalContext::render()
{
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

			TetMesh mesh;
			if (!mesh.load_tetgen(file, &m_file_picker_error)) {
				ImGui::OpenPopup("PopupErrorFileDialog");
			}
			else {
				m_meshes.push_back(std::move(mesh));
			}
		}

		filedialog->Close();
	}


}

