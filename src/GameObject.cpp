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

bool GameObject::load_tetgen(const std::filesystem::path& path, std::string* out_err)
{
	m_name = path.stem().string();

	return m_mesh.load_tetgen(path, out_err);;
}

void GameObject::draw() const
{
	glm::mat4 model = get_model_matrix();
	glm::mat3 inv_t = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(glm::mat3(inv_t)));

	m_mesh.draw_triangles();
}

void GameObject::update_ui(const Context& gc)
{
	ImGui::PushID(this);

	ImGui::TextDisabled("Model editting");
	{
		glm::vec3 t, s, r;
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(m_transform), glm::value_ptr(t), glm::value_ptr(r), glm::value_ptr(s));
		ImGui::DragFloat3("Position", glm::value_ptr(t));
		ImGui::DragFloat3("Scale", glm::value_ptr(s));
		ImGui::DragFloat3("Rotation", glm::value_ptr(r));
		ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(t), glm::value_ptr(r), glm::value_ptr(s), glm::value_ptr(m_transform));
	}
	if (ImGui::Button("Apply transform to model")) {
		apply_model_transform();
	}
	if (ImGui::Button("Flip face orientation")) {
		m_mesh.flip_face_orientation();
	}
	if (ImGui::Button("Recompute normals")) {
		m_mesh.generate_normals();
		m_mesh.upload_to_gpu();
	}

	gc.add_manipulation_guizmo(&m_transform);

	ImGui::PopID();
}

void GameObject::scale_model(float k)
{
	m_transform = glm::scale(m_transform, glm::vec3(k));
}

void GameObject::rotate_model(const glm::vec3& axis, float rad)
{
	m_transform = glm::rotate(m_transform, rad, axis);
}

void GameObject::translate_model(const glm::vec3& v)
{
	m_transform = glm::translate(m_transform, v);
}

void GameObject::apply_model_transform()
{
	m_mesh.apply_transform(get_model_matrix());
	m_transform = glm::mat4(1.0f);
}
