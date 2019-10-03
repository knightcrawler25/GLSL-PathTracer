#pragma once

#include "Quad.h"
#include "Program.h"
#include "SOIL.h"

#include <glm/glm.hpp>
#include <vector>

namespace GLSLPT
{
    Program *loadShaders(const std::string &vertex_shader_fileName, const std::string &frag_shader_fileName);

    struct RenderOptions
    {
        RenderOptions()
        {
            maxDepth = 2;
            numTilesX = 5;
            numTilesY = 5;
            useEnvMap = false;
            resolution = glm::vec2(1280, 720);
            hdrMultiplier = 1.0f;
        }
        glm::ivec2 resolution;
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
        Scene *scene;
		GLuint BVHTex, BBoxminTex, BBoxmaxTex, vertexIndicesTex, verticesTex, normalsTex,
			   materialsTex, transformsTex, lightsTex, textureMapsArrayTex, hdrTex, hdrMarginalDistTex, hdrConditionalDistTex;
        Quad *quad;
        int numOfLights;
        glm::ivec2 screenSize;
        bool initialized;
        std::string shadersDirectory;
    public:
        Renderer(Scene *scene, const std::string& shadersDirectory);
        virtual ~Renderer();
        const glm::ivec2 getScreenSize() const { return screenSize; }

        virtual void init();
        virtual void finish();

        virtual void render() = 0;
        virtual void present() const = 0;
        virtual void update(float secondsElapsed);
        // range is [0..1]
        virtual float getProgress() const = 0;
    };
}