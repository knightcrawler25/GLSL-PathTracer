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
#include "TiledRenderer.h"
#include "ShaderIncludes.h"
#include "Camera.h"
#include "Scene.h"
#include <string>

namespace GLSLPT
{
    TiledRenderer::TiledRenderer(Scene *scene, const std::string& shadersDirectory) : Renderer(scene, shadersDirectory)
        , tileWidth(scene->renderOptions.tileWidth)
        , tileHeight(scene->renderOptions.tileHeight)
        , maxDepth(scene->renderOptions.maxDepth)
        , pathTraceFBO(0)
        , pathTraceFBOLowRes(0)
        , accumFBO(0)
        , outputFBO(0)
        , pathTraceShader(nullptr)
        , pathTraceShaderLowRes(nullptr)
        , accumShader(nullptr)
        , tileOutputShader(nullptr)
        , outputShader(nullptr)
        , pathTraceTexture(0)
        , pathTraceTextureLowRes(0)
        , accumTexture(0)
        , tileOutputTexture()
        , tileX(-1)
        , tileY(-1)
        , numTilesX(-1)
        , numTilesY(-1)
        , currentBuffer(0)
        , sampleCounter(0)
    {
    }

    TiledRenderer::~TiledRenderer() 
    {
    }

    void TiledRenderer::Init()
    {
        if (initialized)
            return;

        Renderer::Init();

        sampleCounter = 0;
        currentBuffer = 0;

        numTilesX = ceil((float)screenSize.x / tileWidth);
        numTilesY = ceil((float)screenSize.y / tileHeight);
        pixelRatio = 0.25f;

        tileX = -1;
        tileY = numTilesY - 1;

        //----------------------------------------------------------
        // Shaders
        //----------------------------------------------------------

        ShaderInclude::ShaderSource vertexShaderSrcObj          = ShaderInclude::load(shadersDirectory + "common/vertex.glsl");
        ShaderInclude::ShaderSource pathTraceShaderSrcObj       = ShaderInclude::load(shadersDirectory + "tiled.glsl");
        ShaderInclude::ShaderSource pathTraceShaderLowResSrcObj = ShaderInclude::load(shadersDirectory + "progressive.glsl");
        ShaderInclude::ShaderSource accumShaderSrcObj           = ShaderInclude::load(shadersDirectory + "accumulation.glsl");
        ShaderInclude::ShaderSource tileOutputShaderSrcObj      = ShaderInclude::load(shadersDirectory + "tileOutput.glsl");
        ShaderInclude::ShaderSource outputShaderSrcObj          = ShaderInclude::load(shadersDirectory + "output.glsl");

        // Add preprocessor defines for conditional compilation
        std::string defines = "";
        if (scene->renderOptions.useEnvMap && scene->hdrData != nullptr)
            defines += "#define ENVMAP\n";
        if (!scene->lights.empty())
            defines += "#define LIGHTS\n";
        if (scene->renderOptions.enableRR)
        {
            defines += "#define RR\n";
            defines += "#define RR_DEPTH " + std::to_string(scene->renderOptions.RRDepth) + "\n";
        }
        if (scene->renderOptions.useConstantBg)
            defines += "#define CONSTANT_BG\n";

        if (defines.size() > 0)
        {
            size_t idx = pathTraceShaderSrcObj.src.find("#version");
            if (idx != -1)
                idx = pathTraceShaderSrcObj.src.find("\n", idx);
            else
                idx = 0;
            pathTraceShaderSrcObj.src.insert(idx + 1, defines);

            idx = pathTraceShaderLowResSrcObj.src.find("#version");
            if (idx != -1)
                idx = pathTraceShaderLowResSrcObj.src.find("\n", idx);
            else
                idx = 0;
            pathTraceShaderLowResSrcObj.src.insert(idx + 1, defines);
        }

        pathTraceShader =       LoadShaders(vertexShaderSrcObj, pathTraceShaderSrcObj);
        pathTraceShaderLowRes = LoadShaders(vertexShaderSrcObj, pathTraceShaderLowResSrcObj);
        accumShader =           LoadShaders(vertexShaderSrcObj, accumShaderSrcObj);
        tileOutputShader =      LoadShaders(vertexShaderSrcObj, tileOutputShaderSrcObj);
        outputShader =          LoadShaders(vertexShaderSrcObj, outputShaderSrcObj);

        printf("Debug sizes : %d %d - %d %d\n", tileWidth, tileHeight, screenSize.x, screenSize.y);
        //----------------------------------------------------------
        // FBO Setup
        //----------------------------------------------------------
        //Create FBOs for path trace shader (Tiled)
        printf("Buffer pathTraceFBO\n");
        glGenFramebuffers(1, &pathTraceFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);

        //Create Texture for FBO
        glGenTextures(1, &pathTraceTexture);
        glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tileWidth, tileHeight, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTraceTexture, 0);

