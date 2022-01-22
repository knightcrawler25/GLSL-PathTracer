/*
 * MIT License
 *
 * Copyright(c) 2019 Asif Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Config.h"
#include "Renderer.h"
#include "ShaderIncludes.h"
#include "Scene.h"
#include "OpenImageDenoise/oidn.hpp"

namespace GLSLPT
{
    Program* LoadShaders(const ShaderInclude::ShaderSource& vertShaderObj, const ShaderInclude::ShaderSource& fragShaderObj)
    {
        std::vector<Shader> shaders;
        shaders.push_back(Shader(vertShaderObj, GL_VERTEX_SHADER));
        shaders.push_back(Shader(fragShaderObj, GL_FRAGMENT_SHADER));
        return new Program(shaders);
    }

    Renderer::Renderer(Scene* scene, const std::string& shadersDirectory)
        : scene(scene)
        , BVHBuffer(0)
        , BVHTex(0)
        , vertexIndicesBuffer(0)
        , vertexIndicesTex(0)
        , verticesBuffer(0)
        , verticesTex(0)
        , normalsBuffer(0)
        , normalsTex(0)
        , transformsTex(0)
        , pathTraceTexture(0)
        , accumTexture(0)
        , pathTraceFBO(0)
        , accumFBO(0)
        , shadersDirectory(shadersDirectory)
        , pathTraceShader(nullptr)
        , outputShader(nullptr)
        , tonemapShader(nullptr)
    {
        if (scene == nullptr)
        {
            printf("No Scene Found\n");
            return;
        }

        if (!scene->initialized)
            scene->ProcessScene();

        quad = new Quad();

        Init();
    }

    Renderer::~Renderer()
    {
        delete quad;

        // Delete textures
        glDeleteTextures(1, &BVHTex);
        glDeleteTextures(1, &vertexIndicesTex);
        glDeleteTextures(1, &verticesTex);
        glDeleteTextures(1, &normalsTex);
        glDeleteTextures(1, &transformsTex);
        glDeleteTextures(1, &pathTraceTexture);
        glDeleteTextures(1, &accumTexture);

        // Delete buffers
        glDeleteBuffers(1, &BVHBuffer);
        glDeleteBuffers(1, &vertexIndicesBuffer);
        glDeleteBuffers(1, &verticesBuffer);
        glDeleteBuffers(1, &normalsBuffer);

        // Delete FBOs
        glDeleteFramebuffers(1, &pathTraceFBO);
        glDeleteFramebuffers(1, &accumFBO);

        // Delete shaders
        delete pathTraceShader;
        delete outputShader;
        delete tonemapShader;
    }

    void Renderer::Init()
    {
        //glPixelStorei(GL_PACK_ALIGNMENT, 1);

        // Create buffer and texture for vertex indices
        glGenBuffers(1, &vertexIndicesBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, vertexIndicesBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(Indices) * scene->vertIndices.size(), &scene->vertIndices[0], GL_STATIC_DRAW);
        glGenTextures(1, &vertexIndicesTex);
        glBindTexture(GL_TEXTURE_BUFFER, vertexIndicesTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32I, vertexIndicesBuffer);

        // Create buffer and texture for vertices
        glGenBuffers(1, &verticesBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, verticesBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(Vec4) * scene->verticesUVX.size(), &scene->verticesUVX[0], GL_STATIC_DRAW);
        glGenTextures(1, &verticesTex);
        glBindTexture(GL_TEXTURE_BUFFER, verticesTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, verticesBuffer);

        // Create buffer and texture for normals
        glGenBuffers(1, &normalsBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, normalsBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(Vec4) * scene->normalsUVY.size(), &scene->normalsUVY[0], GL_STATIC_DRAW);
        glGenTextures(1, &normalsTex);
        glBindTexture(GL_TEXTURE_BUFFER, normalsTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, normalsBuffer);

        sampleCounter = 1;

        renderSize = scene->renderOptions.renderResolution;
        windowSize = scene->renderOptions.windowResolution;

        // Create FBOs for path trace shader 
        glGenFramebuffers(1, &pathTraceFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);

        // Create Texture for FBO
        glGenTextures(1, &pathTraceTexture);
        glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowSize.x, windowSize.y, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTraceTexture, 0);

        // Create FBOs for accum buffer
        glGenFramebuffers(1, &accumFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);

        // Create Texture for FBO
        glGenTextures(1, &accumTexture);
        glBindTexture(GL_TEXTURE_2D, accumTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowSize.x, windowSize.y, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexture, 0);

        ShaderInclude::ShaderSource vertexShaderSrcObj = ShaderInclude::load(shadersDirectory + "vertex.glsl");
        ShaderInclude::ShaderSource pathTraceShaderSrcObj = ShaderInclude::load(shadersDirectory + "pathtrace.glsl");
        ShaderInclude::ShaderSource outputShaderSrcObj = ShaderInclude::load(shadersDirectory + "output.glsl");
        ShaderInclude::ShaderSource tonemapShaderSrcObj = ShaderInclude::load(shadersDirectory + "tonemap.glsl");

        pathTraceShader = LoadShaders(vertexShaderSrcObj, pathTraceShaderSrcObj);
        outputShader = LoadShaders(vertexShaderSrcObj, outputShaderSrcObj);
        tonemapShader = LoadShaders(vertexShaderSrcObj, tonemapShaderSrcObj);

        // Setup shader uniforms
        GLuint shaderObject;
        pathTraceShader->Use();
        shaderObject = pathTraceShader->getObject();
        glUniform1i(glGetUniformLocation(shaderObject, "numIndices"), scene->vertIndices.size());
        glUniform2f(glGetUniformLocation(shaderObject, "resolution"), renderSize.x, renderSize.y);
        glUniform1i(glGetUniformLocation(shaderObject, "accumTex"), 0);
        glUniform1i(glGetUniformLocation(shaderObject, "vertexIndicesTex"), 1);
        glUniform1i(glGetUniformLocation(shaderObject, "verticesTex"), 2);
        glUniform1i(glGetUniformLocation(shaderObject, "normalsTex"), 3);
        pathTraceShader->StopUsing();

        // Bind textures to texture slots as they will not change slots during the lifespan of the renderer
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, vertexIndicesTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_BUFFER, verticesTex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_BUFFER, normalsTex);
    }

    void Renderer::Render()
    {
        if (scene->dirty)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            scene->dirty = false;
        }

        glActiveTexture(GL_TEXTURE0);

        glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);
        glBindTexture(GL_TEXTURE_2D, accumTexture);
        quad->Draw(pathTraceShader);

        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
        glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
        quad->Draw(outputShader);
    }

    void Renderer::Present()
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTexture);
        quad->Draw(tonemapShader);
    }

    void Renderer::Update(float secondsElapsed)
    {
        if (scene->dirty)
            sampleCounter = 1;
        else
            sampleCounter++;

        GLuint shaderObject;
        pathTraceShader->Use();
        shaderObject = pathTraceShader->getObject();
        glUniform3f(glGetUniformLocation(shaderObject, "position"), scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
        glUniform3f(glGetUniformLocation(shaderObject, "right"), scene->camera->right.x, scene->camera->right.y, scene->camera->right.z);
        glUniform3f(glGetUniformLocation(shaderObject, "up"), scene->camera->up.x, scene->camera->up.y, scene->camera->up.z);
        glUniform3f(glGetUniformLocation(shaderObject, "forward"), scene->camera->forward.x, scene->camera->forward.y, scene->camera->forward.z);
        glUniform1f(glGetUniformLocation(shaderObject, "fov"), scene->camera->fov);
        pathTraceShader->StopUsing();

        tonemapShader->Use();
        shaderObject = tonemapShader->getObject();
        glUniform1f(glGetUniformLocation(shaderObject, "invSampleCounter"), 1.0f / (sampleCounter));
        tonemapShader->StopUsing();
    }
}