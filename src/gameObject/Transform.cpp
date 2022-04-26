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
		bool updated = false;
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(m_transform), glm::value_ptr(t), glm::value_ptr(r), glm::value_ptr(s));
		updated |= ImGui::DragFloat3("Position", glm::value_ptr(t));
		updated |= ImGui::DragFloat3("Scale", glm::value_ptr(s), 1.0f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		updated |= ImGui::DragFloat3("Rotation", glm::value_ptr(r));
		if (updated) {
			ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(t), glm::value_ptr(r), glm::value_ptr(s), glm::value_ptr(m_transform));
			m_inverse = glm::inverse(m_transform);
		}
	}

	bool updated = gc.add_manipulation_guizmo(&m_transform, (int32_t)((uint64_t) this >> 2));
	if (updated) {
		m_inverse = glm::inverse(m_transform);
	}

	ImGui::PopID();
}

void Transform::scale(float k)
{
	m_transform = glm::scale(m_transform, glm::vec3(k));
	m_inverse = glm::inverse(m_transform);
}

void Transform::rotate(const glm::vec3& axis, float rad)
{
	m_transform = glm::rotate(m_transform, rad, axis);
	m_inverse = glm::inverse(m_transform);
}

void Transform::translate(const glm::vec3& v)
{
	m_transform = glm::translate(m_transform, v);
	m_inverse = glm::inverse(m_transform);
}

template<class Archive>
void Transform::serialize(Archive& archive)
{
	archive(TF_SERIALIZE_NVP_MEMBER(m_transform));
	archive(TF_SERIALIZE_NVP_MEMBER(m_inverse));
}

TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(Transform)

} // namespace gobj