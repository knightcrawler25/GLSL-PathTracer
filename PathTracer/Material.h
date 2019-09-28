#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace GLSLPT
{
	enum MaterialType
	{
		DISNEY,
		GLASS
	};

	class Material
	{
	public:
		Material()
		{
			albedo = glm::vec3(1.0f, 1.0f, 1.0f);
			materialType = DISNEY;
			emission = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
			metallic = 0.0f;
			roughness = 0.5f;
			ior = 1.45f;
			transmittance = 0.0f;
			albedoTexID = -1.0f;
			metallicRoughnessTexID = -1.0f;
			normalmapTexID = -1.0f;
			heightmapTexID = -1.0f;
		};

		glm::vec3 albedo;
		float materialType;
		glm::vec4 emission;
		float metallic;
		float roughness;
		float ior;
		float transmittance;
		float albedoTexID;
		float metallicRoughnessTexID;
		float normalmapTexID;
		float heightmapTexID;
	};
}