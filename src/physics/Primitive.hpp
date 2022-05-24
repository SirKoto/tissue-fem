#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <memory>

#include "RayIntersection.hpp"
#include "utils/BBox.hpp"

class GameObject;
namespace physics {

struct Projection {
	float u, v;
	float z;
};

class Primitive {
public:
	Primitive() = default;
	Primitive(const Primitive&) = delete;
	Primitive& operator=(const Primitive&) = delete;

	virtual float distance(const glm::vec3& p) const = 0;
	virtual std::optional<SurfaceIntersection> intersect(const Ray& ray, const float max_t) const = 0;
	virtual BBox world_bbox() const = 0;
	virtual Projection plane_project(const glm::vec3& p) const = 0;
	virtual void draw_ui() = 0;
};

}