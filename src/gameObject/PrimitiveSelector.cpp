#include "PrimitiveSelector.hpp"

#include "Context.hpp"
#include "GameObject.hpp"

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
}

const char* PrimitiveSelector::get_name() const
{
	return "Primitive Selector";
}

void Selection::render_ui(const Context& ctx, GameObject* parent, PrimitiveSelector* selector)
{
	glm::vec3 pos = reinterpret_cast<const glm::vec3&>(parent->get_mesh()->nodes()[m_vertex]);
	glm::mat4 transform(1.0f);
	transform = glm::translate(transform, pos);
	ctx.add_manipulation_guizmo(&transform, (int32_t)((uint64_t)this >> 2));
}

} // namespace gobj