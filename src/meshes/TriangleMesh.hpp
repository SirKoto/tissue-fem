#pragma once

#include <glm/glm.hpp>
#include <vector>

class TriangleMesh {
public:

	TriangleMesh();

	TriangleMesh(TriangleMesh&& o) noexcept;
	TriangleMesh(const TriangleMesh&) = delete;
	TriangleMesh& operator=(const TriangleMesh&) = delete;

	~TriangleMesh();

	void update();

	void set_mesh(const std::vector<glm::vec3>& vertices, const std::vector<glm::ivec3>& faces);

	void draw_triangles() const;

	void regenerate_normals();

	void flip_face_orientation();

	void update_vert(size_t idx, const glm::vec3& v);


	// This must be set before loading the mesh
	void set_mesh_dynamic(bool is_dynamic) { m_dynamic_mesh = is_dynamic; }

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 norm;
	};

	const std::vector<Vertex>& vertices() const { return m_vertices; }

private:

	

	std::vector<Vertex> m_vertices;
	std::vector<glm::ivec3> m_faces;
	uint32_t m_vao;
	union
	{
		struct {
			uint32_t m_vbo_vertices;
			uint32_t m_vbo_indices;
		};
		uint32_t m_vbos[2];
	};

	bool m_needs_update = false;
	bool m_dynamic_mesh = false;

	void gl_bind_to_vao() const;

	void upload_vertices_to_gpu() const;
	void upload_faces_to_gpu() const;
	void initialize_gpu_buffers() const;
};