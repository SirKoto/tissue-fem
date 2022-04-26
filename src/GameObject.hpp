#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "meshes/TetMesh.hpp"
#include "gameObject/Transform.hpp"
#include "gameObject/ElasticSim.hpp"
#include "utils/serialization.hpp"

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

	void render(const Context& ctx) const;

	void render_ui(const Context& gc);

	void update(const Context& gc);
	void late_update(const Context& gc);

	const glm::mat4& get_model_matrix() const { return m_transform.mat4(); }
	const glm::mat4& get_inv_model_matrix() const { return m_transform.inverse(); }
	const gobj::Transform& get_transform() const { return m_transform; }
	gobj::Transform& get_transform() { return m_transform; }

	const std::string& get_name() const { return m_name; }
	std::string& get_name() { return m_name; }

	const std::shared_ptr<TetMesh>& get_mesh() const { return m_mesh; }
	std::shared_ptr<TetMesh>& get_mesh() { return m_mesh; }
	//const gobj::ElasticSim& get_sim() const { return m_sim; }
	//gobj::ElasticSim& get_sim() { return m_sim; }
	//const gobj::PrimitiveSelector& get_selector() const { return m_selector; }
	//gobj::PrimitiveSelector& get_selector() { return m_selector; }
	void apply_model_transform();


private:

	std::string m_name;

	std::shared_ptr<TetMesh> m_mesh;
	ShaderProgram m_mesh_draw_program;

	gobj::Transform m_transform;
	gobj::ElasticSim m_sim;
	//std::list<std::unique_ptr<gobj::Addon>> m_addons;
	
	// Serialization
	template<typename Archive>
	void serialize(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS

};
