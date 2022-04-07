#include "TriangleMesh.hpp"

#include <glad/glad.h>

TriangleMesh::TriangleMesh()
{
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
	glBindVertexArray(m_vao);
	this->gl_bind_to_vao();
	glBindVertexArray(0);
}

TriangleMesh::TriangleMesh(TriangleMesh&& o) noexcept
{
	m_vertices = std::move(o.m_vertices);
	m_faces = std::move(o.m_faces);

	m_vao = o.m_vao;
	o.m_vao = 0;
	std::memcpy(m_vbos, o.m_vbos, sizeof(m_vbos));
	std::memset(o.m_vbos, 0, sizeof(m_vbos));
}

TriangleMesh::~TriangleMesh()
{
	if (m_vao != 0) {
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
	}
}

void TriangleMesh::update()
{
	if (m_needs_update) {
		m_needs_update = false;
		this->upload_vertices_to_gpu();
	}
}

void TriangleMesh::set_mesh(const std::vector<glm::vec3>& vertices, const std::vector<glm::ivec3>& faces)
{
	m_vertices.resize(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i) {
		m_vertices[i].pos = vertices[i];
	}

	m_faces = faces;

	this->regenerate_normals();

	this->initialize_gpu_buffers();
}

void TriangleMesh::draw_triangles() const
{
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES,
		3 * (GLsizei)m_faces.size(),
		GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
}

void TriangleMesh::regenerate_normals()
{

	// Compute the planes of all triangles
	std::vector<glm::vec3> triangleNormals(m_faces.size());
	for (uint32_t t = 0; t < (uint32_t)m_faces.size(); ++t) {
		const glm::vec3& v0 = m_vertices[m_faces[t][0]].pos;
		const glm::vec3& v1 = m_vertices[m_faces[t][1]].pos;
		const glm::vec3& v2 = m_vertices[m_faces[t][2]].pos;

		glm::vec3 n = glm::cross(v1 - v0, v2 - v0);
		triangleNormals[t] = glm::normalize(n);
	}

	// Compute V:{F}
	std::vector<std::vector<uint32_t>> vert2faces(m_vertices.size());
	std::vector<uint32_t> vertexArity(m_vertices.size(), 0);
	for (uint32_t t = 0; t < (uint32_t)m_faces.size(); ++t) {
		vertexArity[m_faces[t][0]] += 1;
		vertexArity[m_faces[t][1]] += 1;
		vertexArity[m_faces[t][2]] += 1;
	}
	for (uint32_t v = 0; v < (uint32_t)m_vertices.size(); ++v) {
		vert2faces[v].reserve(vertexArity[v]);
	}
	for (uint32_t t = 0; t < (uint32_t)m_faces.size(); ++t) {
		vert2faces[m_faces[t][0]].push_back(t);
		vert2faces[m_faces[t][1]].push_back(t);
		vert2faces[m_faces[t][2]].push_back(t);
	}

	for (uint32_t v = 0; v < (uint32_t)m_vertices.size(); ++v) {
		m_vertices[v].norm = glm::vec3(0.0f);

		for (const uint32_t& f : vert2faces[v]) {
			m_vertices[v].norm += triangleNormals[f];
		}
		// TODO: compute weighted average normals
		m_vertices[v].norm /= (float)vert2faces[v].size();
	}

	m_needs_update = true;
}

void TriangleMesh::flip_face_orientation()
{
	for (glm::ivec3& face : m_faces) {
		std::swap(face[0], face[1]);
	}

	for (Vertex& n : m_vertices) {
		n.norm = -n.norm;
	}

	this->upload_vertices_to_gpu();
	this->upload_faces_to_gpu();
}

void TriangleMesh::update_vert(size_t idx, const glm::vec3& v)
{
	m_vertices[idx].pos = v;
	m_needs_update = true;
}

void TriangleMesh::gl_bind_to_vao() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertices);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, norm));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_indices);
}

void TriangleMesh::upload_vertices_to_gpu() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertices);
	glBufferSubData(GL_ARRAY_BUFFER,
		0, // offset
		m_vertices.size() * sizeof(m_vertices[0]), // size
		m_vertices.data()); // *data

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void TriangleMesh::upload_faces_to_gpu() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_indices);

	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
		0, // offset
		m_faces.size() * sizeof(m_faces[0]),
		m_faces.data());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TriangleMesh::initialize_gpu_buffers() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER,
		m_vertices.size() * sizeof(m_vertices[0]),
		m_vertices.data(),
		m_dynamic_mesh ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_indices);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		m_faces.size() * sizeof(m_faces[0]),
		m_faces.data(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
