#pragma once

#include "Renderer.h"

namespace GLSLPT
{
    class Scene;
    class TiledRenderer : public Renderer
    {
    private:
		GLuint pathTraceFBO, pathTraceFBOLowRes, accumFBO, outputFBO;
		Program *pathTraceShader, *pathTraceShaderLowRes, *accumShader, *tileOutputShader, *outputShader;
		GLuint pathTraceTexture, pathTraceTextureLowRes, accumTexture, tileOutputTexture[2];
		int tileX, tileY, numTilesX, numTilesY, tileWidth, tileHeight, maxSamples, maxDepth, currentBuffer;
		bool renderCompleted;
		float sampleCounter, totalTime;
    public:
        TiledRenderer(Scene *scene, const std::string& shadersDirectory);
        ~TiledRenderer();
        
        void init();
        void finish();

        void render();
        void present() const;
        void update(float secondsElapsed);
        float getProgress() const;
    };
}