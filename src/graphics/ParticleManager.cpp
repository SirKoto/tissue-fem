#include "ParticleManager.hpp"

#include <glad/glad.h>
#include <array>

#include "utils/Icosphere.hpp"
#include "Context.hpp"
#include "GameObject.hpp"

ParticleManager::ParticleManager()
{
	glGenVertexArrays(1, &m_VAO_particles);
	glGenBuffers(1, &m_VBO_particles);
	glGenBuffers(1, &m_VBO_icosphere);

	// Ico has vertices and normals
	std::vector<float> ico = icosphere::genIcosphere();

	glBindVertexArray(m_VAO_particles);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_icosphere);
	// buffer icosphere static data
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * ico.size(), ico.data(), GL_STATIC_DRAW);
	m_triangles_to_draw = static_cast<int>(ico.size()) / (3 * 2);

	// Location 0 = positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Location 1 = normals
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Buffer particle positions
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_particles);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	const std::filesystem::path proj_dir(PROJECT_DIR);
	const std::filesystem::path shad_dir = proj_dir / "resources/shaders";

	std::array<Shader, 2> shaders = {
		Shader((shad_dir / "particles.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "particles.frag"), Shader::Type::Fragment)
	};
	m_shader = ShaderProgram(shaders.data(), (uint32_t)shaders.size());
}

ParticleManager::ParticleManager(ParticleManager&& o) noexcept
{
	o.m_num_particles = o.m_num_particles;
	m_radius = o.m_radius;

	m_VAO_particles = o.m_VAO_particles;
	m_VBO_particles = o.m_VBO_particles;
	m_VBO_icosphere = o.m_VBO_icosphere;

	m_triangles_to_draw = o.m_triangles_to_draw;
	m_shader = std::move(o.m_shader);

	o.m_VAO_particles = 0;
}

ParticleManager::~ParticleManager()
{
	if (m_VAO_particles != 0) {
		glDeleteBuffers(1, &m_VBO_particles);
		glDeleteBuffers(1, &m_VBO_icosphere);
		glDeleteVertexArrays(1, &m_VAO_particles);

		m_VAO_particles = 0;
	}
}

void ParticleManager::set_particles(const std::vector<glm::vec3>& particles)
{
	m_num_particles = (uint32_t)particles.size();

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_particles);
	glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(glm::vec3),
		particles.data(), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleManager::draw(const Context& ctx, const glm::mat4& model_matrix) const
{
	m_shader.use_program();
	assert(glGetError() == GL_NO_ERROR);
	// Set uniforms
	glm::mat3 inv_t = glm::transpose(glm::inverse(glm::mat3(model_matrix)));
	ShaderProgram::setUniform(0, model_matrix);
	ShaderProgram::setUniform(1, inv_t);
	ShaderProgram::setUniform(2, ctx.camera().getProjView());
	ShaderProgram::setUniform(3, m_radius);


	// Draw
	glBindVertexArray(m_VAO_particles);
	glDrawArraysInstanced(GL_TRIANGLES, 0,
		m_triangles_to_draw,
		static_cast<GLsizei>(m_num_particles));

	glBindVertexArray(0);
}
