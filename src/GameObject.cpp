#include "GameObject.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <imgui.h>
#include <ImGuizmo.h>

#include "gameObject/ElasticSim.hpp"
#include "Context.hpp"

GameObject::GameObject()
{
}

bool GameObject::load_tetgen(const std::filesystem::path& path, std::string* out_err)
{
	m_name = path.stem().string();
	m_mesh = std::make_shared<TetMesh>();
	return m_mesh->load_tetgen(path, out_err);;
}

void GameObject::render() const
{
	glm::mat4 model = get_model_matrix();
	glm::mat3 inv_t = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(glm::mat3(inv_t)));

	m_mesh->draw_triangles();
}

void GameObject::render_ui(const Context& gc)
{
	ImGui::PushID(this);
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Transform##transform")) {
		m_transform.draw_ui(gc);
		ImGui::TreePop();
	}

	ImGui::Separator();

	if (ImGui::TreeNode("Mesh ops##mesh ops")) {
		if (ImGui::Button("Apply transform to model")) {
			apply_model_transform();
		}
		if (ImGui::Button("Flip face orientation")) {
			m_mesh->flip_face_orientation();
		}
		if (ImGui::Button("Recompute normals")) {
			m_mesh->generate_normals();
			m_mesh->upload_to_gpu();
		}
		ImGui::TreePop();
	}

	ImGui::Separator();

	decltype(m_addons)::iterator it_addon = m_addons.begin();
	while(it_addon != m_addons.end()){
		std::unique_ptr<gobj::Addon>& addon = *it_addon;
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		bool is_open = ImGui::TreeNode((void*)addon.get(), addon->get_name());
		// Item context menu
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::Selectable("Remove")) {
				it_addon = m_addons.erase(it_addon);
					ImGui::EndPopup();
					if (is_open) {
						ImGui::TreePop();
					}
					continue;
			}
			ImGui::EndPopup();
		}

		// Body of the tree node
		if(is_open){
			addon->render_ui(gc, this);
			ImGui::Separator();
			ImGui::TreePop();
		}
		

		++it_addon;
	}

	// Add addon
	ImGui::Button("Add addon");
	if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_None)) {
		if (ImGui::Selectable("Elastic Simulator")) {
			this->add_addon<gobj::ElasticSim>();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::PopID();
}

void GameObject::update(const Context& gc)
{
	for (const auto& addon : m_addons) {
		addon->update(gc, this);
	}
}

void GameObject::apply_model_transform()
{
	m_mesh->apply_transform(get_model_matrix());
	m_transform.set_identity();
}
