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

#pragma once

#include "Renderer.h"

namespace GLSLPT
{
    class Scene;
    class TiledRenderer : public Renderer
    {
    private:
        // FBOs
        GLuint pathTraceFBO;
        GLuint pathTraceFBOLowRes;
        GLuint accumFBO;
        GLuint outputFBO;

        // Shaders
        Program *pathTraceShader;
        Program *pathTraceShaderLowRes;
        Program *accumShader;
        Program *tileOutputShader;
        Program *outputShader;

        // Textures
        GLuint pathTraceTexture;
        GLuint pathTraceTextureLowRes;
        GLuint accumTexture;
        GLuint tileOutputTexture[2];

        int tileX;
        int tileY;
        int numTilesX;
        int numTilesY;
        int tileWidth;
        int tileHeight;

        int maxDepth;
        int currentBuffer;
        int sampleCounter;

    public:
        TiledRenderer(Scene *scene, const std::string& shadersDirectory);
        ~TiledRenderer();
        
        void Init();
        void Finish();

        void Render();
        void Present() const;
        void Update(float secondsElapsed);
        float GetProgress() const;
        int GetSampleCount() const;
    };
}