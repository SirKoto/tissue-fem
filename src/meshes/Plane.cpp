#undef NDEBUG
#include "Plane.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

std::optional<SurfaceIntersection> Plane::intersect(const Ray& ray, const float max_t) const
{
	float cos = glm::dot(m_normal, ray.direction);
	if (std::abs(cos) < std::nextafter(0.0f, 1.0f)) {
		return {};
	}

	float t = -(glm::dot(ray.origin, m_normal) - m_displacement) / cos;
	
	if (t > max_t || t <= 0.0f) {
		return {};
	}

	SurfaceIntersection inter;
	inter.point = ray.origin + t * ray.direction;
	inter.normal = m_normal;
	inter.primitive = this;

	if (m_max_uvs[0] != std::numeric_limits<float>::infinity()) {
		if (std::abs(glm::dot(m_right, inter.point - m_point)) > m_max_uvs[0]) {
			return {};
		}
	}

	if (m_max_uvs[1] != std::numeric_limits<float>::infinity()) {
		if (std::abs(glm::dot(m_cross, inter.point - m_point)) > m_max_uvs[1]) {
			return {};
		}
	}

	return { inter };
}

BBox Plane::world_bbox() const
{
	if (m_max_uvs[0] == std::numeric_limits<float>::infinity() ||
		m_max_uvs[1] == std::numeric_limits<float>::infinity()) {
		return BBox::infiniy();
	}

	BBox box;
	for (float i = -1.0f; i <= 1.0f; i += 2.0f) {
		for (float j = -1.0f; j <= 1.0f; j += 2.0f) {
			box.add_point(m_point + i * m_right * m_max_uvs[0] + j * m_cross * m_max_uvs[1]);
		}
	}

	return box;
}

void Plane::draw_ui()
{
	ImGui::PushID(this);
	ImGui::TextDisabled("Plane");
	ImGui::InputFloat3("Dormal", glm::value_ptr(m_normal));
	ImGui::InputFloat("Displacement", &m_displacement);
	ImGui::PopID();
}

float Plane::distance(const glm::vec3& p) const
{
	// Project point into the plane
	float d_n = glm::dot(m_normal, p - m_point);
	float d_r = glm::dot(m_right, p - m_point);
	float d_c = glm::dot(m_cross, p - m_point);
	
	if (std::abs(d_r) < m_max_uvs[0] && std::abs(d_c) < m_max_uvs[1]) {
		return std::abs(d_n);
	}
	else {
		d_r = d_r > 0.0f ? std::min(d_r, m_max_uvs[0]) : std::max(d_r, -m_max_uvs[0]);
		d_c = d_c > 0.0f ? std::min(d_c, m_max_uvs[1]) : std::max(d_c, -m_max_uvs[1]);
		// Compute closest point
		glm::vec3 p_in_plane = m_point + m_right * d_r + m_cross * d_c;

		return glm::distance(p, p_in_plane);
	}
	
}
