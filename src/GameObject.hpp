#pragma once

#include <glm/glm.hpp>

#include "meshes/TetMesh.hpp"

#include "gameObject/Transform.hpp"
#include "gameObject/Addon.hpp"

class Context;
class GameObject
{
public:

	GameObject();

	GameObject(const GameObject&) = delete;
	GameObject(GameObject&&) = default;
	GameObject& operator=(const GameObject&) = delete;
	GameObject& operator=(GameObject&&) = default;

	bool load_tetgen(const std::filesystem::path& path, std::string* out_err = nullptr);

	void draw() const;

	void render_ui(const Context& gc);

	const glm::mat4& get_model_matrix() const { return m_transform.mat4(); }
	const gobj::Transform& get_transform() const { return m_transform; }
	gobj::Transform& get_transform() { return m_transform; }

	const std::string& get_name() const { return m_name; }

	const TetMesh& get_mesh() const { return m_mesh; }
	TetMesh& get_mesh() { return m_mesh; }

	void apply_model_transform();

private:

	std::string m_name;

	TetMesh m_mesh;

	gobj::Transform m_transform;

	std::list<std::unique_ptr<gobj::Addon>> m_addons;

};

