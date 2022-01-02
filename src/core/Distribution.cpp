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
#include <Distribution.h>

namespace GLSLPT
{
    int LowerBound(const float* array, int lower, int upper, const float value)
    {
        while (lower < upper)
        {
            int mid = lower + (upper - lower) / 2;
            if (array[mid] < value)
                lower = mid + 1;
            else
                upper = mid;
        }
        return lower - 1;
    }

    Distribution1D::Distribution1D(const float* f, int n)
        : func(f, f + n), cdf(n + 1)
    {
        cdf[0] = 0;

        for (int i = 1; i < n + 1; i++)
            cdf[i] = cdf[i - 1] + func[i - 1] / n;

        integral = cdf[n];

        // Normalize
        for (int i = 1; i < n + 1; i++)
            cdf[i] /= cdf[n];
    }

    float Distribution1D::SampleContinuous(float r, float& pdf, int& offset)
    {
        offset = LowerBound(&cdf[0], 0, cdf.size(), r);
        pdf = func[offset] / integral;

        float d = r - cdf[offset];

        if ((cdf[offset + 1] - cdf[offset]) > 0)
            d /= (cdf[offset + 1] - cdf[offset]);

        return (offset + d) / func.size();
    }

    int Distribution1D::SampleDiscrete(float r, float& pdf)
    {
        int offset = LowerBound(&cdf[0], 0, cdf.size(), r);
        pdf = func[offset] / (integral * cdf.size());
        return offset;
    }

    float Distribution1D::DiscretePdf(int offset)
    {
        return func[offset] / (integral * cdf.size());
    }

    Distribution2D::Distribution2D(const float* data, int w, int h)
    {
        for (int i = 0; i < h; i++)
            condDist.push_back(new Distribution1D(&data[i * w], w));

        std::vector<float> margFunc;
        for (int i = 0; i < h; i++)
            margFunc.push_back(condDist[i]->integral);

        marginalDist = new Distribution1D(&margFunc[0], h);
    }

    Distribution2D::~Distribution2D()
    {
        for (int i = 0; i < condDist.size(); i++)
            delete condDist[i];
        delete marginalDist;
    }

    Vec2 Distribution2D::SampleContinuous(float r1, float r2, float& pdf)
    {
        float pdfs[2];
        int h, offset;
        float d2 = marginalDist->SampleContinuous(r1, pdfs[0], h);
        float d1 = condDist[h]->SampleContinuous(r2, pdfs[1], offset);
        pdf = pdfs[0] * pdfs[1];
        return Vec2(d1, d2);
    }
}