#include "Config.h"
#include "ProgressiveRenderer.h"
#include "Camera.h"
#include "Scene.h"
namespace GLSLPathTracer
{
    ProgressiveRenderer::ProgressiveRenderer(const Scene *scene, const std::string& shadersDirectory) : Renderer(scene, shadersDirectory)
        , maxDepth(scene->renderOptions.maxDepth)
    {
    }

    void ProgressiveRenderer::init()
    {
        if (initialized)
            return;

        Renderer::init();

        sampleCounter = 1;
        timeToFade = 2.0f;
        fadeTimer = 0.0f;
        lowResTimer = 0.0f;
        lowRes = true;
        fadeIn = false;

        //----------------------------------------------------------
        // Shaders
        //----------------------------------------------------------
        pathTraceShader = loadShaders(shadersDirectory + "PathTraceVert.glsl", shadersDirectory + "PathTraceFrag.glsl");
        accumShader = loadShaders(shadersDirectory + "AccumVert.glsl", shadersDirectory + "AccumFrag.glsl");
        outputShader = loadShaders(shadersDirectory + "OutputVert.glsl", shadersDirectory + "OutputFrag.glsl");
        outputFadeShader = loadShaders(shadersDirectory + "OutputFadeVert.glsl", shadersDirectory + "OutputFadeFrag.glsl");

        //----------------------------------------------------------
        // FBO Setup
        //----------------------------------------------------------
        //Create FBOs for path trace shader
        glGenFramebuffers(1, &pathTraceFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);

        //Create Texture for FBO
        glGenTextures(1, &pathTraceTexture);
        glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x, screenSize.y, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTraceTexture, 0);

