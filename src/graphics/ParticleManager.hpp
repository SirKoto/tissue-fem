#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

#include "ShaderProgram.hpp"

class Context;
class GameObject;
class ParticleManager {
public:
	ParticleManager();

	ParticleManager(const ParticleManager&) = delete;
	ParticleManager(ParticleManager&&) noexcept;
	ParticleManager& operator=(const ParticleManager&) = delete;

	~ParticleManager();

	void set_particles(const std::vector<glm::vec3>& particles);

	void draw(const Context& ctx, const GameObject& obj) const;

	const float& particle_radius() const { return m_radius; }
	float& particle_radius() { return m_radius; }

private:
	uint32_t m_num_particles = 0;
	float m_radius = 0.01f;

	uint32_t m_triangles_to_draw;

	uint32_t m_VAO_particles = 0;
	uint32_t m_VBO_particles = 0;
	uint32_t m_VBO_icosphere = 0;
	ShaderProgram m_shader;
};