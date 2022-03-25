#pragma once

#include "Addon.hpp"
#include "meshes/TetMesh.hpp"

#include <map>
#include <memory>
#include <list>
#include <glm/glm.hpp>

namespace gobj {

class Selection;
struct Delta;
class PrimitiveSelector final : public Addon {
public:
	void render_ui(const Context& ctx, GameObject* parent) override final;
	void update(const Context& ctx, GameObject* parent) override final;
	const char* get_name() const override final;

	const std::map<uint32_t, Delta>& get_movements() const { return m_node_movements; }

private:
	std::weak_ptr<TetMesh> m_mesh;
	std::list<Selection> m_selections;
	std::map<uint32_t, Delta> m_node_movements;

	friend class Selection;
}; // class PrimitiveSelector

struct Delta {
	glm::vec3 delta = glm::vec3(0.0f);
};

class Selection {
public:

	void render_ui(const Context& ctx, GameObject* parent, PrimitiveSelector* selector);

private:
	uint32_t m_vertex = 0;
};

} // namespace gobj