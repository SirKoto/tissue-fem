#pragma once

#include <vector>
#include <Eigen/Dense>
#include <filesystem>

class TetMesh
{
public:

	TetMesh();

	TetMesh(TetMesh&&);
	TetMesh(const TetMesh&) = delete;
	TetMesh& operator=(const TetMesh&) = delete;

	~TetMesh();

	bool load_tetgen(std::filesystem::path path, std::string* out_err);

	void upload_to_gpu(bool dynamic_verts = false, bool dynamic_indices = false) const;

	void draw_triangles() const;

	void clear();

private:

	std::vector<Eigen::Vector3f> m_vertices;
	std::vector<Eigen::Vector3i> m_surface_faces;
	std::vector<Eigen::Vector3f> m_surface_vertices_normals;
	std::vector<Eigen::Vector4i> m_elements;

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

	void generate_normals();
	void gl_bind_to_vao() const;
};