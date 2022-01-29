#include "GlobalContext.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

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
}

void GlobalContext::render()
{
}

