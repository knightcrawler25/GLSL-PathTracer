#pragma once

#include <GL/glew.h>
#include <Renderer.h>

class TiledRenderer: public Renderer
{
private:
	GLuint pathTraceFBO, accumFBO, outputFBO;
	Program *pathTraceShader, *accumShader, *tileOutputShader, *outputShader;
	GLuint pathTraceTexture, accumTexture, tileOutputTexture;
	int tileX, tileY, numTilesX, numTilesY, tileWidth, tileHeight, maxSamples, maxDepth;
	bool renderCompleted;
	float **sampleCounter, totalTime;
public:
	TiledRenderer(const Scene *scene, glm::vec2 scrSize) : Renderer(scene, scrSize)
	{ 
		this->numTilesX = scene->renderOptions.numTilesX;
		this->numTilesY = scene->renderOptions.numTilesY;
		this->maxSamples = scene->renderOptions.maxSamples;
		this->maxDepth = scene->renderOptions.maxDepth;
		init(); 
	};
	void init();
	void render();
	void update(float secondsElapsed);
};