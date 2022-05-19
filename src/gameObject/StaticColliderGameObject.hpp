#pragma once

#include "GameObject.hpp"
#include "meshes/TriangleMesh.hpp"
#include "meshes/Plane.hpp"
#include "graphics/ShaderProgram.hpp"

class StaticColliderGameObject final : public GameObject {
public:
	StaticColliderGameObject();

	void start_simulation(Context& ctx) override final;

	void render(const Context& ctx) const override final;

	void render_ui(const Context& gc) override final;

	void update(const Context& gc) override final;


private:

	TriangleMesh m_mesh;
	ShaderProgram m_mesh_draw_program;

	glm::vec3 m_color = glm::vec3(.5f, 0.2f, 0.8f);

	// Serialization
	template<typename Archive>
	void serialize(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS
};