        //Create Half Res FBOs for path trace shader
        glGenFramebuffers(1, &pathTraceFBOHalf);
        glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBOHalf);

        //Create Half Res Texture for FBO
        glGenTextures(1, &pathTraceTextureHalf);
        glBindTexture(GL_TEXTURE_2D, pathTraceTextureHalf);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x / 2, screenSize.y / 2, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTraceTextureHalf, 0);

        //Create FBOs for screen shader
        glGenFramebuffers(1, &accumFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);

        //Create Texture for FBO
        glGenTextures(1, &accumTexture);
        glBindTexture(GL_TEXTURE_2D, accumTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x, screenSize.y, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexture, 0);

        GLuint shaderObject;

        pathTraceShader->use();
        shaderObject = pathTraceShader->object();

        glUniform1i(glGetUniformLocation(shaderObject, "maxDepth"), maxDepth);
        glUniform2f(glGetUniformLocation(shaderObject, "screenResolution"), float(screenSize.x), float(screenSize.y));
        glUniform1i(glGetUniformLocation(shaderObject, "numOfLights"), numOfLights);
        glUniform1i(glGetUniformLocation(shaderObject, "useEnvMap"), scene->renderOptions.useEnvMap);
        glUniform1f(glGetUniformLocation(shaderObject, "hdrResolution"), float(scene->hdrLoaderRes.width * scene->hdrLoaderRes.height));
        glUniform1f(glGetUniformLocation(shaderObject, "hdrMultiplier"), scene->renderOptions.hdrMultiplier);

        glUniform1i(glGetUniformLocation(shaderObject, "accumTexture"), 0);
        glUniform1i(glGetUniformLocation(shaderObject, "BVH"), 1);
        glUniform1i(glGetUniformLocation(shaderObject, "triangleIndicesTex"), 2);
        glUniform1i(glGetUniformLocation(shaderObject, "verticesTex"), 3);
        glUniform1i(glGetUniformLocation(shaderObject, "normalsTexCoordsTex"), 4);
        glUniform1i(glGetUniformLocation(shaderObject, "materialsTex"), 5);
        glUniform1i(glGetUniformLocation(shaderObject, "lightsTex"), 6);
        glUniform1i(glGetUniformLocation(shaderObject, "albedoTextures"), 7);
        glUniform1i(glGetUniformLocation(shaderObject, "metallicRoughnessTextures"), 8);
        glUniform1i(glGetUniformLocation(shaderObject, "normalTextures"), 9);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrTexture"), 10);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrMarginalDistTexture"), 11);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrCondDistTexture"), 12);

        pathTraceShader->stopUsing();
    }

    void ProgressiveRenderer::finish()
    {
        if (!initialized)
            return;

        glDeleteFramebuffers(1, &pathTraceFBO);
        glDeleteFramebuffers(1, &pathTraceFBOHalf);
        glDeleteFramebuffers(1, &accumFBO);

        glDeleteTextures(1, &pathTraceTexture);
        glDeleteTextures(1, &pathTraceTextureHalf);
        glDeleteTextures(1, &accumTexture);

        delete pathTraceShader;
        delete accumShader;
        delete outputShader;
        delete outputFadeShader;

        Renderer::finish();
    }

    void ProgressiveRenderer::render()
    {
        if (!initialized)
        {
            Log("Tiled Renderer is not initialized\n");
            return;
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, BVHTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_BUFFER, triangleIndicesTexture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_BUFFER, verticesTexture);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_BUFFER, normalsTexCoordsTexture);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_BUFFER, materialsTexture);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_BUFFER, lightsTexture);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D_ARRAY, albedoTextures);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D_ARRAY, metallicRoughnessTextures);
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D_ARRAY, normalTextures);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, hdrMarginalDistTexture);
        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_2D, hdrConditionalDistTexture);

        if (lowRes)
        {
            //---------------------------------------------------------
            // Pass 1: Path trace to half-res texture
            //---------------------------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBOHalf);
            glViewport(0, 0, screenSize.x / 2, screenSize.y / 2);
            quad->Draw(pathTraceShader);
        }
        else
        {
            //---------------------------------------------------------
            // Pass 1: Path trace to full-res texture
            //---------------------------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);
            glViewport(0, 0, screenSize.x, screenSize.y);
            quad->Draw(pathTraceShader);

            //----------------------------------------------------------
            // Pass 2: Accumulation buffer
            //---------------------------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
            glViewport(0, 0, screenSize.x, screenSize.y);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
            quad->Draw(accumShader);
        }
    }

    float ProgressiveRenderer::getProgress() const
    {
        if (lowRes || fadeIn)
            return 0.f;
        return 1.f;
    }

    void ProgressiveRenderer::present() const
    {
        if (!initialized)
            return;

        //----------------------------------------------------------
        // final output
        //----------------------------------------------------------
        if (lowRes)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pathTraceTextureHalf);
            quad->Draw(outputShader);
        }
        else if (fadeIn)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pathTraceTextureHalf);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
            quad->Draw(outputFadeShader);
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
            quad->Draw(outputShader);
        }
    }

    void ProgressiveRenderer::update(float secondsElapsed)
    {
        if (!initialized)
            return;

        float r1, r2, r3;
        r1 = r2 = r3 = 0;

        if (scene->camera->isMoving)
        {
            lowRes = true;
            lowResTimer = 0;
            fadeTimer = 0;
            sampleCounter = 1;

        }
        else if (lowResTimer < 1.0)
        {
            lowResTimer += secondsElapsed;
        }
        else
        {
            // Clear accumulated value before gathering
            if (lowRes)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
                glViewport(0, 0, screenSize.x, screenSize.y);
                glClear(GL_COLOR_BUFFER_BIT);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            lowRes = false;
            sampleCounter += 1;
            r1 = ((float)rand() / (RAND_MAX)), r2 = ((float)rand() / (RAND_MAX)), r3 = ((float)rand() / (RAND_MAX));
        }

        if (!lowRes && fadeTimer < timeToFade)
        {
            fadeIn = true;
            fadeTimer += secondsElapsed;
        }
        else
            fadeIn = false;

        GLuint shaderObject;

        pathTraceShader->use();
        shaderObject = pathTraceShader->object();
        glUniform3fv(glGetUniformLocation(shaderObject, "camera.position"), 1, glm::value_ptr(scene->camera->position));
        glUniform3fv(glGetUniformLocation(shaderObject, "camera.right"), 1, glm::value_ptr(scene->camera->right));
        glUniform3fv(glGetUniformLocation(shaderObject, "camera.up"), 1, glm::value_ptr(scene->camera->up));
        glUniform3fv(glGetUniformLocation(shaderObject, "camera.forward"), 1, glm::value_ptr(scene->camera->forward));
        glUniform1f(glGetUniformLocation(shaderObject, "camera.fov"), scene->camera->fov);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.focalDist"), scene->camera->focalDist);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.aperture"), scene->camera->aperture);
        glUniform3fv(glGetUniformLocation(shaderObject, "randomVector"), 1, glm::value_ptr(glm::vec3(r1, r2, r3)));
        glUniform1i(glGetUniformLocation(shaderObject, "sampleCounter"), int(sampleCounter));
        glUniform1i(glGetUniformLocation(shaderObject, "maxDepth"), lowRes ? 2 : maxDepth);
        glUniform1i(glGetUniformLocation(shaderObject, "isCameraMoving"), scene->camera->isMoving);
        glUniform2f(glGetUniformLocation(shaderObject, "screenResolution"), float(screenSize.x), float(screenSize.y));
        pathTraceShader->stopUsing();

        outputShader->use();
        shaderObject = outputShader->object();
        glUniform1f(glGetUniformLocation(shaderObject, "invSampleCounter"), lowRes ? 1.0f : (1.0f / sampleCounter));
        outputShader->stopUsing();

        outputFadeShader->use();
        shaderObject = outputFadeShader->object();
        glUniform1i(glGetUniformLocation(shaderObject, "pathTraceTextureHalf"), 0);
        glUniform1i(glGetUniformLocation(shaderObject, "pathTraceTexture"), 1);
        glUniform1f(glGetUniformLocation(shaderObject, "invSampleCounter"), 1.0f / sampleCounter);
        glUniform1f(glGetUniformLocation(shaderObject, "fadeAmt"), glm::min(fadeTimer / timeToFade, 1.0f));
        outputFadeShader->stopUsing();
    }
}