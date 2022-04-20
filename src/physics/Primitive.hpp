#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <memory>

#include "RayIntersection.hpp"
#include "utils/BBox.hpp"

class GameObject;
namespace physics {

class Primitive {
public:

	Primitive(const GameObject* parent) : m_obj(parent) {}
	Primitive(const Primitive&) = delete;
	Primitive& operator=(const Primitive&) = delete;

	virtual std::optional<SurfaceIntersection> intersect(const Ray& ray, const float max_t) const = 0;
	virtual BBox world_bbox() const = 0;
	virtual void draw_ui() = 0;

protected:
	const GameObject* m_obj;
};

}