#include "ShaderProgram.hpp"
#include <glad/glad.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

ShaderProgram::ShaderProgram(const Shader* shaders, uint32_t num)
{
	m_id = glCreateProgram();
	for (uint32_t i = 0; i < num; ++i) {
		glAttachShader(m_id, shaders[i].get_id());
	}

	glLinkProgram(m_id);

	int32_t success;
	glGetProgramiv(m_id, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetProgramInfoLog(m_id, 512, NULL, infoLog);
		std::cerr << "ERROR SHADER PROGRAM LINKING_FAILED\n" << infoLog << std::endl;
		exit(2);
	}
}

ShaderProgram::ShaderProgram(ShaderProgram&& o)
{
	m_id = o.m_id;
	o.m_id = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& o)
{
	if (m_id != 0) {
		glDeleteProgram(m_id);
	}

	m_id = o.m_id;
	o.m_id = 0;

	return *this;
}

void ShaderProgram::use_program() const
{
	glUseProgram(m_id);
}

ShaderProgram::~ShaderProgram()
{
	if (m_id != 0) {
		glDeleteProgram(m_id);
	}
}

void ShaderProgram::setUniform(uint32_t location, const glm::mat4& mat)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderProgram::setUniform(uint32_t location, const glm::mat3& mat)
{
	glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}

void ShaderProgram::setUniform(uint32_t location, const glm::vec3& v)
{
	glUniform3fv(location, 1, glm::value_ptr(v));
}

void ShaderProgram::setUniform(uint32_t location, const float f)
{
	glUniform1f(location, f);
}
