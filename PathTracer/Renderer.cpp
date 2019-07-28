#include "Config.h"
#include "Renderer.h"
#include "Scene.h"

namespace GLSLPathTracer
{
    Program *loadShaders(const std::string &vertex_shader_fileName, const std::string &frag_shader_fileName)
    {
        std::vector<Shader> shaders;
        shaders.push_back(Shader(vertex_shader_fileName, GL_VERTEX_SHADER));
        shaders.push_back(Shader(frag_shader_fileName, GL_FRAGMENT_SHADER));
        return new Program(shaders);
    }

    Renderer::Renderer(const Scene *scene, const std::string& shadersDirectory) : albedoTextures(0)
        , metallicRoughnessTextures(0)
        , normalTextures(0)
        , hdrTexture(0)
        , hdrMarginalDistTexture(0)
        , hdrConditionalDistTexture(0)
        , initialized(false)
        , scene(scene)
        , screenSize(scene->renderOptions.resolution)
        , shadersDirectory(shadersDirectory)
    {
    }
    Renderer::~Renderer()
    {
        if (initialized)
            this->finish();
    }

    void Renderer::finish()
    {
        if (!initialized)
            return;

        glDeleteTextures(1, &BVHTexture);
        glDeleteTextures(1, &triangleIndicesTexture);
        glDeleteTextures(1, &verticesTexture);
        glDeleteTextures(1, &materialsTexture);
        glDeleteTextures(1, &lightsTexture);
        glDeleteTextures(1, &normalsTexCoordsTexture);
        glDeleteTextures(1, &albedoTextures);
        glDeleteTextures(1, &metallicRoughnessTextures);
        glDeleteTextures(1, &normalTextures);
        glDeleteTextures(1, &hdrTexture);
        glDeleteTextures(1, &hdrMarginalDistTexture);
        glDeleteTextures(1, &hdrConditionalDistTexture);

        glDeleteBuffers(1, &materialArrayBuffer);
        glDeleteBuffers(1, &triangleBuffer);
        glDeleteBuffers(1, &verticesBuffer);
        glDeleteBuffers(1, &lightArrayBuffer);
        glDeleteBuffers(1, &BVHBuffer);
        glDeleteBuffers(1, &normalTexCoordBuffer);

        initialized = false;
        Log("Renderer finished!\n");
    }

    void Renderer::init()
    {
        if (initialized)
            return;

        if (scene == nullptr)
        {
            Log("Error: No Scene Found\n");
            return ;
        }

        quad = new Quad();

        //Create Texture for BVH Tree
        glGenBuffers(1, &BVHBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, BVHBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(GPUBVHNode) * scene->gpuBVH->bvh->getNumNodes(), &scene->gpuBVH->gpuNodes[0], GL_STATIC_DRAW);
        glGenTextures(1, &BVHTexture);
        glBindTexture(GL_TEXTURE_BUFFER, BVHTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, BVHBuffer);

        //Create Buffer and Texture for TriangleIndices
        glGenBuffers(1, &triangleBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, triangleBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(TriIndexData) * scene->gpuBVH->bvhTriangleIndices.size(), &scene->gpuBVH->bvhTriangleIndices[0], GL_STATIC_DRAW);
        glGenTextures(1, &triangleIndicesTexture);
        glBindTexture(GL_TEXTURE_BUFFER, triangleIndicesTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, triangleBuffer);

        //Create Buffer and Texture for Vertices
        glGenBuffers(1, &verticesBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, verticesBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(VertexData) * scene->vertexData.size(), &scene->vertexData[0], GL_STATIC_DRAW);
        glGenTextures(1, &verticesTexture);
        glBindTexture(GL_TEXTURE_BUFFER, verticesTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, verticesBuffer);

        //Create Buffer and Normals and TexCoords
        glGenBuffers(1, &normalTexCoordBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, normalTexCoordBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(NormalTexData) * scene->normalTexData.size(), &scene->normalTexData[0], GL_STATIC_DRAW);
        glGenTextures(1, &normalsTexCoordsTexture);
        glBindTexture(GL_TEXTURE_BUFFER, normalsTexCoordsTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, normalTexCoordBuffer);

        //Create Buffer and Texture for Materials
        glGenBuffers(1, &materialArrayBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, materialArrayBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(MaterialData) * scene->materialData.size(), &scene->materialData[0], GL_STATIC_DRAW);
        glGenTextures(1, &materialsTexture);
        glBindTexture(GL_TEXTURE_BUFFER, materialsTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, materialArrayBuffer);

        //Create Buffer and Texture for Lights
        numOfLights = int(scene->lightData.size());

        if (numOfLights > 0)
        {
            glGenBuffers(1, &lightArrayBuffer);
            glBindBuffer(GL_TEXTURE_BUFFER, lightArrayBuffer);
            glBufferData(GL_TEXTURE_BUFFER, sizeof(LightData) * scene->lightData.size(), &scene->lightData[0], GL_STATIC_DRAW);
            glGenTextures(1, &lightsTexture);
            glBindTexture(GL_TEXTURE_BUFFER, lightsTexture);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, lightArrayBuffer);
        }

        // Albedo Texture
        if (scene->texData.albedoTexCount > 0)
        {
            glGenTextures(1, &albedoTextures);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, albedoTextures);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, scene->texData.albedoTextureSize.x, scene->texData.albedoTextureSize.y, scene->texData.albedoTexCount, 0, GL_RGB, GL_UNSIGNED_BYTE, scene->texData.albedoTextures);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        }

        //Metallic Roughness
        if (scene->texData.metallicRoughnessTexCount > 0)
        {
            glGenTextures(1, &metallicRoughnessTextures);
            glBindTexture(GL_TEXTURE_2D_ARRAY, metallicRoughnessTextures);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, scene->texData.metallicRoughnessTextureSize.x, scene->texData.metallicRoughnessTextureSize.y, scene->texData.metallicRoughnessTexCount, 0, GL_RGB, GL_UNSIGNED_BYTE, scene->texData.metallicRoughnessTextures);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        }

        //NormalMap
        if (scene->texData.normalTexCount > 0)
        {
            glGenTextures(1, &normalTextures);
            glBindTexture(GL_TEXTURE_2D_ARRAY, normalTextures);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, scene->texData.normalTextureSize.x, scene->texData.normalTextureSize.y, scene->texData.normalTexCount, 0, GL_RGB, GL_UNSIGNED_BYTE, scene->texData.normalTextures);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        }

        // Environment Map
        if (scene->renderOptions.useEnvMap)
        {
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, scene->hdrLoaderRes.width, scene->hdrLoaderRes.height, 0, GL_RGB, GL_FLOAT, scene->hdrLoaderRes.cols);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenTextures(1, &hdrMarginalDistTexture);
            glBindTexture(GL_TEXTURE_2D, hdrMarginalDistTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, scene->hdrLoaderRes.height, 1, 0, GL_RG, GL_FLOAT, scene->hdrLoaderRes.marginalDistData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenTextures(1, &hdrConditionalDistTexture);
            glBindTexture(GL_TEXTURE_2D, hdrConditionalDistTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, scene->hdrLoaderRes.width, scene->hdrLoaderRes.height, 0, GL_RG, GL_FLOAT, scene->hdrLoaderRes.conditionalDistData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        initialized = true;
    }
}