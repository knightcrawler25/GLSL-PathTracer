#pragma once

#include <GL/glew.h>
#include <Renderer.h>

class ProgressiveRenderer: public Renderer
{
private:
	GLuint pathTraceFBO, pathTraceFBOHalf, accumFBO;
	Program *pathTraceShader, *accumShader, *outputShader, *outputFadeShader;
	GLuint pathTraceTexture, pathTraceTextureHalf, accumTexture;
	int maxSamples, maxDepth;
	float sampleCounter, timeToFade, fadeTimer, lowResTimer;
	bool lowRes, fadeIn;

public:
	ProgressiveRenderer(const Scene *scene, glm::vec2 scrSize, int maxDepth) : Renderer(scene, scrSize)
	{ 
		this->maxDepth = maxDepth;
		init(); 
	};
	void init();
	void render();
	void update(float secondsElapsed);
};