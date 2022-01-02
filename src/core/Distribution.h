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

// Code is based on PBRT-v3 https://github.com/mmp/pbrt-v3/blob/master/src/core/sampling.h

#pragma once

#include <vector>
#include <algorithm>
#include <Vec2.h>

namespace GLSLPT
{
    class Distribution1D
    {
    public:
        Distribution1D(const float* f, int n);
        float SampleContinuous(float r, float& pdf, int& offset);
        int SampleDiscrete(float r, float& pdf);
        float DiscretePdf(int offset);

        std::vector<float> func, cdf;
        float integral;
    };

    class Distribution2D
    {
    public:
        Distribution2D(const float* data, int w, int h);
        ~Distribution2D();
        Vec2 SampleContinuous(float r1, float r2, float& pdf);

        std::vector<Distribution1D*> condDist;
        Distribution1D* marginalDist;
    };
}