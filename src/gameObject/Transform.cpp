#include "Transform.hpp"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

#include "Context.hpp"

namespace gobj {

void Transform::draw_ui(const Context& gc)
{
	ImGui::PushID(this);

	ImGui::TextDisabled("Transform");
	{
		glm::vec3 t, s, r;
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(m_transform), glm::value_ptr(t), glm::value_ptr(r), glm::value_ptr(s));
		ImGui::DragFloat3("Position", glm::value_ptr(t));
		ImGui::DragFloat3("Scale", glm::value_ptr(s), 1.0f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat3("Rotation", glm::value_ptr(r));
		ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(t), glm::value_ptr(r), glm::value_ptr(s), glm::value_ptr(m_transform));
	}

	gc.add_manipulation_guizmo(&m_transform);

	ImGui::Separator();
	ImGui::PopID();
}

void Transform::scale(float k)
{
	m_transform = glm::scale(m_transform, glm::vec3(k));
}

void Transform::rotate(const glm::vec3& axis, float rad)
{
	m_transform = glm::rotate(m_transform, rad, axis);
}

void Transform::translate(const glm::vec3& v)
{
	m_transform = glm::translate(m_transform, v);
}

} // namespace gobj