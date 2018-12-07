#include "Loader.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <SOIL.h>
#include "linear_math.h"
# define M_PI  3.14159265358979323846

static const int kMaxLineLength = 2048;

bool LoadModel(Scene *scene, std::string filename, float materialId)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), 0, true);

	if (!ret)
	{
		printf_s("Unable to load model\n");
		return false;
	}

	// Load vertices
	int vertCount = attrib.vertices.size() / 3;
	size_t vertStartIndex = scene->vertexData.size();
	for (int i = 0; i < vertCount; i++)
		scene->vertexData.push_back(VertexData{ glm::vec3(attrib.vertices[3 * i + 0], attrib.vertices[3 * i + 1], attrib.vertices[3 * i + 2]) });

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++)
	{
		// Loop over faces(polygon)
		int index_offset = 0;
		
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			glm::vec4 indices;
			glm::vec3 v[3], n[3], t[3];
			bool hasNormals = true;
			float vx, vy, vz, nx, ny, nz, tx, ty;

			int fv = shapes[s].mesh.num_face_vertices[f];
			for (int i = 0; i < fv; i++)
			{
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + i];
				indices[i] = vertStartIndex + (3 * idx.vertex_index + i) / 3;

				vx = attrib.vertices[3 * idx.vertex_index + 0];
				vy = attrib.vertices[3 * idx.vertex_index + 1];
				vz = attrib.vertices[3 * idx.vertex_index + 2];

				//Normals
				if (idx.normal_index != -1)
				{
					nx = attrib.normals[3 * idx.normal_index + 0];
					ny = attrib.normals[3 * idx.normal_index + 1];
					nz = attrib.normals[3 * idx.normal_index + 2];
				}
				else
				{
					hasNormals = false;
				}

				//TexCoords
				if (idx.texcoord_index != -1)
				{
					tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					ty = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
				}
				else
				{
					//If model has no texture Coords then default to 0
					tx = ty = 0;
				}

				n[i] = glm::vec3(nx, ny, nz);
				t[i] = glm::vec3(tx, ty, materialId);
			}
			if (!hasNormals)
			{
				//If model has no normals then Generate flat normals
				glm::vec3 flatNormal = glm::normalize(glm::cross(v[1] - v[0], v[2] - v[0]));
				n[0] = n[1] = n[2] = flatNormal;
			}

			scene->triangleIndices.push_back (TriangleData{ indices });
			scene->normalTexData.push_back(NormalTexData{ n[0], n[1], n[2], t[0], t[1], t[2] });
			
			index_offset += fv;
		}
	}
	return true;
}

