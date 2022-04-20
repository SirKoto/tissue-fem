#pragma once

#include <glm/glm.hpp>
#include <numeric>
#include <optional>

struct Ray;
class BBox {
public:

	void add_point(const glm::vec3& p);
	BBox union_bbox(const BBox& o) const;
	BBox intersect_bbox(const BBox& o) const;

	/**
	* Intersect ray with bbox
	* Returns the 2 intersections of the ray with the box
	* If the origin of the ray is inside the box, the first hit is 0
	* If the end of the ray is inside the bbox, the second hit is t_max
	*/
	std::optional<std::pair<float, float>> intersect(const Ray& ray, const float t_max) const;

	const glm::vec3& min() const { return m_min; }
	const glm::vec3& max() const { return m_max; }

	static BBox infiniy();

private:
	glm::vec3 m_min = glm::vec3(+std::numeric_limits<float>::infinity());
	glm::vec3 m_max = glm::vec3(-std::numeric_limits<float>::infinity());
};