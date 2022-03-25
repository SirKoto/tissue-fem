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
		sel.render_ui(ctx, parent, this);
	}

	ImGui::PopID();
}

void PrimitiveSelector::update(const Context& ctx, GameObject* parent)
{
	if (m_mesh.expired()) {
		m_mesh = parent->get_mesh();
	}

	m_node_movements.clear();
}

const char* PrimitiveSelector::get_name() const
{
	return "Primitive Selector";
}

void Selection::render_ui(const Context& ctx, GameObject* parent, PrimitiveSelector* selector)
{
	const int tmp = 1;
	ImGui::InputScalar("Vidx", ImGuiDataType_U32, &m_vertex, &tmp);
	m_vertex = std::min(m_vertex, (uint32_t)parent->get_mesh()->nodes().size() - 1);

	glm::vec3 pos = reinterpret_cast<const glm::vec3&>(parent->get_mesh()->nodes()[m_vertex]);
	glm::mat4 transform(1.0f);
	transform = glm::translate(transform, pos);
	transform = parent->get_model_matrix() * transform;

	ctx.add_manipulation_guizmo(&transform, (int32_t)((uint64_t)this >> 2));
	glm::vec3 pos_pre = glm::vec3(parent->get_model_matrix() * glm::vec4(pos, 1.f));
	glm::mat4 inv_trans(1.0f);
	inv_trans = glm::translate(inv_trans, -pos);
	transform = transform * inv_trans;

	glm::vec3 translation = glm::vec3(transform * glm::vec4(pos, 1.f)) - pos_pre;
	translation = glm::vec3(parent->get_inv_model_matrix() * glm::vec4(translation, 0.f));
	selector->m_node_movements[m_vertex].delta += translation;
}

} // namespace gobj