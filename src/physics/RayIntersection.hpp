#pragma once

#include <glm/glm.hpp>

struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;
};

struct SurfaceIntersection {
	glm::vec3 point;
	glm::vec3 normal;
};