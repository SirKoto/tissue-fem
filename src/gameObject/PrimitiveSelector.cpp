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

	for (Selection& sel : m_selections) {
		ImGui::PushID(&sel);
		sel.render_ui(ctx, parent, this);
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

void Selection::render_ui(const Context& ctx, GameObject* parent, PrimitiveSelector* selector)
{
	ImGui::Checkbox("Fixed", &m_fixed);
	const int tmp = 1;
	ImGui::InputScalar("Vidx", ImGuiDataType_U32, &m_node, &tmp);
	m_node = std::min(m_node, (uint32_t)parent->get_mesh()->nodes().size() - 1);

	glm::vec3 pos = reinterpret_cast<const glm::vec3&>(parent->get_mesh()->nodes()[m_node]);
	glm::mat4 transform(1.0f);
	transform = glm::translate(transform, pos);
	transform = parent->get_model_matrix() * transform;

	ctx.add_manipulation_guizmo(&transform, (int32_t)((uint64_t)this >> 2));
	glm::vec3 pos_pre = glm::vec3(parent->get_model_matrix() * glm::vec4(pos, 1.f));
	glm::mat4 inv_trans(1.0f);
	inv_trans = glm::translate(inv_trans, -pos);
	transform = transform * inv_trans;

	glm::vec3 translation = glm::vec3(transform * glm::vec4(pos, 1.f)) - pos_pre;
	m_translation = glm::vec3(parent->get_inv_model_matrix() * glm::vec4(translation, 0.f));
}

void Selection::update(const Context& ctx, GameObject* parent, PrimitiveSelector* selector)
{
	if (glm::dot(m_translation, m_translation) >= 1e-4f || m_fixed) {
		selector->m_node_movements[m_node].delta += m_translation;
	}
	m_translation = glm::vec3(0.0f);
}

} // namespace gobj