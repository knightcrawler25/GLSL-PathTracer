#include "Texture.h"
#include "SOIL.h"
#include <iostream>

namespace GLSLPT
{
	bool Texture::loadTexture(const std::string &filename)
	{
		texData = SOIL_load_image(filename.c_str(), &width, &height, 0, SOIL_LOAD_RGB);
		return true;
	}
}