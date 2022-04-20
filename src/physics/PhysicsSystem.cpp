#include "PhysicsSystem.hpp"

PhysicsSystem::PhysicEntityId PhysicsSystem::insert_primitive(const PrimitivePtr& primitive)
{
    return m_primitives.insert(m_primitives.end(), primitive);
}

PhysicsSystem::PhysicEntityId PhysicsSystem::insert_primitive(PrimitivePtr&& primitive)
{
    return m_primitives.insert(m_primitives.end(), std::move(primitive));
}

void PhysicsSystem::erase_primitive(const PhysicEntityId& id)
{
    m_primitives.erase(id);
}

std::optional<SurfaceIntersection> PhysicsSystem::intersect(const Ray& ray, const float max_t) const
{
    std::optional<SurfaceIntersection> result;
    // Find any intersection
    for (const PrimitivePtr& primitive : m_primitives) {
        result = primitive->intersect(ray, max_t);
        if (result.has_value()) {
            break;
        }
    }

    return result;
}
