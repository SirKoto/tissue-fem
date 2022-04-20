#include "BBox.hpp"

#include "physics/Primitive.hpp"

void BBox::add_point(const glm::vec3& p)
{
	m_min = glm::min(m_min, p);
	m_max = glm::max(m_max, p);
}

BBox BBox::union_bbox(const BBox& o) const
{
	BBox box;
	box.m_min = glm::min(this->m_min, o.m_min);
	box.m_max = glm::max(this->m_max, o.m_max);
	return box;
}

BBox BBox::intersect_bbox(const BBox& o) const
{
	BBox box;
	box.m_min = glm::max(this->m_min, o.m_min);
	box.m_max = glm::min(this->m_max, o.m_max);
	return box;
}

std::optional<std::pair<float, float>> 
BBox::intersect(const Ray& ray, const float t_max) const
{
	float t0 = 0.0f;
	float t1 = t_max;

	// intersect with the 3 different slabs
	for (uint32_t i = 0; i < 3; ++i) {
		const float inv_ray_dir = 1.0f / ray.direction[i];
		float t_near = (m_min[i] - ray.origin[i]) * inv_ray_dir;
		float t_far = (m_max[i] - ray.origin[i]) * inv_ray_dir;
		if (t_near > t_far) {
			std::swap(t_near, t_far);
		}

		// TODO: update t_far for robustness
		t0 = std::max(t0, t_near);
		t1 = std::min(t1, t_far);

		if (t0 > t1) {
			return {}; // return empty
		}

	}

	// Return intersection
	return { {t0, t1} };
}

BBox BBox::infiniy()
{
	BBox box;
	box.m_min = glm::vec3(-std::numeric_limits<float>::infinity());
	box.m_max = glm::vec3(+std::numeric_limits<float>::infinity());
	return box;
}
