#pragma once

#include <glm/glm.hpp>

#include "meshes/TetMesh.hpp"

class GameObject
{
public:

	GameObject();


	bool load_tetgen(const std::filesystem::path& path, std::string* out_err);

	void draw() const;

	void update_ui();

	const std::string& get_name() const { return m_name; }

private:

	std::string m_name;

	TetMesh m_mesh;

	glm::vec3 m_scale = glm::vec3(1.0f);
	glm::vec3 m_translation = glm::vec3(0.0f);


	glm::mat4 get_model_matrix() const;
};

