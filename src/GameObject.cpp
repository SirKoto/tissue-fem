#include "GameObject.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <imgui.h>
#include <ImGuizmo.h>


#include "Context.hpp"

GameObject::GameObject()
{

}

void GameObject::start_simulation(const Context& ctx)
{
	
}

void GameObject::render_ui(const Context& gc)
{

	ImGui::PushID(m_name.c_str());

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Transform##transform")) {
		ImGui::BeginDisabled(m_disable_interaction);

		m_transform.draw_ui(gc, !m_disable_interaction);

		ImGui::EndDisabled();
		ImGui::TreePop();
	}

	/*decltype(m_addons)::iterator it_addon = m_addons.begin();
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
		if (ImGui::Selectable("Primitive Selector")) {
			this->add_addon<gobj::PrimitiveSelector>();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	*/

	ImGui::PopID();


}

void GameObject::update(const Context& gc)
{

}

void GameObject::late_update(const Context& gc)
{
	
}


template<class Archive>
void GameObject::serialize(Archive& archive)
{
	archive(TF_SERIALIZE_NVP_MEMBER(m_name));
	archive(TF_SERIALIZE_NVP_MEMBER(m_transform));
}

TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(GameObject)

TF_SERIALIZE_TYPE(GameObject)
