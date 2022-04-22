#include "PrimitiveSelector.hpp"

#include "Context.hpp"
#include "GameObject.hpp"

#include <glad/glad.h>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gobj {

PrimitiveSelector::PrimitiveSelector()
{
}

void PrimitiveSelector::render_ui(const Context& ctx, GameObject* parent)
{
	ImGui::PushID(this);
	
	if (ImGui::Button("+")) {
		m_selections.push_back({});
	}

	uint32_t id = 0;
	std::list<Selection>::iterator it = m_selections.begin();
	while (it != m_selections.end()) {
		Selection& sel = *it;
		ImGui::PushID(&sel);
		ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
		if (ImGui::TreeNode(std::to_string(id++).c_str())) {
			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::Selectable("Remove Selection")) {
					it = m_selections.erase(it);
					ImGui::EndPopup();
					ImGui::TreePop();
					ImGui::PopID();
					continue;
				}
				ImGui::EndPopup();
			}
			sel.render_ui(ctx, parent, this);

			ImGui::TreePop();
		}
		ImGui::PopID();
		++it;
	}

	ImGui::InputFloat("Particle radius", &m_particle_manager.particle_radius(), 0.01f);
	m_particle_manager.particle_radius() = std::max(0.0f, m_particle_manager.particle_radius());

	ImGui::PopID();
}

void PrimitiveSelector::render(const Context& ctx, const GameObject& parent) const
{
	m_particle_manager.draw(ctx, parent.get_model_matrix());
}

void PrimitiveSelector::update(const Context& ctx, GameObject* parent)
{
	if (m_mesh.expired()) {
		m_mesh = parent->get_mesh();
	}

	for (Selection& sel : m_selections) {
		sel.update(ctx, parent, this);
	}

	auto mesh = m_mesh.lock();
	std::vector<glm::vec3> pos;
	for (Selection& sel : m_selections) {
		for (uint32_t v : sel.nodes()) {
			pos.push_back(mesh->nodes_glm()[v]);
		}
	}
	m_particle_manager.set_particles(pos);
}

void PrimitiveSelector::late_update(const Context& ctx, GameObject* parent)
{
	m_node_movements.clear();
}

PrimitiveSelector::Selection::Selection()
{
	m_nodes.push_back({});
}

void PrimitiveSelector::Selection::render_ui(const Context& ctx, GameObject* parent, PrimitiveSelector* selector)
{
	ImGui::Checkbox("Fixed", &m_fixed);
	const int tmp = 1;
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Points")) {
		if (ImGui::Button("Add point")) {
			m_nodes.push_back({});
		}

		std::list<uint32_t>::iterator it = m_nodes.begin();
		uint32_t count = 0;
		while (it != m_nodes.end()) {
			ImGui::PushID(it._Ptr);
			std::string id = "####in" + std::to_string(count++);
			ImGui::InputScalar(id.c_str(),
				ImGuiDataType_U32, &(*it), &tmp);
			*it = std::min(*it, (uint32_t)parent->get_mesh()->nodes().size() - 1);

			if (m_nodes.size() > 1 && ImGui::BeginPopupContextItem(id.c_str())) {
				if (ImGui::Selectable("Remove Point")) {
					it = m_nodes.erase(it);
					ImGui::EndPopup();
					ImGui::PopID();
					continue;
				}
				ImGui::EndPopup();
			}

			ImGui::PopID();
			++it;
		}

		ImGui::TreePop();
	}

	glm::vec3 centroid = glm::vec3(0.0f);
	for (const uint32_t& node : m_nodes) {
		centroid += sim::cast_vec3(parent->get_mesh()->nodes()[node]);
	}
	centroid /= (float)m_nodes.size();
	glm::mat4 transform(1.0f);
	transform = glm::translate(transform, centroid);
	transform = parent->get_model_matrix() * transform;

	ctx.add_manipulation_guizmo(&transform, (int32_t)((uint64_t)this >> 2));
	glm::mat4 inv_trans(1.0f);
	inv_trans = glm::translate(inv_trans, -centroid);
	transform = transform * inv_trans;

	for (const uint32_t& node : m_nodes) {
		glm::vec3 pos = reinterpret_cast<const glm::vec3&>(parent->get_mesh()->nodes()[node]);
		glm::vec3 pos_pre = glm::vec3(parent->get_model_matrix() * glm::vec4(pos, 1.f));
		glm::vec3 translation = glm::vec3(transform * glm::vec4(pos, 1.f)) - pos_pre;

		if (glm::dot(translation, translation) >= 1e-4f || m_fixed) {
			selector->m_node_movements[node].delta += translation;
		}
	}
}

void PrimitiveSelector::Selection::update(const Context& ctx, GameObject* parent, PrimitiveSelector* selector)
{
	if (m_fixed) {
		for (const uint32_t& node : m_nodes) {
			if (!selector->m_node_movements.count(node)) {
				selector->m_node_movements.insert({ node, {} });
			}
		}
	}
}

} // namespace gobj