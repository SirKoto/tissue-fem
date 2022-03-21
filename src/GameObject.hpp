#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "meshes/TetMesh.hpp"
#include "gameObject/Transform.hpp"
#include "gameObject/Addon.hpp"

class Context;
class GameObject
{
public:

	GameObject();

	GameObject(const GameObject&) = delete;
	GameObject(GameObject&&) noexcept = default;
	GameObject& operator=(const GameObject&) = delete;
	GameObject& operator=(GameObject&&) = default;

	bool load_tetgen(const std::filesystem::path& path, std::string* out_err = nullptr);

	void render() const;

	void render_ui(const Context& gc);

	void update(const Context& gc);

	const glm::mat4& get_model_matrix() const { return m_transform.mat4(); }
	const gobj::Transform& get_transform() const { return m_transform; }
	gobj::Transform& get_transform() { return m_transform; }

	const std::string& get_name() const { return m_name; }

	const std::shared_ptr<TetMesh>& get_mesh() const { return m_mesh; }
	std::shared_ptr<TetMesh>& get_mesh() { return m_mesh; }

	void apply_model_transform();

	template<typename T>
	T* add_addon();

private:

	std::string m_name;

	std::shared_ptr<TetMesh> m_mesh;

	gobj::Transform m_transform;

	std::list<std::unique_ptr<gobj::Addon>> m_addons;

};

template<typename T>
inline T* GameObject::add_addon()
{
	T* ptr = new T();
	std::unique_ptr<gobj::Addon> addon(ptr);
	m_addons.push_back(std::move(addon));
	return ptr;
}
