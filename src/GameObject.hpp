#pragma once

#include <glm/glm.hpp>

#include "meshes/TetMesh.hpp"

class GlobalContext;
class GameObject
{
public:

	GameObject();


	bool load_tetgen(const std::filesystem::path& path, std::string* out_err);

	void draw() const;

	void update_ui(const GlobalContext& gc);

	const glm::mat4& get_model_matrix() const { return m_transform; }

	const std::string& get_name() const { return m_name; }

private:

	std::string m_name;

	TetMesh m_mesh;

	glm::mat4 m_transform = glm::mat4(1.0f);

};

