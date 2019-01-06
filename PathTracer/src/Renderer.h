#pragma once

#include "Scene.h"
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
    public:
        Renderer(const Scene *scene) : albedoTextures(0)
            , metallicRoughnessTextures(0)
            , normalTextures(0)
            , hdrTexture(0)
            , hdrMarginalDistTexture(0)
            , hdrConditionalDistTexture(0)
        {
            this->screenSize = scene->renderOptions.resolution;
            this->scene = scene;
            init();
        };
        const glm::ivec2 getScreenSize() const { return screenSize; }
        bool init();
        virtual void render() = 0;
        virtual void present() const = 0;
        virtual void update(float secondsElapsed) = 0;
        // range is [0..1]
        virtual float getProgress() const = 0;
        // used for UI
        virtual RendererType getType() const = 0;
    };
}