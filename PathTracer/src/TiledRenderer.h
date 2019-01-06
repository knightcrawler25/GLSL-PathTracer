#pragma once

#include "Renderer.h"

namespace GLSLPathTracer
{
    class Scene;
    class TiledRenderer : public Renderer
    {
    private:
        GLuint pathTraceFBO, accumFBO, outputFBO;
        Program *pathTraceShader, *accumShader, *tileOutputShader, *outputShader;
        GLuint pathTraceTexture, accumTexture, tileOutputTexture;
        int tileX, tileY, numTilesX, numTilesY, tileWidth, tileHeight, maxSamples, maxDepth;
        bool renderCompleted;
        float **sampleCounter, totalTime;
    public:
        TiledRenderer(const Scene *scene, const std::string& shadersDirectory);
        ~TiledRenderer();
        
        void init();
        void finish();

        void render();
        void present() const;
        void update(float secondsElapsed);
        float getProgress() const;
        RendererType getType() const { return Renderer_Tiled; }
    };
}