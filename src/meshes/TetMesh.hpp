#pragma once

#include <vector>
#include <Eigen/Dense>
#include <glm/glm.hpp>
#include <filesystem>
#include <map>

#include "utils/serialization.hpp"
#include "TriangleMesh.hpp"

class TetMesh
{
public:


	bool load_tetgen(std::filesystem::path path, std::string* out_err = nullptr);

	void apply_transform(const glm::mat4& m);

	void flip_face_orientation();

	void generate_normals();

	void draw_triangles() const;

	void update();

	const std::vector<Eigen::Vector3f>& nodes() const { return reinterpret_cast<const std::vector<Eigen::Vector3f>&>(m_vertices); }
	const std::vector<glm::vec3>& nodes_glm() const { return reinterpret_cast<const std::vector<glm::vec3>&>(m_vertices); }
	const std::vector<Eigen::Vector4i>& elements() const { return reinterpret_cast<const std::vector<Eigen::Vector4i>&>(m_elements); }
	const std::map<int32_t, int32_t>& global_to_local_surface_vertices() const { return m_global_to_local_surface_vertex; }
	void update_node(int32_t idx, const glm::vec3& pos);

	inline void update_node(int32_t idx, const Eigen::Vector3f& pos) {
		this->update_node(idx, reinterpret_cast<const glm::vec3&>(pos));
	}


private:

	std::filesystem::path m_path;
	std::vector<glm::vec3> m_vertices;
	std::vector<glm::ivec4> m_elements;

	std::map<int32_t, int32_t> m_global_to_local_surface_vertex;

	TriangleMesh m_tri_mesh;
	bool m_flip_face_orientation_on_load = false;

	void create_surface_faces(std::vector<glm::ivec3>&& faces);

	// Serialization
	template<class Archive>
	void save(Archive& archive) const;

	template<class Archive>
	void load(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS
};