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
	m_name = path.filename().string();

	return m_mesh.load_tetgen(path, out_err);;
}

void GameObject::draw() const
{

	glm::mat4 model(1.0f);
	model = glm::translate(model, m_translation);
	model = glm::scale(model, m_scale);
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(model));

	m_mesh.draw_triangles();
}

void GameObject::update_ui()
{
	ImGui::Text(m_name.c_str());
}
