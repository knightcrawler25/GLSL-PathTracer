/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
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

#pragma once

#include <vector>
#include "Quad.h"
#include "Program.h"
#include "Vec2.h"
#include "Vec3.h"

namespace GLSLPT
{
    Program* LoadShaders(const ShaderInclude::ShaderSource& vertShaderObj, const ShaderInclude::ShaderSource& fragShaderObj);

    struct RenderOptions
    {
        RenderOptions()
        {
            maxDepth = 2;
            tileWidth = 100;
            tileHeight = 100;
            useEnvMap = false;
            resolution = iVec2(1280, 720);
            hdrMultiplier = 1.0f;
            enableRR = true;
            useUniformLight = false;
            RRDepth = 2;
            uniformLightCol = Vec3(0.3f, 0.3f, 0.3f);
            denoiserFrameCnt = 20;
            enableDenoiser = false;
            enableTonemap = true;
            useAces = false;
            texArrayWidth = 4096;
            texArrayHeight = 4096;
            openglNormalMap = true;
            hideEmitters = false;
            enableBackground = false;
            backgroundCol = Vec3(1.0f, 1.0f, 1.0f);
            transparentBackground = false;
        }

        iVec2 resolution;
        int maxDepth;
        int tileWidth;
        int tileHeight;
        bool useEnvMap;
        bool enableRR;
        bool enableDenoiser;
        bool useUniformLight;
        bool enableTonemap;
        bool useAces;
        bool simpleAcesFit;
        int RRDepth;
        int denoiserFrameCnt;
        float hdrMultiplier;
        Vec3 uniformLightCol;
        int texArrayWidth;
        int texArrayHeight;
        bool openglNormalMap;
        bool hideEmitters;
        Vec3 backgroundCol;
        bool enableBackground;
        bool transparentBackground;
    };

    class Scene;

    class Renderer
    {
    protected:
        Scene *scene;
        Quad* quad;

        iVec2 screenSize;
        std::string shadersDirectory;

        GLuint BVHBuffer;
        GLuint BVHTex;
        GLuint vertexIndicesBuffer;
        GLuint vertexIndicesTex;
        GLuint verticesBuffer;
        GLuint verticesTex;
        GLuint normalsBuffer;
        GLuint normalsTex;
        GLuint materialsTex;
        GLuint transformsTex;
        GLuint lightsTex;
        GLuint textureMapsArrayTex;
        GLuint hdrTex;
        GLuint hdrMarginalDistTex;
        GLuint hdrConditionalDistTex;

        int numOfLights;
        bool initialized;

    public:
        Renderer(Scene *scene, const std::string& shadersDirectory);
        virtual ~Renderer();

        const iVec2 GetScreenSize() const { return screenSize; }

        virtual void Init();
        virtual void Finish();

        virtual void Render() = 0;
        virtual void Present() const = 0;
        virtual void Update(float secondsElapsed);
        virtual float GetProgress() const = 0;
        virtual int GetSampleCount() const = 0;
        virtual void GetOutputBuffer(unsigned char**, int &w, int &h) = 0;
    };
}