        //Create FBOs for path trace shader (Progressive)
        printf("Buffer pathTraceFBOLowRes\n");
        glGenFramebuffers(1, &pathTraceFBOLowRes);
        glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBOLowRes);

        //Create Texture for FBO
        glGenTextures(1, &pathTraceTextureLowRes);
        glBindTexture(GL_TEXTURE_2D, pathTraceTextureLowRes);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenSize.x * pixelRatio, screenSize.y * pixelRatio, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTraceTextureLowRes, 0);

        //Create FBOs for accum buffer
        printf("Buffer accumFBO\n");
        glGenFramebuffers(1, &accumFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);

        //Create Texture for FBO
        glGenTextures(1, &accumTexture);
        glBindTexture(GL_TEXTURE_2D, accumTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, GLsizei(screenSize.x), GLsizei(screenSize.y), 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexture, 0);

        //Create FBOs for tile output shader
        printf("Buffer outputFBO\n");
        glGenFramebuffers(1, &outputFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);

        //Create Texture for FBO
        glGenTextures(1, &tileOutputTexture[0]);
        glBindTexture(GL_TEXTURE_2D, tileOutputTexture[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenSize.x, screenSize.y, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenTextures(1, &tileOutputTexture[1]);
        glBindTexture(GL_TEXTURE_2D, tileOutputTexture[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenSize.x, screenSize.y, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tileOutputTexture[currentBuffer], 0);

        GLuint shaderObject;

        pathTraceShader->Use();
        shaderObject = pathTraceShader->getObject();

        glUniform1f(glGetUniformLocation(shaderObject, "hdrResolution"), scene->hdrData == nullptr ? 0 : float(scene->hdrData->width * scene->hdrData->height));
        glUniform1i(glGetUniformLocation(shaderObject, "topBVHIndex"), scene->bvhTranslator.topLevelIndex);
        glUniform2f(glGetUniformLocation(shaderObject, "screenResolution"), float(screenSize.x), float(screenSize.y));
        glUniform1i(glGetUniformLocation(shaderObject, "numOfLights"), numOfLights);
        glUniform1f(glGetUniformLocation(shaderObject, "invNumTilesX"), 1.0f / ((float)screenSize.x / tileWidth));
        glUniform1f(glGetUniformLocation(shaderObject, "invNumTilesY"), 1.0f / ((float)screenSize.y / tileHeight));
        glUniform1i(glGetUniformLocation(shaderObject, "accumTexture"), 0);
        glUniform1i(glGetUniformLocation(shaderObject, "BVH"), 1);
        glUniform1i(glGetUniformLocation(shaderObject, "vertexIndicesTex"), 2);
        glUniform1i(glGetUniformLocation(shaderObject, "verticesTex"), 3);
        glUniform1i(glGetUniformLocation(shaderObject, "normalsTex"), 4);
        glUniform1i(glGetUniformLocation(shaderObject, "materialsTex"), 5);
        glUniform1i(glGetUniformLocation(shaderObject, "transformsTex"), 6);
        glUniform1i(glGetUniformLocation(shaderObject, "lightsTex"), 7);
        glUniform1i(glGetUniformLocation(shaderObject, "textureMapsArrayTex"), 8);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrTex"), 9);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrMarginalDistTex"), 10);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrCondDistTex"), 11);

        pathTraceShader->StopUsing();

        pathTraceShaderLowRes->Use();
        shaderObject = pathTraceShaderLowRes->getObject();

        glUniform1f(glGetUniformLocation(shaderObject, "hdrResolution"), scene->hdrData == nullptr ? 0 : float(scene->hdrData->width * scene->hdrData->height));
        glUniform1i(glGetUniformLocation(shaderObject, "topBVHIndex"), scene->bvhTranslator.topLevelIndex);
        glUniform2f(glGetUniformLocation(shaderObject, "screenResolution"), float(screenSize.x), float(screenSize.y));
        glUniform1i(glGetUniformLocation(shaderObject, "numOfLights"), numOfLights);
        glUniform1i(glGetUniformLocation(shaderObject, "accumTexture"), 0);
        glUniform1i(glGetUniformLocation(shaderObject, "BVH"), 1);
        glUniform1i(glGetUniformLocation(shaderObject, "vertexIndicesTex"), 2);
        glUniform1i(glGetUniformLocation(shaderObject, "verticesTex"), 3);
        glUniform1i(glGetUniformLocation(shaderObject, "normalsTex"), 4);
        glUniform1i(glGetUniformLocation(shaderObject, "materialsTex"), 5);
        glUniform1i(glGetUniformLocation(shaderObject, "transformsTex"), 6);
        glUniform1i(glGetUniformLocation(shaderObject, "lightsTex"), 7);
        glUniform1i(glGetUniformLocation(shaderObject, "textureMapsArrayTex"), 8);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrTex"), 9);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrMarginalDistTex"), 10);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrCondDistTex"), 11);

        pathTraceShaderLowRes->StopUsing();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, BVHTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_BUFFER, vertexIndicesTex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_BUFFER, verticesTex);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_BUFFER, normalsTex);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, materialsTex);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, transformsTex);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, lightsTex);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureMapsArrayTex);
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, hdrTex);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, hdrMarginalDistTex);
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, hdrConditionalDistTex);
    }

    void TiledRenderer::Finish()
    {
        if (!initialized)
            return;

        glDeleteTextures(1, &pathTraceTexture);
        glDeleteTextures(1, &pathTraceTextureLowRes);
        glDeleteTextures(1, &accumTexture);
        glDeleteTextures(1, &tileOutputTexture[0]);
        glDeleteTextures(1, &tileOutputTexture[1]);

        glDeleteFramebuffers(1, &pathTraceFBO);
        glDeleteFramebuffers(1, &pathTraceFBOLowRes);
        glDeleteFramebuffers(1, &accumFBO);
        glDeleteFramebuffers(1, &outputFBO);

        delete pathTraceShader;
        delete accumShader;
        delete tileOutputShader;
        delete outputShader;

        Renderer::Finish();
    }

    void TiledRenderer::Render()
    {
        if (!initialized)
        {
            printf("Tiled Renderer is not initialized\n");
            return;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTexture);

        if (!scene->camera->isMoving)
        {
            // If instances are moved then render to low res buffer once to avoid ghosting from previous frame
            if (scene->instancesModified)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBOLowRes);
                glViewport(0, 0, screenSize.x * pixelRatio, screenSize.y * pixelRatio);
                quad->Draw(pathTraceShaderLowRes);
                scene->instancesModified = false;
            }

            GLuint shaderObject;
            pathTraceShader->Use();

            shaderObject = pathTraceShader->getObject();
            glUniform1i(glGetUniformLocation(shaderObject, "tileX"), tileX);
            glUniform1i(glGetUniformLocation(shaderObject, "tileY"), tileY);
            pathTraceShader->StopUsing();

            glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);
            glViewport(0, 0, tileWidth, tileHeight);
            quad->Draw(pathTraceShader);

            glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
            glViewport(tileWidth * tileX, tileHeight * tileY, tileWidth, tileHeight);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
            quad->Draw(accumShader);

            glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tileOutputTexture[currentBuffer], 0);
            glViewport(0, 0, screenSize.x, screenSize.y);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, accumTexture);
            quad->Draw(tileOutputShader);
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBOLowRes);
            glViewport(0, 0, screenSize.x * pixelRatio, screenSize.y * pixelRatio);
            quad->Draw(pathTraceShaderLowRes);
        }
    }

    void TiledRenderer::Present() const
    {
        if (!initialized)
            return;

        if (!scene->camera->isMoving)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tileOutputTexture[1 - currentBuffer]);
            quad->Draw(outputShader);
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pathTraceTextureLowRes);
            quad->Draw(outputShader);
        }
    }

    float TiledRenderer::GetProgress() const
    {
        return float((numTilesY - tileY - 1) * numTilesX + tileX) / float(numTilesX * numTilesY);
    }

    int TiledRenderer::GetSampleCount() const
    {
        return sampleCounter;
    }

    void TiledRenderer::Update(float secondsElapsed)
    {
        Renderer::Update(secondsElapsed);

        float r1, r2, r3;

        if (scene->camera->isMoving || scene->instancesModified)
        {
            r1 = r2 = r3 = 0;
            tileX = -1;
            tileY = numTilesY - 1;
            sampleCounter = 0;

            glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
            glViewport(0, 0, screenSize.x, screenSize.y);
            glClear(GL_COLOR_BUFFER_BIT);

            glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tileOutputTexture[1 - currentBuffer], 0);
            glViewport(0, 0, screenSize.x, screenSize.y);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pathTraceTextureLowRes);
            quad->Draw(accumShader);

            /*glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
            glViewport(0, 0, screenSize.x, screenSize.y);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pathTraceTextureLowRes);
            quad->Draw(accumShader);*/

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else
        {
            tileX++;
            if (tileX >= numTilesX)
            {
                tileX = 0;
                tileY--;
                if (tileY < 0)
                {
                    tileX = 0;
                    tileY = numTilesY - 1;
                    sampleCounter++;
                    currentBuffer = 1 - currentBuffer;
                }
            }

            r1 = ((float)rand() / (RAND_MAX));
            r2 = ((float)rand() / (RAND_MAX));
            r3 = ((float)rand() / (RAND_MAX));
        }


        GLuint shaderObject;

        pathTraceShader->Use();
        shaderObject = pathTraceShader->getObject();
        glUniform3f(glGetUniformLocation(shaderObject, "camera.position"), scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.right"),    scene->camera->right.x, scene->camera->right.y, scene->camera->right.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.up"),       scene->camera->up.x, scene->camera->up.y, scene->camera->up.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.forward"),  scene->camera->forward.x, scene->camera->forward.y, scene->camera->forward.z);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.fov"), scene->camera->fov);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.focalDist"), scene->camera->focalDist);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.aperture"), scene->camera->aperture);
        glUniform3f(glGetUniformLocation(shaderObject, "randomVector"), r1, r2, r3);
        glUniform1i(glGetUniformLocation(shaderObject, "useEnvMap"), scene->hdrData == nullptr ? false : scene->renderOptions.useEnvMap);
        glUniform1f(glGetUniformLocation(shaderObject, "hdrMultiplier"), scene->renderOptions.hdrMultiplier);
        glUniform1i(glGetUniformLocation(shaderObject, "maxDepth"), scene->renderOptions.maxDepth);
        glUniform1i(glGetUniformLocation(shaderObject, "tileX"), tileX);
        glUniform1i(glGetUniformLocation(shaderObject, "tileY"), tileY);
        glUniform3f(glGetUniformLocation(shaderObject, "bgColor"), scene->renderOptions.bgColor.x, scene->renderOptions.bgColor.y, scene->renderOptions.bgColor.z);
        pathTraceShader->StopUsing();

        pathTraceShaderLowRes->Use();
        shaderObject = pathTraceShaderLowRes->getObject();
        glUniform3f(glGetUniformLocation(shaderObject, "camera.position"), scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.right"), scene->camera->right.x, scene->camera->right.y, scene->camera->right.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.up"), scene->camera->up.x, scene->camera->up.y, scene->camera->up.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.forward"), scene->camera->forward.x, scene->camera->forward.y, scene->camera->forward.z);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.fov"), scene->camera->fov);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.focalDist"), scene->camera->focalDist);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.aperture"), scene->camera->aperture);
        glUniform1i(glGetUniformLocation(shaderObject, "useEnvMap"), scene->hdrData == nullptr ? false : scene->renderOptions.useEnvMap);
        glUniform1f(glGetUniformLocation(shaderObject, "hdrMultiplier"), scene->renderOptions.hdrMultiplier);
        glUniform1i(glGetUniformLocation(shaderObject, "maxDepth"), scene->camera->isMoving || scene->instancesModified ? 2: scene->renderOptions.maxDepth);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.position"), scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
        glUniform3f(glGetUniformLocation(shaderObject, "bgColor"), scene->renderOptions.bgColor.x, scene->renderOptions.bgColor.y, scene->renderOptions.bgColor.z);
        pathTraceShaderLowRes->StopUsing();

        outputShader->Use();
        shaderObject = outputShader->getObject();
        glUniform1f(glGetUniformLocation(shaderObject, "invSampleCounter"), scene->camera->isMoving || sampleCounter <= 0? 1.0f : 1.0f / sampleCounter);
        outputShader->StopUsing();
    }
}