/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
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

#include <Vec3.h>

namespace GLSLPT
{
    struct Mat4
    {

    public:
        Mat4();

        float(&operator [](int i))[4]{ return data[i]; };
        Mat4 operator*(const Mat4& b) const;

        static Mat4 Translate(const Vec3& a);
        static Mat4 Scale(const Vec3& a);

        float data[4][4];
    };

    inline Mat4::Mat4()
    {
        data[0][0] = 1; data[0][1] = 0;  data[0][2] = 0;  data[0][3] = 0;
        data[1][0] = 0; data[1][1] = 1;  data[1][2] = 0;  data[1][3] = 0;
        data[2][0] = 0; data[2][1] = 0;  data[2][2] = 1;  data[2][3] = 0;
        data[3][0] = 0; data[3][1] = 0;  data[3][2] = 0;  data[3][3] = 1;
    };

    inline Mat4 Mat4::Translate(const Vec3& a)
    {
        Mat4 out;
        out[3][0] = a.x;
        out[3][1] = a.y;
        out[3][2] = a.z;
        return out;
    }

    inline Mat4 Mat4::Scale(const Vec3& a)
    {
        Mat4 out;
        out[0][0] = a.x;
        out[1][1] = a.y;
        out[2][2] = a.z;
        return out;
    }

    inline Mat4 Mat4::operator*(const Mat4& b) const
    {
        Mat4 out;

        out[0][0] = data[0][0] * b.data[0][0] + data[0][1] * b.data[1][0] + data[0][2] * b.data[2][0] + data[0][3] * b.data[3][0];
        out[0][1] = data[0][0] * b.data[0][1] + data[0][1] * b.data[1][1] + data[0][2] * b.data[2][1] + data[0][3] * b.data[3][1];
        out[0][2] = data[0][0] * b.data[0][2] + data[0][1] * b.data[1][2] + data[0][2] * b.data[2][2] + data[0][3] * b.data[3][2];
        out[0][3] = data[0][0] * b.data[0][3] + data[0][1] * b.data[1][3] + data[0][2] * b.data[2][3] + data[0][3] * b.data[3][3];

        out[1][0] = data[1][0] * b.data[0][0] + data[1][1] * b.data[1][0] + data[1][2] * b.data[2][0] + data[1][3] * b.data[3][0];
        out[1][1] = data[1][0] * b.data[0][1] + data[1][1] * b.data[1][1] + data[1][2] * b.data[2][1] + data[1][3] * b.data[3][1];
        out[1][2] = data[1][0] * b.data[0][2] + data[1][1] * b.data[1][2] + data[1][2] * b.data[2][2] + data[1][3] * b.data[3][2];
        out[1][3] = data[1][0] * b.data[0][3] + data[1][1] * b.data[1][3] + data[1][2] * b.data[2][3] + data[1][3] * b.data[3][3];

        out[2][0] = data[2][0] * b.data[0][0] + data[2][1] * b.data[1][0] + data[2][2] * b.data[2][0] + data[2][3] * b.data[3][0];
        out[2][1] = data[2][0] * b.data[0][1] + data[2][1] * b.data[1][1] + data[2][2] * b.data[2][1] + data[2][3] * b.data[3][1];
        out[2][2] = data[2][0] * b.data[0][2] + data[2][1] * b.data[1][2] + data[2][2] * b.data[2][2] + data[2][3] * b.data[3][2];
        out[2][3] = data[2][0] * b.data[0][3] + data[2][1] * b.data[1][3] + data[2][2] * b.data[2][3] + data[2][3] * b.data[3][3];

        out[3][0] = data[3][0] * b.data[0][0] + data[3][1] * b.data[1][0] + data[3][2] * b.data[2][0] + data[3][3] * b.data[3][0];
        out[3][1] = data[3][0] * b.data[0][1] + data[3][1] * b.data[1][1] + data[3][2] * b.data[2][1] + data[3][3] * b.data[3][1];
        out[3][2] = data[3][0] * b.data[0][2] + data[3][1] * b.data[1][2] + data[3][2] * b.data[2][2] + data[3][3] * b.data[3][2];
        out[3][3] = data[3][0] * b.data[0][3] + data[3][1] * b.data[1][3] + data[3][2] * b.data[2][3] + data[3][3] * b.data[3][3];

        return out;
    }
}