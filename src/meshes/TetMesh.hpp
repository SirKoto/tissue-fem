#pragma once

#include <vector>
#include <Eigen/Dense>
#include <filesystem>

class TetMesh
{
public:

	TetMesh() {}

	TetMesh(const TetMesh&&);
	TetMesh(const TetMesh&) = delete;
	TetMesh& operator=(const TetMesh&) = delete;

	bool load_tetgen(std::filesystem::path path, std::string* out_err);

	void clear();

private:

	std::vector<Eigen::Vector3f> m_vertices;
	std::vector<Eigen::Vector3i> m_surface_faces;
	std::vector<Eigen::Vector3f> m_surface_vertices_normals;
	std::vector<Eigen::Vector4i> m_elements;
};