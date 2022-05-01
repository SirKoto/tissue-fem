#include "TetMesh.hpp"

#include <fstream>
#include <glad/glad.h>


bool TetMesh::load_tetgen(std::filesystem::path path, std::string* out_err)
{
	assert(path.is_absolute());
	m_vertices.clear();
	m_elements.clear();
	std::vector<glm::ivec3> surface_faces;

	auto err_out = [&](const std::string& msg) {
		if (out_err != nullptr) {
			*out_err = "Error loading " + path.string() + ": " + msg;
		}
	};

	namespace fs = std::filesystem;

	if (!fs::exists(path)) {
		err_out("Does not exist!");
		return false;
	}

	if (!path.has_filename()) {
		err_out("Does not have a filename!");
		return false;
	}

	fs::path elements_path = path.replace_extension("ele");
	fs::path nodes_path = path.replace_extension("node");
	fs::path surfaces_path = path.replace_extension("face");

	if (!fs::exists(elements_path)) {
		err_out(".ele file does not exists");
		return false;
	}
	if (!fs::exists(nodes_path)) {
		err_out(".node file does not exists");
		return false;
	}
	if (!fs::exists(surfaces_path)) {
		err_out(".face file does not exists");
		return false;
	}

	// Read nodes/vertices
	std::ifstream stream;
	{
		stream.open(nodes_path, std::ios::in);
		int32_t num_vertices, num_components;

		stream >> num_vertices >> num_components;
		if (num_vertices == 0) {
			err_out("No nodes");
			return false;
		}
		if (num_components != 3) {
			err_out("Nodes have not 3 components");
			return false;
		}

		stream.ignore(std::numeric_limits<std::streamsize>::max(), stream.widen('\n')); // skip line
		m_vertices.resize(num_vertices);
		int32_t idx;
		glm::vec3 v;
		while (num_vertices-- > 0) {
			stream >> idx >> v[0] >> v[1] >> v[2];
			m_vertices[idx] = v;
			if (stream.fail()) {
				err_out("Something went wrong when loading .node");
				return false;
			}
		}
		
	}
	stream.close();
	// Read elements
	{
		stream.open(elements_path);
		int32_t num_elements, num_components;

		stream >> num_elements >> num_components;
		if (num_elements == 0) {
			err_out("No elements");
			return false;
		}
		if (num_components != 4) {
			err_out("Elements have not 4 nodes");
			return false;
		}

		stream.ignore(std::numeric_limits<std::streamsize>::max(), stream.widen('\n')); // skip line
		m_elements.resize(num_elements);
		int32_t idx;
		while (num_elements-- > 0) {
			stream >> idx;
			stream >> m_elements[idx][0] >> m_elements[idx][1] >> m_elements[idx][2] >> m_elements[idx][3];
			if (stream.fail()) {
				err_out("Something went wrong when loading .ele");
				return false;
			}
		}
	}
	stream.close();
	// Read surface faces
	{
		stream.open(surfaces_path);

		int32_t num_faces, tmp;

		stream >> num_faces >> tmp;
		if (num_faces == 0) {
			err_out("No faces");
			return false;
		}

		surface_faces.resize(num_faces);
		int32_t idx;
		while (num_faces-- > 0) {
			stream >> idx;
			stream >> surface_faces[idx][0] >> surface_faces[idx][1] >> surface_faces[idx][2];
			if (stream.fail()) {
				err_out("Something went wrong when loading .ele");
				return false;
			}
		}
	}
	stream.close();

	this->create_surface_faces(std::move(surface_faces));
	this->generate_normals();

	// Set path to validate load
	m_path = path;

	return true;
}


void TetMesh::apply_transform(const glm::mat4& m)
{
	static_assert(sizeof(glm::vec3) == sizeof(m_vertices[0]), "Same FP type");
	std::vector<glm::vec3>& vertices = reinterpret_cast<std::vector<glm::vec3>&>(m_vertices);

	for (glm::vec3& v : vertices) {
		v = glm::vec3(m * glm::vec4(v, 1.0f));
	}

	for (const auto& it : m_global_to_local_surface_vertex) {
		m_tri_mesh.update_vert(it.second, m_vertices[it.first]);
	}
}

void TetMesh::flip_face_orientation()
{
	m_tri_mesh.flip_face_orientation();
	m_flip_face_orientation_on_load = !m_flip_face_orientation_on_load;
}

void TetMesh::draw_triangles() const
{
	m_tri_mesh.draw_triangles();
}

void TetMesh::update()
{
	m_tri_mesh.update();
}

void TetMesh::create_surface_faces(std::vector<glm::ivec3>&& faces)
{
	m_global_to_local_surface_vertex.clear();
	// Convert surface faces global indices to local
	for (glm::ivec3& face : faces) {
		for (uint32_t i = 0; i < 3; ++i) {
			const int32_t vertex = face[i];
			auto it = m_global_to_local_surface_vertex.find(vertex);
			int32_t new_id = (int32_t)m_global_to_local_surface_vertex.size();
			if (it == m_global_to_local_surface_vertex.end()) {
				m_global_to_local_surface_vertex.emplace(
					vertex, new_id);
			}
			else {
				new_id = it->second;
			}
			face[i] = new_id;
		}
	}

	std::vector<glm::vec3> surface_vertices(m_global_to_local_surface_vertex.size());
	for (const auto& it : m_global_to_local_surface_vertex) {
		surface_vertices[it.second] = m_vertices[it.first];
	}

	m_tri_mesh.set_mesh_dynamic(true);
	m_tri_mesh.set_mesh(surface_vertices, faces);
}

void TetMesh::update_node(int32_t idx, const glm::vec3& pos)
{
	m_vertices.at(idx) = pos;
	const std::map<int32_t, int32_t>::const_iterator it = 
		m_global_to_local_surface_vertex.find((int32_t)idx);

	if (it != m_global_to_local_surface_vertex.end()) {
		m_tri_mesh.update_vert(it->second, pos);
	}
}

void TetMesh::generate_normals()
{
	m_tri_mesh.regenerate_normals();
}

template<class Archive>
inline void TetMesh::save(Archive& ar) const
{
	// Save relative path
	std::filesystem::path p = std::filesystem::proximate(m_path, ar.save_path());

	ar(TF_SERIALIZE_NVP("path", p.string()));
	ar(TF_SERIALIZE_NVP_MEMBER(m_flip_face_orientation_on_load));
}

template<class Archive>
void TetMesh::load(Archive& ar)
{
	std::string path_str;
	ar(TF_SERIALIZE_NVP("path", path_str));
	ar(TF_SERIALIZE_NVP_MEMBER(m_flip_face_orientation_on_load));

	m_path = path_str;
	if (m_path.is_relative()) {
		// If it is relative, compose path with scene path
		m_path = std::filesystem::canonical(ar.save_path() / m_path);
	}


	if (!m_path.empty()) {
		bool res = this->load_tetgen(m_path);
		assert(res);
		if (m_flip_face_orientation_on_load) {
			m_tri_mesh.flip_face_orientation();
		}
	}
}

TF_SERIALIZE_LOAD_STORE_TEMPLATE_EXPLICIT_IMPLEMENTATION(TetMesh)