bool LoadScene(Scene *scene, const char* filename)
{
	FILE* file;
	fopen_s(&file, filename, "r");

	if (!file)
	{
		printf("Couldn't open %s for reading\n", filename);
		return false;
	}

	printf("Loading Scene..\n");

	struct Material
	{
		MaterialData materialData;
		int id;
	};

	std::map<std::string, Material> materialMap;
	std::vector<std::string> albedoTex;
	std::vector<std::string> metallicTex;
	std::vector<std::string> roughnessTex;
	std::vector<std::string> normalTex;

	int materialCount = 0;
	char line[kMaxLineLength];

	//Defaults
	MaterialData defaultMat;
	scene->materialData.push_back(defaultMat);
	materialCount++;
	Camera *defaultCamera = new Camera(glm::vec3(0,0,0), glm::vec3(0,0,-1));
	bool cameraAdded = false;

	while (fgets(line, kMaxLineLength, file))
	{
		// skip comments
		if (line[0] == '#')
			continue;

		// name used for materials and meshes
		char name[kMaxLineLength] = { 0 };

		//--------------------------------------------
		// Material

		if (sscanf(line, " material %s", name) == 1)
		{
			printf("%s", line);

			MaterialData material;
			char albedoTexName[100]    = "None";
			char metallicTexName[100]  = "None";
			char roughnessTexName[100] = "None";
			char normalTexName[100]    = "None";

			while (fgets(line, kMaxLineLength, file))
			{
				// end group
				if (strchr(line, '}'))
					break;

				sscanf(line, " name %s", name);
				sscanf(line, " color %f %f %f", &material.albedo.x, &material.albedo.y, &material.albedo.z);
				sscanf(line, " materialType %f", &material.albedo.w);
				sscanf(line, " metallic %f", &material.params.x);
				sscanf(line, " roughness %f", &material.params.y);
				sscanf(line, " ior %f", &material.params.z);
				sscanf(line, " transmittance %f", &material.params.w);

				sscanf(line, " albedoTexture %s", &albedoTexName);
				sscanf(line, " metallicTexture %s", &metallicTexName);
				sscanf(line, " roughnessTexture %s", &roughnessTexName);
				sscanf(line, " normalTexture %s", normalTexName);
			}

			// Albedo Texture
			if (strcmp(albedoTexName, "None") != 0)
			{
				ptrdiff_t pos = std::distance(albedoTex.begin(), find(albedoTex.begin(), albedoTex.end(), albedoTexName));
				if (pos == albedoTex.size()) // New texture
				{
					albedoTex.push_back(albedoTexName);
					material.texIDs.x = albedoTex.size() - 1;
				}
				else
					material.texIDs.x = pos;
			}
			else
				material.texIDs.x = -1;

			// Metallic Texture
			if (strcmp(metallicTexName, "None") != 0)
			{
				ptrdiff_t pos = std::distance(metallicTex.begin(), find(metallicTex.begin(), metallicTex.end(), metallicTexName));
				if (pos == metallicTex.size())
				{
					metallicTex.push_back(metallicTexName);
					material.texIDs.y = metallicTex.size() - 1;
				}
				else
					material.texIDs.y = pos;
			}
			else
				material.texIDs.y = -1;

			// Roughness Texture
			if (strcmp(roughnessTexName, "None") != 0)
			{
				ptrdiff_t pos = std::distance(roughnessTex.begin(), find(roughnessTex.begin(), roughnessTex.end(), roughnessTexName));
				if (pos == roughnessTex.size())
				{
					roughnessTex.push_back(roughnessTexName);
					material.texIDs.z = roughnessTex.size() - 1;
				}
				else
					material.texIDs.z = pos;
			}
			else
				material.texIDs.z = -1;

			// Normals Texture
			if (strcmp(normalTexName, "None") != 0)
			{
				ptrdiff_t pos = std::distance(normalTex.begin(), find(normalTex.begin(), normalTex.end(), normalTexName));
				if (pos == normalTex.size())
				{
					normalTex.push_back(normalTexName);
					material.texIDs.w = normalTex.size() - 1;
				}
				else
					material.texIDs.w = pos;
			}
			else
				material.texIDs.w = -1;

			// add material to map
			if (materialMap.find(name) == materialMap.end()) // New material
			{
				materialMap[name] = Material{ material, materialCount++ };
				scene->materialData.push_back(material);
			}
		}

		//--------------------------------------------
		// Light

		if (strstr(line, "light"))
		{
			LightData light;
			glm::vec3 v1, v2;
			char light_type[20] = "None";

			while (fgets(line, kMaxLineLength, file))
			{
				// end group
				if (strchr(line, '}'))
					break;

				sscanf(line, " position %f %f %f", &light.position.x, &light.position.y, &light.position.z);
				sscanf(line, " emission %f %f %f", &light.emission.x, &light.emission.y, &light.emission.z);

				sscanf(line, " radius %f", &light.radiusAreaType.x);
				sscanf(line, " v1 %f %f %f", &v1.x, &v1.y, &v1.z);
				sscanf(line, " v2 %f %f %f", &v2.x, &v2.y, &v2.z);
				sscanf(line, " type %s", light_type);
			}

			if (strcmp(light_type, "Quad") == 0)
			{
				light.radiusAreaType.z = 0;
				light.u = v1 - light.position;
				light.v = v2 - light.position;
				light.radiusAreaType.y = glm::length(glm::cross(light.u, light.v));
			}
			else if (strcmp(light_type, "Sphere") == 0)
			{
				light.radiusAreaType.z = 1;
				light.radiusAreaType.y = 4.0f * M_PI * light.radiusAreaType.x * light.radiusAreaType.x;
			}

			scene->lightData.push_back(light);
		}

		//--------------------------------------------
		// Camera

		if (strstr(line, "Camera"))
		{
			glm::vec3 position;
			glm::vec3 lookAt;
			
			while (fgets(line, kMaxLineLength, file))
			{
				// end group
				if (strchr(line, '}'))
					break;

				sscanf(line, " position %f %f %f", &position.x, &position.y, &position.z);
				sscanf(line, " lookAt %f %f %f", &lookAt.x, &lookAt.y, &lookAt.z);
			}

			scene->camera = new Camera(position, lookAt);
			cameraAdded = true;
		}


		//--------------------------------------------
		// Mesh

		if (strstr(line, "mesh"))
		{
			std::string meshPath;
			float materialId = 0.0f; // Default Material ID
			while (fgets(line, kMaxLineLength, file))
			{
				// end group
				if (strchr(line, '}'))
					break;

				char path[2048];
				
				if (sscanf(line, " file %s", path) == 1)
				{
					//meshPath = std::string("./assets/") + path;
					meshPath = path;
				}

				if (sscanf(line, " material %s", path) == 1)
				{
					// look up material in dictionary
					if (materialMap.find(path) != materialMap.end())
					{
						materialId = materialMap[path].id;
					}
					else
					{
						printf_s("Could not find material %s\n", path);
					}
				}
			}
			if (!meshPath.empty())
				LoadModel(scene, meshPath, materialId);
		}
	}

	if (!cameraAdded)
		scene->camera = defaultCamera;

	//Load all textures 
	//(Assume for now that all textures have same width and height as their group (albedo, metallic etc))
	int width, height;

	scene->texData.albedoTexCount = albedoTex.size();
	scene->texData.metallicTexCount = metallicTex.size();
	scene->texData.roughnessTexCount = roughnessTex.size();
	scene->texData.normalTexCount = normalTex.size();

	//Load albedo Textures
	for (size_t i = 0; i < albedoTex.size(); i++)
	{
		unsigned char * texture = SOIL_load_image(albedoTex[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
		if(i == 0) // Alloc memory based on first texture size
			scene->texData.albedoTextures = new unsigned char[width * height * 3 * albedoTex.size()];
		memcpy(&(scene->texData.albedoTextures[width * height * 3 * i]), &texture[0], width * height * 3);
		delete texture;
	}
	scene->texData.albedoTextureSize = glm::vec2(width, height);

	// TODO: Combine metallic and roughness into two channels of same texture
	//Load Metallic textures
	for (size_t i = 0; i < metallicTex.size(); i++)
	{
		unsigned char * texture = SOIL_load_image(metallicTex[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
		if (i == 0) // Alloc memory based on first texture size
			scene->texData.metallicTextures = new unsigned char[width * height * 3 * metallicTex.size()];
		memcpy(&(scene->texData.metallicTextures[width * height * 3 * i]), &texture[0], width * height * 3);
		delete texture;
	}
	scene->texData.metallicTextureSize = glm::vec2(width, height);

	//Load roughness textures
	for (size_t i = 0; i < roughnessTex.size(); i++)
	{
		unsigned char * texture = SOIL_load_image(roughnessTex[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
		if (i == 0) // Alloc memory based on first texture size
			scene->texData.roughnessTextures = new unsigned char[width * height * 3 * roughnessTex.size()];
		memcpy(&(scene->texData.roughnessTextures[width * height * 3 * i]), &texture[0], width * height * 3);
		delete texture;
	}
	scene->texData.roughnessTextureSize = glm::vec2(width, height);

	//Load normal textures
	for (size_t i = 0; i < normalTex.size(); i++)
	{
		unsigned char * texture = SOIL_load_image(normalTex[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
		if (i == 0) // Alloc memory based on first texture size
			scene->texData.normalTextures = new unsigned char[width * height * 3 * normalTex.size()];
		memcpy(&(scene->texData.normalTextures[width * height * 3 * i]), &texture[0], width * height * 3);
		delete texture;
	}
	scene->texData.normalTextureSize = glm::vec2(width, height);

	return true;
}