#pragma once

#include <glm/glm.hpp>

#include "physics/Primitive.hpp"


class Plane final : public physics::Primitive {
public:
	Plane() = default;

	Plane(const glm::vec3& p, const glm::vec3& n) :
		m_normal(n),
		m_point(p),
		m_displacement(glm::dot(p, n)) {}
	Plane(const glm::vec3& p, const glm::vec3& n, 
		const glm::vec3& right,
		const glm::vec3& cross) :
		m_normal(n),
		m_right(right),
		m_cross(cross),
		m_point(p),
		m_displacement(glm::dot(p, n)) {}

	Plane(const glm::vec3& p, const glm::vec3& n,
		const glm::vec3& right,
		const glm::vec3& cross,
		const glm::vec2& max_uvs) :
		m_normal(n),
		m_right(right),
		m_cross(cross),
		m_max_uvs(max_uvs),
		m_point(p),
		m_displacement(glm::dot(p, n)) {}

	float distance(const glm::vec3& p) const override final;
	std::optional<SurfaceIntersection> intersect(const Ray& ray, const float max_t) const override final;
	BBox world_bbox() const override final;
	void draw_ui() override final;
private:
	glm::vec3 m_normal = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 m_right = glm::vec3(0.0f);
	glm::vec3 m_cross = glm::vec3(0.0f);
	glm::vec2 m_max_uvs = glm::vec2(std::numeric_limits<float>::infinity());
	glm::vec3 m_point = glm::vec3(0.0f);;
	float m_displacement = 0.0f;
};
