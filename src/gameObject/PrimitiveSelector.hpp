#pragma once

#include "Addon.hpp"
#include "meshes/TetMesh.hpp"

#include <map>
#include <memory>
#include <list>
#include <glm/glm.hpp>

namespace gobj {


class PrimitiveSelector final : public Addon {
private:
	class Selection;
public:
	void render_ui(const Context& ctx, GameObject* parent) override final;
	void update(const Context& ctx, GameObject* parent) override final;
	void late_update(const Context& ctx, GameObject* parent) override final;
	const char* get_name() const override final;

	struct Delta;
	const std::map<uint32_t, Delta>& get_movements() const { return m_node_movements; }


	struct Delta {
		glm::vec3 delta = glm::vec3(0.0f);
	};


private:
	std::weak_ptr<TetMesh> m_mesh;
	std::list<Selection> m_selections;
	std::map<uint32_t, Delta> m_node_movements;

	class Selection {
	public:
		Selection();
		void render_ui(const Context& ctx, GameObject* parent, PrimitiveSelector* selector);
		void update(const Context& ctx, GameObject* parent, PrimitiveSelector* selector);

	private:
		std::list<uint32_t> m_nodes;
		bool m_fixed = true;
	};

	friend class Selection;
}; // class PrimitiveSelector



} // namespace gobj