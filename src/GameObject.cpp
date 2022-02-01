#include "GameObject.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <imgui.h>

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
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));

	m_mesh.draw_triangles();
}

void GameObject::update_ui()
{
	ImGui::PushID(this);

	ImGui::TextDisabled("Model editting");
	ImGui::InputFloat3("Position", glm::value_ptr(m_translation));
	ImGui::InputFloat3("Scale", glm::value_ptr(m_scale));

	if (ImGui::Button("Apply transform to model")) {
		m_mesh.apply_transform(get_model_matrix());
		m_translation = glm::vec3(0.0f);
		m_scale = glm::vec3(1.0f);
	}
	if (ImGui::Button("Flip face orientation")) {
		m_mesh.flip_face_orientation();
	}
	if (ImGui::Button("Recompute normals")) {
		m_mesh.generate_normals();
		m_mesh.upload_to_gpu();
	}

	ImGui::PopID();
}

glm::mat4 GameObject::get_model_matrix() const
{
	glm::mat4 model(1.0f);
	model = glm::translate(model, m_translation);
	model = glm::scale(model, m_scale);
	return model;
}
