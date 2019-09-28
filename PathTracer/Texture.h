#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "split_bvh.h"

namespace GLSLPT
{	
	class Texture
	{
	public:
		Texture() : texData(nullptr) {};
		~Texture() { delete texData; }

		bool loadTexture(const std::string &filename);
		
		int width, height;

		// Texture Data
		unsigned char* texData;
	};
}
