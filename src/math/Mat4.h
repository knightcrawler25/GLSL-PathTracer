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

namespace GLSLPT
{
    struct Mat4
    {

    public:
        Mat4();

        inline float(&operator [](int i))[4]{ return data[i]; };
  
        float data[4][4];
    };

    inline Mat4::Mat4()
    {
        data[0][0] = 1; data[0][1] = 0;  data[0][2] = 0;  data[0][3] = 0;
        data[1][0] = 0; data[1][1] = 1;  data[1][2] = 0;  data[1][3] = 0;
        data[2][0] = 0; data[2][1] = 0;  data[2][2] = 1;  data[2][3] = 0;
        data[3][0] = 0; data[3][1] = 0;  data[3][2] = 0;  data[3][3] = 1;
    };
}