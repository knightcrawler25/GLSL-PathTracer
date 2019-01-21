#pragma once

#include "Quad.h"
#include "Program.h"
#include "GPUBVH.h"
#include "Loader.h"
#include "SOIL.h"

namespace GLSLPathTracer
{
    Program *loadShaders(const std::string &vertex_shader_fileName, const std::string &frag_shader_fileName);

    enum RendererType
    {
        Renderer_Progressive,
        Renderer_Tiled,
    };

    struct RenderOptions
    {
        RenderOptions()
        {
            rendererType = Renderer_Tiled;
            maxSamples = 10;
            maxDepth = 2;
            numTilesX = 5;
            numTilesY = 5;
            useEnvMap = false;
            resolution = glm::vec2(500, 500);
            hdrMultiplier = 1.0f;
        }
        //std::string rendererType;
        int rendererType; // see RendererType
        glm::ivec2 resolution;
        int maxSamples;
        int maxDepth;
        int numTilesX;
        int numTilesY;
        bool useEnvMap;
        float hdrMultiplier;
    };
    class Scene;
    class Renderer
    {
    protected:
        const Scene *scene;
        GLuint BVHTexture, triangleIndicesTexture, verticesTexture, materialsTexture, lightsTexture, normalsTexCoordsTexture;
        GLuint albedoTextures, metallicRoughnessTextures, normalTextures, hdrTexture, hdrMarginalDistTexture, hdrConditionalDistTexture;
        GLuint materialArrayBuffer, triangleBuffer, verticesBuffer, lightArrayBuffer, BVHBuffer, normalTexCoordBuffer;
        Quad *quad;
        int numOfLights;
        glm::ivec2 screenSize;
        bool initialized;
        std::string shadersDirectory;
    public:
        Renderer(const Scene *scene, const std::string& shadersDirectory);
        virtual ~Renderer();
        const glm::ivec2 getScreenSize() const { return screenSize; }

        virtual void init();
        virtual void finish();

        virtual void render() = 0;
        virtual void present() const = 0;
        virtual void update(float secondsElapsed) = 0;
        // range is [0..1]
        virtual float getProgress() const = 0;
        // used for UI
        virtual RendererType getType() const = 0;
    };
}