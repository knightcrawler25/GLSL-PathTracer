#pragma once

#include "Renderer.h"

namespace GLSLPathTracer
{
    class Scene;
    class ProgressiveRenderer : public Renderer
    {
    private:
        GLuint pathTraceFBO, pathTraceFBOHalf, accumFBO;
        Program *pathTraceShader, *accumShader, *outputShader, *outputFadeShader;
        GLuint pathTraceTexture, pathTraceTextureHalf, accumTexture;
        int maxSamples, maxDepth;
        float sampleCounter, timeToFade, fadeTimer, lowResTimer;
        bool lowRes, fadeIn;

    public:
        ProgressiveRenderer(const Scene *scene, const std::string& shadersDirectory);
        
        void init();
        void finish();

        void render();
        void present() const;
        void update(float secondsElapsed);
        float getProgress() const;
        RendererType getType() const { return Renderer_Progressive; }
    };
}