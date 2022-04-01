#include "Icosphere.hpp"

#include <glm/glm.hpp>


std::vector<float> icosphere::genIcosphere() {
	constexpr float PI = 3.14159265358979323846f;
	constexpr float H_ANGLE = (PI / 180) * 72;         // 72 degree = 360 / 5
	float V_ANGLE = std::atan(1.0f / 2.0f);  // elevation = 26.565 degree

	std::vector<glm::vec3> vertices(12);
	float z = std::sin(V_ANGLE), xy = std::cos(V_ANGLE); // elevation
	float hAngle1 = -PI / 2 - H_ANGLE / 2; // start at the second row. -126 degrees
	float hAngle2 = -PI / 2; // -90 degrees. third row

	vertices[0] = glm::vec3(0, 0, 1); // topmost vertex of radius 1

	// all the vertices (10) between the top and the bottom
	for (int i = 1; i <= 5; ++i)
	{
		int j = i + 5; // the simetric index

		vertices[i] = glm::vec3(
			xy * std::cos(hAngle1),
			xy * std::sin(hAngle1),
			z
		);

		vertices[j] = glm::vec3(
			xy * std::cos(hAngle2),
			xy * std::sin(hAngle2),
			-z
		);
		hAngle1 += H_ANGLE;
		hAngle2 += H_ANGLE;
	}

	vertices[11] = glm::vec3(0, 0, -1);

	std::vector<int> indices;
	indices.reserve(20 * 3);
	auto addIndices = [&indices](int a, int b, int c)
	{
		indices.push_back(a);
		indices.push_back(b);
		indices.push_back(c);
	};

	int v0, v1, v2, v3, v4, v11;
	v0 = 0;
	v11 = 11;
	for (int i = 1; i <= 5; ++i)
	{
		// 4 vertices in the 2nd row
		v1 = i;
		v3 = i + 5;
		if (i < 5) {
			v2 = i + 1;
			v4 = i + 6;
		}
		else
		{
			v2 = 1;
			v4 = 6;
		}

		// 4 new triangles per iteration
		addIndices(v0, v1, v2);
		addIndices(v1, v3, v2);
		addIndices(v2, v3, v4);
		addIndices(v3, v11, v4);
	}

	std::vector<glm::vec3> normals(indices.size());
	for (int i = 0; i < indices.size(); i += 3)
	{
		glm::vec3 ab = vertices[indices[i + 1]] - vertices[indices[i]],
			ac = vertices[indices[i + 2]] - vertices[indices[i]];
		glm::vec3 normal = glm::normalize(glm::cross(ab, ac));
		if (glm::dot(normal, vertices[indices[i + 1]]) < 0)
		{
			// invert vertices and normal
			normal = -normal;
			std::swap(indices[i + 1], indices[i + 2]);
		}


		normals[i] = normal;
		normals[i + 1] = normal;
		normals[i + 2] = normal;
	}

	std::vector<glm::vec3> normals2(vertices.size(), glm::vec3(0));
	// smoth normals
	for (int i = 0; i < indices.size(); ++i)
	{
		normals2[indices[i]] += normals[i];
	}
	for (glm::vec3& v : normals2)
	{
		v = glm::normalize(v);
	}

	std::vector<float> res(2 * 3 * indices.size());
	for (int i = 0; i < indices.size(); ++i)
	{
		int p = 6 * i;
		res[p + 0] = vertices[indices[i]].x;
		res[p + 1] = vertices[indices[i]].y;
		res[p + 2] = vertices[indices[i]].z;

		res[p + 3] = normals2[indices[i]].x;
		res[p + 4] = normals2[indices[i]].y;
		res[p + 5] = normals2[indices[i]].z;

	}
	return res;
}