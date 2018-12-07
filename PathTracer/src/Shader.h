#ifndef SHADER_H
#define SHADER_H
// third-party libraries
#include <GL/glew.h>

// standard C++ libraries
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>

#include <string>
#include <sstream>
#include <iostream>

class Shader
{
	private:
		GLuint _object;
	public:
		Shader(std::string filePath, GLuint shaderType);
		GLuint object() const;
};

#endif SHADER_H