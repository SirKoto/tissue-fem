#pragma once

#include "Shader.hpp"
#include <glm/glm.hpp>

class ShaderProgram {
public:
	ShaderProgram() = default;
	ShaderProgram(const Shader* shaders, uint32_t num);
	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram(ShaderProgram&&);
	ShaderProgram& operator=(const ShaderProgram&) = delete;
	ShaderProgram& operator=(ShaderProgram&&);

	void use_program() const;

	~ShaderProgram();

	static void setUniform(uint32_t location, const glm::mat4& mat);
	static void setUniform(uint32_t location, const glm::mat3& mat);
	static void setUniform(uint32_t location, const glm::vec3& v);
	static void setUniform(uint32_t location, const float f);

private:
	uint32_t m_id = 0;
};