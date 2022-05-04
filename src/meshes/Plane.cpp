#include "Plane.hpp"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

std::optional<SurfaceIntersection> Plane::intersect(const Ray& ray, const float max_t) const
{
	float cos = glm::dot(m_normal, ray.direction);

	float t = -(glm::dot(ray.origin, m_normal) + m_displacement) / cos;
	
	if (t > max_t || t <= 0.0f) {
		return {};
	}

	SurfaceIntersection inter;
	inter.point = ray.origin + t * ray.direction;
	inter.normal = m_normal;

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
