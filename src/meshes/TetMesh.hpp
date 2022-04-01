#pragma once

#include <vector>
#include <Eigen/Dense>
#include <glm/glm.hpp>
#include <filesystem>
#include <map>

class TetMesh
{
public:

	TetMesh();

	TetMesh(TetMesh&&);
	TetMesh(const TetMesh&) = delete;
	TetMesh& operator=(const TetMesh&) = delete;

	~TetMesh();

	bool load_tetgen(std::filesystem::path path, std::string* out_err = nullptr);

	void upload_to_gpu(bool dynamic_verts = false, bool dynamic_indices = false) const;

	void apply_transform(const glm::mat4& m);

	void flip_face_orientation();

	void generate_normals();

	void draw_triangles() const;

	void clear();

	const std::vector<Eigen::Vector3f>& nodes() const { return m_vertices; }
	const std::vector<glm::vec3>& nodes_glm() const { return reinterpret_cast<const std::vector<glm::vec3>&>(m_vertices); }
	const std::vector<Eigen::Vector4i>& elements() const { return m_elements; }
	const std::vector<Eigen::Vector3i>& surface_faces() const { return m_surface_faces; }

	void update_node(int32_t idx, const Eigen::Vector3f& pos);

private:

	std::vector<Eigen::Vector3f> m_vertices;
	std::vector<Eigen::Vector3f> m_surface_vertices;
	std::vector<Eigen::Vector3i> m_surface_faces;
	std::vector<Eigen::Vector3f> m_surface_vertices_normals;
	std::vector<Eigen::Vector4i> m_elements;

	std::map<int32_t, int32_t> m_global_to_local_surface_vertex;

	uint32_t m_vao;
	union
	{
		struct {
			uint32_t m_vbo_vertices;
			uint32_t m_vbo_indices;
			uint32_t m_vbo_normals;
		};
		uint32_t m_vbos[3];
	};

	void gl_bind_to_vao() const;

	void create_surface_faces();
};