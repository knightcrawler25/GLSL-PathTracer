#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "Mesh.h"
#include <iostream>

namespace GLSLPT
{
	bool Mesh::loadFromFile(const std::string &filename)
	{
		meshName = filename;
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), 0, true);

		if (!ret)
		{
			printf("Unable to load model\n");
			return false;
		}

		// Load vertices
		int vertCount = int(attrib.vertices.size() / 3);
		vertices.resize(vertCount);
		#pragma omp parallel for
		for (int i = 0; i < vertCount; i++)
			vertices[i] = glm::vec3(attrib.vertices[3 * i + 0], attrib.vertices[3 * i + 1], attrib.vertices[3 * i + 2]);

		// Load normals
		int nrmCount = int(attrib.normals.size() / 3);
		normals.resize(nrmCount);
		#pragma omp parallel for
		for (int i = 0; i < nrmCount; i++)
			normals[i] = glm::vec3(attrib.normals[3 * i + 0], attrib.normals[3 * i + 1], attrib.normals[3 * i + 2]);
			
		// Load uvs
		int uvCount = int(attrib.texcoords.size() / 2);
		uvs.resize(uvCount);
		#pragma omp parallel for
		for (int i = 0; i < uvCount; i++)
			uvs[i] = glm::vec2(attrib.texcoords[2 * i + 0], attrib.texcoords[2 * i + 1]);

		// Loop over shapes
		for (int s = 0; s < shapes.size(); s++)
		{
			// Loop over faces(polygon)
			int index_offset = 0;

			for (int f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				int fv = shapes[s].mesh.num_face_vertices[f];
				for (int i = 0; i < fv; i++)
				{
					tinyobj::index_t idx = tinyobj::index_t(shapes[s].mesh.indices[index_offset + i]);
					vert_indices.push_back(idx.vertex_index);
					nrm_indices.push_back(idx.normal_index);
					uv_indices.push_back(idx.texcoord_index);
				}
				index_offset += fv;
			}
		}
		return true;
	}

	void Mesh::buildBVH()
	{
		const int numTris = vert_indices.size() / 3;
		std::vector<RadeonRays::bbox> bounds(numTris);

		#pragma omp parallel for
		for (int i = 0; i < numTris; ++i)
		{
			const glm::vec3 v1 = vertices[vert_indices[i * 3 + 0]];
			const glm::vec3 v2 = vertices[vert_indices[i * 3 + 1]];
			const glm::vec3 v3 = vertices[vert_indices[i * 3 + 2]];

			bounds[i].grow(v1);
			bounds[i].grow(v2);
			bounds[i].grow(v3);
		}

		bvh->Build(&bounds[0], numTris);
	}
}