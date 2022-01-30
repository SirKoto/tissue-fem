#include "TetMesh.hpp"

#include <fstream>

TetMesh::TetMesh(const TetMesh&& o)
{
	m_vertices = std::move(o.m_vertices);
	m_surface_faces = std::move(o.m_surface_faces);
	m_surface_vertices_normals = std::move(o.m_surface_vertices_normals);
	m_elements = std::move(o.m_elements);
}

bool TetMesh::load_tetgen(std::filesystem::path path, std::string* out_err)
{
	this->clear();

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
		Eigen::Vector3f v;
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

		m_surface_faces.resize(num_faces);
		int32_t idx;
		while (num_faces-- > 0) {
			stream >> idx;
			stream >> m_surface_faces[idx][0] >> m_surface_faces[idx][1] >> m_surface_faces[idx][2];
			if (stream.fail()) {
				err_out("Something went wrong when loading .ele");
				return false;
			}
		}
	}
	stream.close();

	return true;
}

void TetMesh::clear()
{
	m_vertices.clear();
	m_surface_faces.clear();
	m_surface_vertices_normals.clear();
	m_elements.clear();
}
