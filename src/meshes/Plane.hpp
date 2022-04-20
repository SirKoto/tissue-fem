#pragma once

#include <glm/glm.hpp>

#include "physics/Primitive.hpp"

namespace physics {

class Plane final : public Primitive {
public:
	Plane(const GameObject* parent) : Primitive(parent) {}
	Plane(const GameObject* parent, const glm::vec3& p, const glm::vec3& n) :
		Primitive(parent), m_normal(n), m_displacement(glm::dot(p, n)) {}

	std::optional<SurfaceIntersection> intersect(const Ray& ray, const float max_t) const override final;
	BBox world_bbox() const override final;
	void draw_ui() override final;
private:
	glm::vec3 m_normal = glm::vec3(0.0f, 1.0f, 0.0f);
	float m_displacement = 0.0f;
};

}; // namespace physics