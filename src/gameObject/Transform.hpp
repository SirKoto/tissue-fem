#pragma once

#include <glm/glm.hpp>


class Context;
namespace gobj {

class Transform {
public:

	void draw_ui(const Context& gc);

	const glm::mat4& mat4() const { return m_transform; }
	const glm::mat4& inverse() const { return m_inverse; }

	void scale(float k);
	void rotate(const glm::vec3& axis, float rad);
	void translate(const glm::vec3& v);

	void set_identity() { m_transform = m_inverse = glm::mat4(1.0f); }

private:
	glm::mat4 m_transform = glm::mat4(1.0f);
	glm::mat4 m_inverse = glm::mat4(1.0f);
};

} // namespace gobj