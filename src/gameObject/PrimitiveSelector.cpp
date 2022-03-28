#include "PrimitiveSelector.hpp"

#include "Context.hpp"
#include "GameObject.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gobj {

void PrimitiveSelector::render_ui(const Context& ctx, GameObject* parent)
{
	ImGui::PushID(this);
	
	if (ImGui::Button("+")) {
		m_selections.push_back({});
	}

	uint32_t id = 0;
	for (Selection& sel : m_selections) {
		ImGui::PushID(&sel);
		ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
		if (ImGui::TreeNode(std::to_string(id).c_str())) {
			sel.render_ui(ctx, parent, this);

			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	ImGui::PopID();
}

void PrimitiveSelector::update(const Context& ctx, GameObject* parent)
{
	if (m_mesh.expired()) {
		m_mesh = parent->get_mesh();
	}

	for (Selection& sel : m_selections) {
		sel.update(ctx, parent, this);
	}
}

void PrimitiveSelector::late_update(const Context& ctx, GameObject* parent)
{
	m_node_movements.clear();
}

const char* PrimitiveSelector::get_name() const
{
	return "Primitive Selector";
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
		while (it != m_nodes.end()) {
			ImGui::PushID(&(*it));
			ImGui::InputScalar("", ImGuiDataType_U32, &(*it), &tmp);
			*it = std::min(*it, (uint32_t)parent->get_mesh()->nodes().size() - 1);

			ImGui::PopID();
			++it;
		}

		ImGui::TreePop();
	}

	glm::vec3 centroid = glm::vec3(0.0f);
	for (const uint32_t& node : m_nodes) {
		centroid += reinterpret_cast<const glm::vec3&>(parent->get_mesh()->nodes()[node]);
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
				selector->m_node_movements.insert({});
			}
		}
	}
}

} // namespace gobj