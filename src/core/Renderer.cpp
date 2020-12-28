/*
 * MIT License
 *
 * Copyright(c) 2019-2020 Asif Ali
 *
 * Authors/Contributors:
 *
 * Asif Ali
 * Cedric Guillemet
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this softwareand associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Config.h"
#include "Renderer.h"
#include "Scene.h"

namespace GLSLPT
{
    Program *LoadShaders(const std::string& vertShaderFilename, const std::string& fragShaderFilename)
    {
        std::vector<Shader> shaders;
        shaders.push_back(Shader(vertShaderFilename, GL_VERTEX_SHADER));
        shaders.push_back(Shader(fragShaderFilename, GL_FRAGMENT_SHADER));
        return new Program(shaders);
    }

    Renderer::Renderer(Scene *scene, const std::string& shadersDirectory) 
        : BVHTex(0)
        , vertexIndicesTex(0)
        , verticesTex(0)
        , normalsTex(0)
        , materialsTex(0)
        , transformsTex(0)
        , lightsTex(0)
        , textureMapsArrayTex(0)
        , hdrTex(0)
        , hdrMarginalDistTex(0)
        , hdrConditionalDistTex(0)
        , scene(scene)
        , quad(nullptr)
        , initialized(false)
        , numOfLights(scene->lights.size())
        , screenSize(scene->renderOptions.resolution)
        , shadersDirectory(shadersDirectory)
    {
    }

    Renderer::~Renderer()
    {
        if (initialized)
            this->Finish();
    }

    void Renderer::Finish()
    {
        if (!initialized)
            return;

        delete quad;

        glDeleteTextures(1, &BVHTex);
        glDeleteTextures(1, &vertexIndicesTex);
        glDeleteTextures(1, &verticesTex);
        glDeleteTextures(1, &normalsTex);
        glDeleteTextures(1, &materialsTex);
        glDeleteTextures(1, &transformsTex);
        glDeleteTextures(1, &lightsTex);
        glDeleteTextures(1, &textureMapsArrayTex);
        glDeleteTextures(1, &hdrTex);
        glDeleteTextures(1, &hdrMarginalDistTex);
        glDeleteTextures(1, &hdrConditionalDistTex);

        initialized = false;
        printf("Renderer finished!\n");
    }

    void Renderer::Init()
    {
        if (initialized)
            return;

        if (scene == nullptr)
        {
            printf("Error: No Scene Found\n");
            return ;
        }

        quad = new Quad();

        //Create Buffer and Texture for BVH Tree
        glGenBuffers(1, &BVHBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, BVHBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(RadeonRays::BvhTranslator::Node) * scene->bvhTranslator.nodes.size(), &scene->bvhTranslator.nodes[0], GL_STATIC_DRAW);
        glGenTextures(1, &BVHTex);
        glBindTexture(GL_TEXTURE_BUFFER, BVHTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, BVHBuffer);

        //Create Buffer and Texture for VertexIndices
        glGenBuffers(1, &vertexIndicesBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, vertexIndicesBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(Indices) * scene->vertIndices.size(), &scene->vertIndices[0], GL_STATIC_DRAW);
        glGenTextures(1, &vertexIndicesTex);
        glBindTexture(GL_TEXTURE_BUFFER, vertexIndicesTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32I, vertexIndicesBuffer);

        //Create Buffer and Texture for Vertices
        glGenBuffers(1, &verticesBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, verticesBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(Vec4) * scene->verticesUVX.size(), &scene->verticesUVX[0], GL_STATIC_DRAW);
        glGenTextures(1, &verticesTex);
        glBindTexture(GL_TEXTURE_BUFFER, verticesTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, verticesBuffer);

        //Create Buffer and Texture for Normals
        glGenBuffers(1, &normalsBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, normalsBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(Vec4) * scene->normalsUVY.size(), &scene->normalsUVY[0], GL_STATIC_DRAW);
        glGenTextures(1, &normalsTex);
        glBindTexture(GL_TEXTURE_BUFFER, normalsTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, normalsBuffer);

        //Create texture for Materials
        glGenTextures(1, &materialsTex);
        glBindTexture(GL_TEXTURE_2D, materialsTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (sizeof(Material) / sizeof(Vec4)) * scene->materials.size(), 1, 0, GL_RGBA, GL_FLOAT, &scene->materials[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        //Create texture for Transforms
        glGenTextures(1, &transformsTex);
        glBindTexture(GL_TEXTURE_2D, transformsTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (sizeof(Mat4) / sizeof(Vec4)) * scene->transforms.size(), 1, 0, GL_RGBA, GL_FLOAT, &scene->transforms[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        //Create texture for Lights
        if (numOfLights > 0)
        {
            //Create texture for lights
            glGenTextures(1, &lightsTex);
            glBindTexture(GL_TEXTURE_2D, lightsTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, (sizeof(Light) / sizeof(Vec3)) * scene->lights.size(), 1, 0, GL_RGB, GL_FLOAT, &scene->lights[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        if (!scene->textures.empty())
        {
            glGenTextures(1, &textureMapsArrayTex);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, textureMapsArrayTex);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, scene->texWidth, scene->texHeight, scene->textures.size(), 0, GL_RGB, GL_UNSIGNED_BYTE, &scene->textureMapsArray[0]);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        }

        // Environment Map
        if (scene->hdrData != nullptr)
        {
            glGenTextures(1, &hdrTex);
            glBindTexture(GL_TEXTURE_2D, hdrTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, scene->hdrData->width, scene->hdrData->height, 0, GL_RGB, GL_FLOAT, scene->hdrData->cols);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenTextures(1, &hdrMarginalDistTex);
            glBindTexture(GL_TEXTURE_2D, hdrMarginalDistTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, scene->hdrData->height, 1, 0, GL_RG, GL_FLOAT, scene->hdrData->marginalDistData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenTextures(1, &hdrConditionalDistTex);
            glBindTexture(GL_TEXTURE_2D, hdrConditionalDistTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, scene->hdrData->width, scene->hdrData->height, 0, GL_RG, GL_FLOAT, scene->hdrData->conditionalDistData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        initialized = true;
    }

    void Renderer::Update(float secondsElapsed)
    {
        if (scene->instancesModified)
        {
            glBindTexture(GL_TEXTURE_2D, transformsTex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (sizeof(Mat4) / sizeof(Vec4)) * scene->transforms.size(), 1, GL_RGBA, GL_FLOAT, &scene->transforms[0]);

            glBindTexture(GL_TEXTURE_2D, materialsTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (sizeof(Material) / sizeof(Vec4)) * scene->materials.size(), 1, 0, GL_RGBA, GL_FLOAT, &scene->materials[0]);

            int index = scene->bvhTranslator.topLevelIndex;

            glBindBuffer(GL_TEXTURE_BUFFER, BVHBuffer);
            glBufferSubData(GL_TEXTURE_BUFFER, sizeof(RadeonRays::BvhTranslator::Node) * index, sizeof(RadeonRays::BvhTranslator::Node) * (scene->bvhTranslator.nodes.size() - index), &scene->bvhTranslator.nodes[index]);
        }
    }
}