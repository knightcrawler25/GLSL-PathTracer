#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "hdrloader.h"
#include "GPUBVH.h"

namespace GLSLPathTracer
{
    class Camera;

    struct TriangleData
    {
        glm::vec4 indices;
    };

    struct NormalTexData
    {
        glm::vec3 normals[3];
        glm::vec3 texCoords[3];
    };

    struct VertexData
    {
        glm::vec3 vertex;
    };

    struct MaterialData
    {
        MaterialData()
        {
            albedo = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
            emission = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            params = glm::vec4(0.0f, 0.5f, 0.0f, 0.0f);
            texIDs = glm::vec4(-1.0f, -1.0f, -1.0f, -1.0f);
        };
        glm::vec4 albedo;  // layout: R,G,B, MaterialType
        glm::vec4 emission;
        glm::vec4 params;  // layout: metallic, roughness, IOR, transmittance
        glm::vec4 texIDs;  // layout: (Texture Map IDs) albedo ID, metallicRoughness ID, normalMap ID
    };

    struct TexData
    {
        unsigned char* albedoTextures;
        unsigned char* metallicRoughnessTextures;
        unsigned char* normalTextures;

        int albedoTexCount;
        int metallicRoughnessTexCount;
        int normalTexCount;

        glm::ivec2 albedoTextureSize;
        glm::ivec2 metallicRoughnessTextureSize;
        glm::ivec2 normalTextureSize;
    };

    struct LightData
    {
        glm::vec3 position;
        glm::vec3 emission;
        glm::vec3 u;
        glm::vec3 v;
        glm::vec3 radiusAreaType;
    };

    struct RenderOptions
    {
        RenderOptions()
        {
            rendererType = "Tiled";
            maxSamples = 10;
            maxDepth = 2;
            numTilesX = 5;
            numTilesY = 5;
            useEnvMap = false;
            resolution = glm::vec2(500, 500);
            hdrMultiplier = 1.0f;
        }
        std::string rendererType;
        glm::ivec2 resolution;
        int maxSamples;
        int maxDepth;
        int numTilesX;
        int numTilesY;
        bool useEnvMap;
        float hdrMultiplier;
    };

    class Scene
    {
    public:
        Scene(const std::string filename) : filename(filename)
            , camera(nullptr) 
            , gpuBVH(nullptr)
        {}
        ~Scene();
        void addCamera(glm::vec3 pos, glm::vec3 lookAt, float fov);
        Camera *camera;
        GPUBVH *gpuBVH;
        std::vector<TriangleData> triangleIndices;
        std::vector<NormalTexData> normalTexData;
        std::vector<VertexData> vertexData;
        std::vector<MaterialData> materialData;
        std::vector<LightData> lightData;
        TexData texData;
        RenderOptions renderOptions;
        HDRLoaderResult hdrLoaderRes;
        void buildBVH();
        const std::string& getSceneName() const { return filename; }
    protected:
        std::string filename;
    };
}