#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>
#include "hdrloader.h"
#include "bvh.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Camera.h"
#include "bvh_translator.h"
#include "Texture.h"
#include "Material.h"

namespace GLSLPT
{
	class Camera;

	enum LightType
	{
		AreaLight,
		SphereLight
	};

	struct Light
	{
		glm::vec3 position;
		glm::vec3 emission;
		glm::vec3 u;
		glm::vec3 v;
		float radius;
		float area;
		LightType type;
	};

	struct Indices
	{
		int x, y, z;
	};

	class Scene
	{
	public:
		Scene() : camera(nullptr), hdrData(nullptr) {
			sceneBvh = new RadeonRays::Bvh(10.0f, 64, false);
		}
		~Scene() { delete camera; delete sceneBvh; delete hdrData; };
		void addCamera(glm::vec3 eye, glm::vec3 lookat, float fov);
		int addMesh(const std::string &filename);
		int addTexture(const std::string &filename);
		int addMaterial(const Material &material);
		int addMeshInstance(const MeshInstance &meshInstance);
		int addLight(const Light &light);
		void addHDR(const std::string &filename);
		void createAccelerationStructures();
		void rebuildInstancesData();

		//Options
		RenderOptions renderOptions;

		//Mesh Data
		std::vector<Mesh*> meshes;

		//Instance Data
		std::vector<Material> materials;
		std::vector<MeshInstance> meshInstances;

		//Lights
		std::vector<Light> lights;

		//HDR
		HDRData *hdrData;

		//Camera
		Camera *camera;

		// Scene Mesh Data 
		std::vector<Indices> vertIndices;
		std::vector<glm::vec4> vertices_uvx;
		std::vector<glm::vec4> normals_uvy;
		std::vector<glm::mat4> transforms;

		int indicesTexWidth;
		int triDataTexWidth;

		//Bvh
		RadeonRays::BvhTranslator bvhTranslator;

		//Texture Data
		std::vector<Texture *> textures;
		std::vector<unsigned char> textureMapsArray;
		int texWidth, texHeight; // TODO: allow textures of different sizes
		RadeonRays::bbox sceneBounds;

		bool instancesModified = false;

	private:
		std::map<std::string, int> meshMap;
		std::map<std::string, int> textureMap;
		RadeonRays::Bvh *sceneBvh;
		void createBLAS();
		void createTLAS();
	};
}