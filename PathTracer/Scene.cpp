#include <iostream>

#include "Scene.h"
#include "Camera.h"

namespace GLSLPT
{
    void Scene::addCamera(glm::vec3 pos, glm::vec3 lookAt, float fov)
    {
        delete camera;
        camera = new Camera(pos, lookAt, fov);
    }

	int Scene::addMesh(const std::string& filename)
	{
		// Check if mesh was already loaded
		int id = -1;
		std::map<std::string, int>::iterator it = meshMap.find(filename);

		if (it == meshMap.end()) // New Mesh
		{
			id = meshes.size();

			Mesh *mesh = new Mesh;

			if (mesh->loadFromFile(filename))
			{
				meshes.push_back(mesh);
				meshMap[filename] = id;
				printf("Model %s loaded\n", filename.c_str());
			}
			else
				id = -1;
		}
		else // Existing Mesh
		{
			id = meshMap[filename];
		}

		return id;
	}

	int Scene::addTexture(const std::string& filename)
	{
		// Check if texture was already loaded
		std::map<std::string, int>::iterator it = textureMap.find(filename);
		int id = -1;

		if (it == textureMap.end()) // New Texture
		{
			id = textures.size();

			Texture *texture = new Texture;

			if (texture->loadTexture(filename))
			{
				textures.push_back(texture);
				textureMap[filename] = id;
				printf("Texture %s loaded\n", filename.c_str());
			}
			else
				id = -1;
		}
		else // Existing Mesh
		{
			id = meshMap[filename];
		}

		return id;
	}

	int Scene::addMaterial(const Material& material)
	{
		int id = materials.size();
		materials.push_back(material);
		return id;
	}

	void Scene::addHDR(const std::string& filename)
	{
		delete hdrData;
		hdrData = HDRLoader::load(filename.c_str());
		if (hdrData == nullptr)
			printf("Unable to load HDR\n");
		else
		{
			printf("HDR %s loaded\n", filename.c_str());
			renderOptions.useEnvMap = true;
		}
	}

	int Scene::addMeshInstance(const MeshInstance &meshInstance)
	{
		int id = meshInstances.size();
		meshInstances.push_back(meshInstance);
		return id;
	}

	int Scene::addLight(const Light &light)
	{
		int id = lights.size();
		lights.push_back(light);
		return id;
	}

	void Scene::createTLAS()
	{
		// Loop through all the mesh Instances and build a Top Level BVH
		std::vector<RadeonRays::bbox> bounds;
		bounds.resize(meshInstances.size());

		#pragma omp parallel for
		for (int i = 0; i < meshInstances.size(); i++)
		{
			RadeonRays::bbox bbox = meshes[meshInstances[i].meshID]->bvh->Bounds();
			glm::mat4 matrix = meshInstances[i].transform;

			glm::vec3 minBound = bbox.pmin;
			glm::vec3 maxBound = bbox.pmax;

			glm::vec3 right       = glm::vec3(matrix[0][0], matrix[0][1], matrix[0][2]);
			glm::vec3 up          = glm::vec3(matrix[1][0], matrix[1][1], matrix[1][2]);
			glm::vec3 forward     = glm::vec3(matrix[2][0], matrix[2][1], matrix[2][2]);
			glm::vec3 translation = glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);

			glm::vec3 xa = right * minBound.x;
			glm::vec3 xb = right * maxBound.x;

			glm::vec3 ya = up * minBound.y;
			glm::vec3 yb = up * maxBound.y;

			glm::vec3 za = forward * minBound.z;
			glm::vec3 zb = forward * maxBound.z;

			minBound = glm::min(xa, xb) + glm::min(ya, yb) + glm::min(za, zb) + translation;
			maxBound = glm::max(xa, xb) + glm::max(ya, yb) + glm::max(za, zb) + translation;

			RadeonRays::bbox bound;
			bound.pmin = minBound;
			bound.pmax = maxBound;

			bounds[i] = bound;
		}
		sceneBvh->Build(&bounds[0], bounds.size());
		sceneBounds = sceneBvh->Bounds();
	}

	void Scene::createBLAS()
	{
		// Loop through all meshes and build BVHs
		#pragma omp parallel for
		for (int i = 0; i < meshes.size(); i++)
		{
			printf("Building BVH for %s\n", meshes[i]->meshName.c_str());
			meshes[i]->buildBVH();
		}
	}
	
	void Scene::rebuildInstancesData()
	{
		delete sceneBvh;
		sceneBvh = new RadeonRays::Bvh(10.0f, 64, false);

		createTLAS();
		bvhTranslator.UpdateTLAS(sceneBvh, meshInstances);

		//Copy transforms
		for (int i = 0; i < meshInstances.size(); i++)
			transforms[i] = meshInstances[i].transform;

		instancesModified = true;
	}

	void Scene::createAccelerationStructures()
	{
		createBLAS();

		printf("Building scene BVH\n");
		createTLAS();

		// Flatten BVH
		bvhTranslator.Process(sceneBvh, meshes, meshInstances);

		int verticesCnt = 0;

		//Copy mesh data
		for (int i = 0; i < meshes.size(); i++)
		{
			// Copy indices from BVH and not from Mesh
			int numIndices = meshes[i]->bvh->GetNumIndices();
			const int * triIndices = meshes[i]->bvh->GetIndices();

			for (int j = 0; j < numIndices; j++)
			{
				int index = triIndices[j];
				int v1 = (index * 3 + 0) + verticesCnt;
				int v2 = (index * 3 + 1) + verticesCnt;
				int v3 = (index * 3 + 2) + verticesCnt;

				vertIndices.push_back(Indices{ v1, v2, v3 });
			}

			vertices_uvx.insert(vertices_uvx.end(), meshes[i]->vertices_uvx.begin(), meshes[i]->vertices_uvx.end());
			normals_uvy.insert(normals_uvy.end(), meshes[i]->normals_uvy.begin(), meshes[i]->normals_uvy.end());

			verticesCnt += meshes[i]->vertices_uvx.size();
		}

		// Resize to power of 2
		indicesTexWidth  = (int)(sqrt(vertIndices.size()) + 1); 
		triDataTexWidth  = (int)(sqrt(vertices_uvx.size())+ 1); 

		vertIndices.resize(indicesTexWidth * indicesTexWidth);
		vertices_uvx.resize(triDataTexWidth * triDataTexWidth);
		normals_uvy.resize(triDataTexWidth * triDataTexWidth);

		for (int i = 0; i < vertIndices.size(); i++)
		{
			vertIndices[i].x = ((vertIndices[i].x % triDataTexWidth) << 12) | (vertIndices[i].x / triDataTexWidth);
			vertIndices[i].y = ((vertIndices[i].y % triDataTexWidth) << 12) | (vertIndices[i].y / triDataTexWidth);
			vertIndices[i].z = ((vertIndices[i].z % triDataTexWidth) << 12) | (vertIndices[i].z / triDataTexWidth);
		}

		//Copy transforms
		transforms.resize(meshInstances.size());
		#pragma omp parallel for
		for (int i = 0; i < meshInstances.size(); i++)
			transforms[i] = meshInstances[i].transform;

		//Copy Textures
		for (int i = 0; i < textures.size(); i++)
		{
			texWidth = textures[i]->width;
			texHeight = textures[i]->height;
			int texSize = texWidth * texHeight;
			textureMapsArray.insert(textureMapsArray.end(), &textures[i]->texData[0], &textures[i]->texData[texSize * 3]);
		}
	}
}