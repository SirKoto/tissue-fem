#pragma once

#include <cstdint>
#include <list>
#include <memory>

#include "Primitive.hpp"
#include "RayIntersection.hpp"

class PhysicsSystem {
public:
	PhysicsSystem() {};
	PhysicsSystem(const PhysicsSystem&) = delete;
	PhysicsSystem(PhysicsSystem&&) = default;
	PhysicsSystem& operator=(const PhysicsSystem&) = delete;

	typedef std::shared_ptr<const physics::Primitive> PrimitivePtr;
	typedef std::list<PrimitivePtr>::const_iterator PhysicEntityId;

	PhysicEntityId insert_primitive(const PrimitivePtr& primitive);
	PhysicEntityId insert_primitive(PrimitivePtr&& primitive);

	void erase_primitive(const PhysicEntityId& id);

	std::optional<SurfaceIntersection> intersect(const Ray& ray, const float max_t) const;

private:

	std::list<PrimitivePtr> m_primitives;
};