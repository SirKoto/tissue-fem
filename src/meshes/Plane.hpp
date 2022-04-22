#pragma once

#include <glm/glm.hpp>

#include "physics/Primitive.hpp"


class Plane final : public physics::Primitive {
public:
	Plane(const glm::vec3& p, const glm::vec3& n) : m_normal(n), m_displacement(glm::dot(p, n)) {}

	std::optional<SurfaceIntersection> intersect(const Ray& ray, const float max_t) const override final;
	BBox world_bbox() const override final;
	void draw_ui() override final;
private:
	glm::vec3 m_normal = glm::vec3(0.0f, 1.0f, 0.0f);
	float m_displacement = 0.0f;
};
