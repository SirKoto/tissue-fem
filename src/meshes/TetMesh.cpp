#include "TetMesh.hpp"

#include <fstream>
#include <glad/glad.h>

TetMesh::TetMesh()
{
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
	glBindVertexArray(m_vao);
	this->gl_bind_to_vao();
	glBindVertexArray(0);
}

TetMesh::TetMesh(TetMesh&& o)
{
	m_vertices = std::move(o.m_vertices);
	m_surface_faces = std::move(o.m_surface_faces);
	m_surface_vertices_normals = std::move(o.m_surface_vertices_normals);
	m_elements = std::move(o.m_elements);

	m_vao = o.m_vao;
	o.m_vao = 0;
	std::memcpy(m_vbos, o.m_vbos, sizeof(m_vbos));
	std::memset(o.m_vbos, 0, sizeof(m_vbos));
}

TetMesh::~TetMesh()
{
	if (m_vao != 0) {
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(sizeof(m_vbos) / sizeof(*m_vbos), m_vbos);
	}
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

	this->generate_normals();

	this->upload_to_gpu();

	return true;
}

void TetMesh::upload_to_gpu(bool dynamic_verts, bool dynamic_indices) const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER,
		m_vertices.size() * sizeof(Eigen::Vector3f),
		m_vertices.data(),
		dynamic_verts ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_normals);
	glBufferData(GL_ARRAY_BUFFER,
		m_surface_vertices_normals.size() * sizeof(Eigen::Vector3f),
		m_surface_vertices_normals.data(),
		dynamic_verts ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_indices);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		m_surface_faces.size() * sizeof(Eigen::Vector3i),
		m_surface_faces.data(),
		dynamic_indices ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TetMesh::draw_triangles() const
{
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES,
		3 * (GLsizei)m_surface_faces.size(),
		GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
}

void TetMesh::gl_bind_to_vao() const
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertices);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Eigen::Vector3f), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_normals);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Eigen::Vector3f), (void*)0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vbo_indices);
}

void TetMesh::clear()
{
	m_vertices.clear();
	m_surface_faces.clear();
	m_surface_vertices_normals.clear();
	m_elements.clear();
}

void TetMesh::generate_normals()
{
	m_surface_vertices_normals.clear();
	m_surface_vertices_normals.resize(m_vertices.size());

	// Compute the planes of all triangles
	std::vector<Eigen::Vector3f> triangleNormals(m_surface_faces.size());
	for (uint32_t t = 0; t < (uint32_t)m_surface_faces.size(); ++t) {
		const Eigen::Vector3f& v0 = m_vertices[m_surface_faces[t][0]];
		const Eigen::Vector3f& v1 = m_vertices[m_surface_faces[t][1]];
		const Eigen::Vector3f& v2 = m_vertices[m_surface_faces[t][2]];

		Eigen::Vector3f n = (v1 - v0).cross(v2 - v0);
		triangleNormals[t] = n.normalized();
	}

	// Compute V:{F}
	std::vector<std::vector<uint32_t>> vert2faces(m_vertices.size());
	std::vector<uint32_t> vertexArity(m_vertices.size(), 0);
	for (uint32_t t = 0; t < (uint32_t)m_surface_faces.size(); ++t) {
		vertexArity[m_surface_faces[t][0]] += 1;
		vertexArity[m_surface_faces[t][1]] += 1;
		vertexArity[m_surface_faces[t][2]] += 1;
	}
	for (uint32_t v = 0; v < (uint32_t)m_vertices.size(); ++v) {
		vert2faces[v].reserve(vertexArity[v]);
	}
	for (uint32_t t = 0; t < (uint32_t)m_surface_faces.size(); ++t) {
		vert2faces[m_surface_faces[t][0]].push_back(t);
		vert2faces[m_surface_faces[t][1]].push_back(t);
		vert2faces[m_surface_faces[t][2]].push_back(t);
	}

	for (uint32_t v = 0; v < (uint32_t)m_surface_vertices_normals.size(); ++v) {
		m_surface_vertices_normals[v] = Eigen::Vector3f::Zero();

		for (const uint32_t& f : vert2faces[v]) {
			m_surface_vertices_normals[v] += triangleNormals[f];
		}
		// TODO: compute weighted average normals
		m_surface_vertices_normals[v] /= (float)vert2faces[v].size();
	}
}
