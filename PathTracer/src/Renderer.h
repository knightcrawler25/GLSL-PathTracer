#pragma once

#include <GL/glew.h>
#include <Scene.h>
#include <Quad.h>
#include <Program.h>
#include <GPUBVH.h>
#include <Loader.h>
#include <GLFW/glfw3.h>	
#include <SOIL.h>

Program *loadShaders(std::string vertex_shader_fileName, std::string frag_shader_fileName);

class Renderer
{
protected:
	const Scene *scene;
	GLuint BVHTexture, triangleIndicesTexture, verticesTexture, materialsTexture, lightsTexture, normalsTexCoordsTexture;
	GLuint albedoTextures, metallicRoughnessTextures, normalTextures, hdrTexture, hdrMarginalDistTexture, hdrConditionalDistTexture;
	GLuint materialArrayBuffer, triangleBuffer, verticesBuffer, lightArrayBuffer, BVHBuffer, normalTexCoordBuffer;
	Quad *quad;
	int numOfLights;
	glm::vec2 screenSize;
public:
	Renderer(const Scene *scene)
	{ 
		this->screenSize = scene->renderOptions.resolution;
		this->scene = scene;
		init(); 
	};
	void init();
	virtual void render() = 0;
	virtual void update(float secondsElapsed) = 0;
};