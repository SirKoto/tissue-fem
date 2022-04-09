#pragma once

#include "meshes/TetMesh.hpp"
#include "graphics/ParticleManager.hpp"

#include <map>
#include <memory>
#include <list>
#include <glm/glm.hpp>

class GameObject;
namespace gobj {

class PrimitiveSelector {
private:
	class Selection;
public:
	PrimitiveSelector();
	PrimitiveSelector(const PrimitiveSelector&) = delete;
	PrimitiveSelector(PrimitiveSelector&&) noexcept = default;
	PrimitiveSelector& operator=(const PrimitiveSelector&) = delete;
	PrimitiveSelector& operator=(PrimitiveSelector&&) = default;

	void render_ui(const Context& ctx, GameObject* parent);
	void render(const Context& ctx, const GameObject& parent) const;
	void update(const Context& ctx, GameObject* parent);
	void late_update(const Context& ctx, GameObject* parent);

	struct Delta;
	const std::map<uint32_t, Delta>& get_movements() const { return m_node_movements; }


	struct Delta {
		glm::vec3 delta = glm::vec3(0.0f);
	};


private:
	std::weak_ptr<TetMesh> m_mesh;
	std::list<Selection> m_selections;
	std::map<uint32_t, Delta> m_node_movements;

	ParticleManager m_particle_manager;

	class Selection {
	public:
		Selection();
		void render_ui(const Context& ctx, GameObject* parent, PrimitiveSelector* selector);
		void update(const Context& ctx, GameObject* parent, PrimitiveSelector* selector);

		const std::list<uint32_t>& nodes() const { return m_nodes; }
	private:
		std::list<uint32_t> m_nodes;
		bool m_fixed = true;
	};

	friend class Selection;
}; // class PrimitiveSelector



} // namespace gobj