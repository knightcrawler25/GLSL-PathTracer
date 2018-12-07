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
	float **sampleCounter;
public:
	TiledRenderer(const Scene *scene, glm::vec2 scrSize, int numTilesX, int numTilesY, int maxSamples, int maxDepth) : Renderer(scene, scrSize)
	{ 
		this->numTilesX = numTilesX;
		this->numTilesY = numTilesY;
		this->maxSamples = maxSamples;
		this->maxDepth = maxDepth;
		init(); 
	};
	void init();
	void render();
	void update(float secondsElapsed);
};