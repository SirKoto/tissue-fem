#include "Context.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <ImGuiFileDialog.h>
#include <implot.h>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <glm/gtc/type_ptr.hpp>

#include "gameObject/SimulatedGameObject.hpp"
#include "meshes/TetMesh.hpp"
#include "sim/SimpleFEM.hpp"


const char* STRING_IGFD_LOAD_MODEL = "FilePickerModel";
const char* STRING_IGFD_SAVE_AS = "FilePickerSaveAs";
const char* STRING_IGFD_LOAD = "FilePickerLoad";

Context::Context(Engine* engine) :
	m_engine(engine)
{
	assert(m_engine != nullptr);
	m_delta_times_buffer.fill(1.0f / 60.0f);
}

void Context::update()
{
	m_time = (float)glfwGetTime();

	ImGuiIO& io = ImGui::GetIO();

	std::rotate(m_delta_times_buffer.begin(), std::next(m_delta_times_buffer.begin()), m_delta_times_buffer.end());
	m_delta_times_buffer.back() = io.DeltaTime;
	// Recover from a breakpoint
	if (m_delta_times_buffer.back() > 1.0f) {
		m_delta_times_buffer.back() = 1 / 60.0f;
	}

	m_dynamic_delta_time = std::accumulate(m_delta_times_buffer.begin(), m_delta_times_buffer.end(), 0.0f) / (float)m_delta_times_buffer.size();

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

void Context::draw_ui()
{
	// Add more information to menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Load Mesh", nullptr, nullptr, !m_file_picker_open))
			{
				this->open_file_picker("Pick a model to load",
					"Model Files (*.ele *.face *.node){.ele,.face,.node}",

					[](const Context& ctx, 
						Scene& scene, 
						const std::filesystem::path& p, 
						std::string* err) -> bool {

						std::shared_ptr<SimulatedGameObject> obj = std::make_shared<SimulatedGameObject>();

						bool res = obj->load_tetgen(p, err);
						if (!res) {
							return false;
						}

						scene.add_gameObject(std::move(obj));

						return true;
					}
					);
				
			}
			
			if (ImGui::MenuItem("Save", nullptr, nullptr, !m_engine->m_scene_path.empty() && !m_file_picker_open)) {
				if (!m_engine->save_scene(m_engine->m_scene_path , &m_file_picker_error)) {
					ImGui::OpenPopup("PopupErrorFileDialog");
				}
			}

			if (ImGui::MenuItem("Save As", nullptr, nullptr, !m_file_picker_open)) {

				m_file_picker_open = true;
				ImGuiFileDialog::Instance()->OpenDialog(
					STRING_IGFD_SAVE_AS,
					"Save As", ".bin,.json",
					".", "", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite);
			}

			if (ImGui::MenuItem("Load Scene", nullptr, nullptr, !m_file_picker_open)) {
				m_file_picker_open = true;
				ImGuiFileDialog::Instance()->OpenDialog(
					STRING_IGFD_LOAD,
					"Save As", ".bin,.json",
					".", "", 1, nullptr);
			}

			if (ImGui::BeginMenu("Example models")) {

				if (ImGui::MenuItem("Armadillo Simple")) {
					const std::filesystem::path proj_dir(PROJECT_DIR);
					const std::filesystem::path file = proj_dir / "resources/models/armadilloSimp/armadSimp.ele";
					std::shared_ptr<SimulatedGameObject> obj = std::make_shared<SimulatedGameObject>();
					bool loaded = obj->load_tetgen(file);
					assert(loaded);
					obj->get_transform().translate(glm::vec3(0.0f, 2.0f, 0.0f));
					obj->get_transform().scale(0.01f);
					obj->get_transform().rotate(glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(180.0f));
					m_engine->m_scene->add_gameObject(std::move(obj));
				}
				if (ImGui::MenuItem("Armadillo Complex")) {
					const std::filesystem::path proj_dir(PROJECT_DIR);
					const std::filesystem::path file = proj_dir / "resources/models/armadilloSimp/armadilloComp.ele";
					std::shared_ptr<SimulatedGameObject> obj = std::make_shared<SimulatedGameObject>();
					bool loaded = obj->load_tetgen(file);
					assert(loaded);
					obj->get_transform().scale(0.01f);
					obj->get_transform().rotate(glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(-90.0f));
					m_engine->m_scene->add_gameObject(std::move(obj));
				}
				if (ImGui::MenuItem("Armadillo Complex Bad")) {
					const std::filesystem::path proj_dir(PROJECT_DIR);
					const std::filesystem::path file = proj_dir / "resources/models/armadilloSimp/armadilloComp_bad.ele";
					std::shared_ptr<SimulatedGameObject> obj = std::make_shared<SimulatedGameObject>();
					bool loaded = obj->load_tetgen(file);
					assert(loaded);
					obj->get_transform().scale(0.01f);
					obj->get_transform().rotate(glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(-90.0f));
					m_engine->m_scene->add_gameObject(std::move(obj));
				}
				if (ImGui::MenuItem("Sphere")) {
					const std::filesystem::path proj_dir(PROJECT_DIR);
					const std::filesystem::path file = proj_dir / "resources/models/sphere/sphere.ele";
					std::shared_ptr<SimulatedGameObject> obj = std::make_shared<SimulatedGameObject>();
					bool loaded = obj->load_tetgen(file);
					assert(loaded);
					m_engine->m_scene->add_gameObject(std::move(obj));
				}
				if (ImGui::MenuItem("Sphere Bad")) {
					const std::filesystem::path proj_dir(PROJECT_DIR);
					const std::filesystem::path file = proj_dir / "resources/models/sphere/sphere_bad.ele";
					std::shared_ptr<SimulatedGameObject> obj = std::make_shared<SimulatedGameObject>();
					bool loaded = obj->load_tetgen(file);
					assert(loaded);
					m_engine->m_scene->add_gameObject(std::move(obj));
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{			
			ImGui::Checkbox("ImGui Demo Window", &m_show_imgui_demo_window);
			ImGui::Checkbox("ImPlot Demo Window", &m_show_implot_demo_window);
			ImGui::Checkbox("Camera Window", &m_show_camera_window);
			ImGui::Checkbox("Profiling Window", &m_show_profiling_window);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			int32_t max_fps =(int32_t)std::round( 1.0 / m_engine->m_min_delta_time );
			if (ImGui::InputInt("Max FPS", &max_fps)) {
				max_fps = std::max(max_fps, 10);
				m_engine->m_min_delta_time = 1.0 / (double)max_fps;
			}

			if (ImGui::InputFloat("Objective FPS", &m_objective_fps, 1.0f)) {
				m_objective_dt = 1.0f / m_objective_fps;
			}

			ImGui::Text("Simulation delta time:");
			if (ImGui::RadioButton("Dynamic", m_use_dynamic_dt)) {
				m_use_dynamic_dt = true;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Static", !m_use_dynamic_dt)) {
				m_use_dynamic_dt = false;
			}

			ImGui::BeginDisabled(m_use_dynamic_dt);
			float dynamic_fps = 1.0f / m_static_dt;
			if (ImGui::InputFloat("Static dt simulation", &dynamic_fps, 1.0f)) {
				m_static_dt = 1.0f / dynamic_fps;
			}
			ImGui::EndDisabled();

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

		ImGui::Text("Framerate %.1f", ImGui::GetIO().Framerate);

		if (ImGui::MenuItem("Play", nullptr, m_engine->m_simulation_mode)) {

			// Save or retrieve the scene
			std::string err;
			bool res;
			if (!m_engine->m_simulation_mode) {
				res = m_engine->save_scene(m_engine->m_scene_path_tmp, &err);
			}
			else {
				res = m_engine->reload_scene(m_engine->m_scene_path_tmp, &err);
			}

			if (!res) {
				ImGui::OpenPopup("PopupErrorFileDialog");
				m_engine->stop_simulation();
			}
			else if (!m_engine->m_simulation_mode) {
				m_engine->signal_start_simulation();
			}
			else {
				m_engine->stop_simulation();
			}

		}
		if (ImGui::MenuItem("Pause", nullptr, !m_engine->m_run_simulation, m_engine->m_simulation_mode)) {
			m_engine->m_run_simulation = !m_engine->m_run_simulation;
		}

		ImGui::EndMainMenuBar();
	}

	if (m_show_imgui_demo_window) {
		ImGui::ShowDemoWindow(&m_show_imgui_demo_window);
	}

	if (m_show_implot_demo_window) {
		ImPlot::ShowDemoWindow(&m_show_implot_demo_window);
	}

	if (m_show_camera_window) {
		if (ImGui::Begin("Camera", &m_show_camera_window)) {
			m_engine->m_scene->camera().draw_ui();
		}
		ImGui::End();
	}

	if (m_show_profiling_window) {
		ImGui::SetNextWindowSize(ImVec2(450, 380), ImGuiCond_FirstUseEver);
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(
			main_viewport->WorkPos.x + main_viewport->WorkSize.x - 400,
			main_viewport->WorkPos.y + 340), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Profiling", &m_show_profiling_window)) {
			ui_draw_profiling();
		}
		ImGui::End();
	}


	if (m_show_world_grid) {
		ImGuizmo::DrawGrid(glm::value_ptr(camera().getView()), glm::value_ptr(camera().getProj()), glm::value_ptr(glm::mat4(1.0f)), 100.f);
	}

	this->handle_file_picker();
}


bool Context::add_manipulation_guizmo(glm::mat4* transform, int32_t id) const
{
	assert(transform != nullptr);
	return this->add_manipulation_guizmo(transform, m_guizmos_mode, nullptr, id);
}

bool Context::add_manipulation_guizmo(glm::mat4* transform, glm::mat4* delta_transform, int32_t id) const
{
	assert(transform != nullptr);
	return this->add_manipulation_guizmo(transform, m_guizmos_mode, delta_transform, id);
}

bool Context::add_manipulation_guizmo(glm::mat4* transform, Context::GuizmosInteraction op_ctx, glm::mat4* delta_transform, int32_t id) const
{
	assert(transform != nullptr);

	if (!m_show_guizmos) {
		return false;
	}

	ImGuizmo::OPERATION op;
	switch (op_ctx)
	{
	case Context::GuizmosInteraction::eTranslate:
		op = ImGuizmo::OPERATION::TRANSLATE;
		break;
	case Context::GuizmosInteraction::eRotate:
		op = ImGuizmo::OPERATION::ROTATE;
		break;
	case Context::GuizmosInteraction::eScale:
		op = ImGuizmo::OPERATION::SCALE;
		break;
	default:
		assert(false);
		return false;
	}

	ImGuizmo::SetID(id);
	return ImGuizmo::Manipulate(
		glm::value_ptr(camera().getView()),
		glm::value_ptr(camera().getProj()),
		op,
		m_use_local_space_interaction ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD,
		glm::value_ptr(*transform),
		(float*)delta_transform);
}

void Context::open_file_picker(
	const char* window_title,
	const char* filter, 
	const FilePickerCallback& callback)
{
	if (m_file_picker_open) {
		return;
	}

	m_file_picker_open = true;
	ImGuiFileDialog::Instance()->OpenDialog(
		STRING_IGFD_LOAD_MODEL,
		window_title,
		filter,
		"."
	);

	m_file_picker_callback = callback;
}

void Context::handle_file_picker()
{

	if (ImGui::BeginPopup("PopupErrorFileDialog")) {
		ImGui::Text("ERROR:");
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

			bool res = m_file_picker_callback(*this, *m_engine->m_scene, file, &m_file_picker_error);
			if (!res) {
				ImGui::OpenPopup("PopupErrorFileDialog");
			}
		}

		filedialog->Close();
	}

	if (filedialog->Display(STRING_IGFD_SAVE_AS, ImGuiWindowFlags_NoCollapse, ImVec2(0, 280))) {

		m_file_picker_open = false;

		if (filedialog->IsOk()) {

			std::filesystem::path file = filedialog->GetFilePathName();
			m_engine->m_scene_path = file;
			if (!m_engine->save_scene(file, &m_file_picker_error)) {
				m_engine->m_scene_path = "";
				ImGui::OpenPopup("PopupErrorFileDialog");
			}
		}

		filedialog->Close();
	}

	if (filedialog->Display(STRING_IGFD_LOAD, ImGuiWindowFlags_NoCollapse, ImVec2(0, 280))) {

		m_file_picker_open = false;

		if (filedialog->IsOk()) {

			std::filesystem::path file = filedialog->GetFilePathName();
			
			if (!std::filesystem::exists(file)) {
				m_file_picker_error = std::string("ERROR: ") + file.string() + " does not exist!";
				ImGui::OpenPopup("PopupErrorFileDialog");
			}
			else {
				m_engine->m_scene_path = file;
				if (!m_engine->reload_scene(file, &m_file_picker_error)) {
					m_engine->m_scene_path = "";
					ImGui::OpenPopup("PopupErrorFileDialog");
				}
			}
		}

		filedialog->Close();
	}


}

void Context::ui_draw_profiling()
{
	ImGui::SliderFloat("Past seconds", &m_metrics_past_seconds, 0.1f, 30.0f, "%.1f");

	if (ImPlot::BeginPlot("Engine", ImVec2(-1, 200))) {
		ImPlot::SetupAxes("time (s)", "dt (s)");
		float x = m_engine_timings.size() > 0 ? m_engine_timings.back().time : 0.0f;
		ImPlot::SetupAxisLimits(ImAxis_X1, x - m_metrics_past_seconds, x, ImGuiCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 0.03);
		auto plot_engine_timings = [&](const char* id, const float* data) -> void {
			ImPlot::PlotLine(id,
				&m_engine_timings.data()->time,
				data,
				(int)m_engine_timings.size(),
				(int)m_engine_timings.offset(),
				sizeof(*m_engine_timings.data()));
		};
		plot_engine_timings("Window poll", &m_engine_timings.data()->window_poll);
		plot_engine_timings("Context update", &m_engine_timings.data()->context_update);
		plot_engine_timings("Scene draw ui", &m_engine_timings.data()->scene_draw_ui);
		plot_engine_timings("Scene update", &m_engine_timings.data()->scene_update);
		plot_engine_timings("Scene render", &m_engine_timings.data()->scene_render);
		plot_engine_timings("ImGui new Frame", &m_engine_timings.data()->imgui_new_frame);
		plot_engine_timings("ImGui render cpu", &m_engine_timings.data()->imgui_render_cpu);
		plot_engine_timings("ImGui render gpu", &m_engine_timings.data()->imgui_render_gpu);

		ImPlot::EndPlot();
	}

	if (ImPlot::BeginPlot("Scene", ImVec2(-1, 200))) {
		ImPlot::SetupAxes("time (s)", "dt (s)");
		float x = m_engine_timings.size() > 0 ? m_engine_timings.back().time : 0.0f;
		ImPlot::SetupAxisLimits(ImAxis_X1, x - m_metrics_past_seconds, x, ImGuiCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 0.03);
		auto plot_engine_timings = [&](const char* id, const float* data) -> void {
			ImPlot::PlotLine(id,
				&m_engine_timings.data()->time,
				data,
				(int)m_engine_timings.size(),
				(int)m_engine_timings.offset(),
				sizeof(*m_engine_timings.data()));
		};
		plot_engine_timings("Update", &m_engine_timings.data()->scene_times.update);
		plot_engine_timings("Simulation update", &m_engine_timings.data()->scene_times.simulation_update);
		plot_engine_timings("Late Update", &m_engine_timings.data()->scene_times.late_update);

		ImPlot::EndPlot();
	}
}

