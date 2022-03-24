#pragma once

#include "Addon.hpp"
#include "meshes/TetMesh.hpp"

#include <memory>
#include <list>
#include <glm/glm.hpp>

namespace gobj {

class Selection;
class PrimitiveSelector final : public Addon {
public:
	void render_ui(const Context& ctx, GameObject* parent) override final;
	void update(const Context& ctx, GameObject* parent) override final;
	const char* get_name() const override final;

private:
	std::weak_ptr<TetMesh> m_mesh;
	std::list<Selection> m_selections;

	friend class Selection;
}; // class PrimitiveSelector

class Selection {
public:

	void render_ui(const Context& ctx, GameObject* parent, PrimitiveSelector* selector);

private:
	uint32_t m_vertex = 0;
};

} // namespace gobj