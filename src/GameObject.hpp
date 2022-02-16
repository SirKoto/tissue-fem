#pragma once

#include <glm/glm.hpp>

#include "meshes/TetMesh.hpp"

class GlobalContext;
class GameObject
{
public:

	GameObject();


	bool load_tetgen(const std::filesystem::path& path, std::string* out_err = nullptr);

	void draw() const;

	void update_ui(const GlobalContext& gc);

	const glm::mat4& get_model_matrix() const { return m_transform; }

	const std::string& get_name() const { return m_name; }

	const TetMesh& get_mesh() const { return m_mesh; }
	TetMesh& get_mesh() { return m_mesh; }

	void scale_model(float k);
	void rotate_model(const glm::vec3& axis, float rad);
	void translate_model(const glm::vec3& v);

	void apply_model_transform();

private:

	std::string m_name;

	TetMesh m_mesh;

	glm::mat4 m_transform = glm::mat4(1.0f);

};

