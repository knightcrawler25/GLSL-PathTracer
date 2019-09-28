#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "split_bvh.h"

namespace GLSLPT
{	
	class Mesh
	{
	public:
		Mesh()
		{ 
			bvh = new RadeonRays::SplitBvh(2.0f, 64, 0, 0.001f, 2.5f); 
		}
		~Mesh() { delete bvh; }
		
		// Mesh Data
		std::vector<int> vert_indices;
		std::vector<int> nrm_indices;
		std::vector<int> uv_indices;

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> uvs;

		RadeonRays::Bvh *bvh;
		std::string meshName;
		void buildBVH();
		bool loadFromFile(const std::string &filename);
	};

	class MeshInstance
	{
	public:
		MeshInstance(int mesh_id, glm::mat4 xform, int mat_id) : meshID(mesh_id), transform(xform) , materialID(mat_id) {}
		~MeshInstance() {}
		glm::mat4 transform;
		int materialID;
		int meshID;
	};
}
