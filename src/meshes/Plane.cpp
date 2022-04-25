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

	return { inter };
}

BBox Plane::world_bbox() const
{
	return BBox::infiniy();
}

void Plane::draw_ui()
{
	ImGui::PushID(this);
	ImGui::TextDisabled("Plane");
	ImGui::InputFloat3("Dormal", glm::value_ptr(m_normal));
	ImGui::InputFloat("Displacement", &m_displacement);
	ImGui::PopID();
}
