#ifndef PROGRAM_H
#define PROGRAM_H

#include <GL/glew.h>
#include <Shader.h>

// standard C++ libraries
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>

class Program
{
	private:
		GLuint _object;
	public:
		Program(const std::vector<Shader> shaders);
		void use();
		void stopUsing();
		GLuint object();
};

#endif PROGRAM_H