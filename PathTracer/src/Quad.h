#pragma once
#include <Program.h>
#include <Camera.h>
#include <glm/gtc/type_ptr.hpp>

class Quad
{
	public:
		Quad();
		void Draw(Program *);
	private:
		GLuint vao, vbo;
};