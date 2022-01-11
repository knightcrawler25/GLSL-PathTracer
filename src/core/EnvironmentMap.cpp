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

#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <string>
#include "EnvironmentMap.h"

namespace GLSLPT
{
    float Luminance(float r, float g, float b)
    {
        return 0.212671f * r + 0.715160f * g + 0.072169f * b;
    }

    // https://pbr-book.org/3ed-2018/Light_Transport_I_Surface_Reflection/Sampling_Light_Sources#InfiniteAreaLights
    void EnvironmentMap::BuildCDF()
    {
        // Gather weights for CDF
        float* weights = new float[width * height];
        for (int v = 0; v < height; v++)
        {
            for (int u = 0; u < width; u++)
            {
                int imgIdx = v * width * 3 + u * 3;
                weights[u + v * width] = Luminance(img[imgIdx + 0], img[imgIdx + 1], img[imgIdx + 2]);
            }
        }

        // Build CDF
        cdf = new float[width * height];
        cdf[0] = weights[0];
        for (int i = 1; i < width * height; i++)
            cdf[i] = cdf[i - 1] + weights[i];

        totalSum = cdf[width * height - 1];

        delete[] weights;
    }

    bool EnvironmentMap::LoadMap(const std::string& filename)
    {
        img = stbi_loadf(filename.c_str(), &width, &height, NULL, 3);

        if (img == nullptr)
            return false;

        BuildCDF();

        return true;
    }
}