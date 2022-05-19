#pragma once

#include "GameObject.hpp"
#include "meshes/TetMesh.hpp"
#include "gameObject/extra/PrimitiveSelector.hpp"

class SimulatedGameObject final : public GameObject {
public:
	SimulatedGameObject();

	void start_simulation(Context& ctx) override final;

	void render(const Context& ctx) const override final;

	void render_ui(const Context& gc) override final;

	void update(const Context& gc) override final;
	void late_update(const Context& gc) override final;

	bool load_tetgen(const std::filesystem::path& path, std::string* out_err = nullptr);

	const std::shared_ptr<TetMesh>& get_mesh() const { return m_mesh; }
	std::shared_ptr<TetMesh>& get_mesh() { return m_mesh; }
	const gobj::PrimitiveSelector& get_selector() const { return m_selector; }

private:


	std::shared_ptr<TetMesh> m_mesh;
	ShaderProgram m_mesh_draw_program;
	gobj::PrimitiveSelector m_selector;

	// Serialization
	template<typename Archive>
	void serialize(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS
};
