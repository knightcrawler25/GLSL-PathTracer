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
#include <MathUtils.h>
#include <float.h>

namespace GLSLPT
{
    class Camera
    {
    public:
        Camera(Vec3 eye, Vec3 lookat, float fov);
        Camera(const Camera& other);
        Camera& operator = (const Camera& other);

        void OffsetOrientation(float dx, float dy);
        void Strafe(float dx, float dy);
        void SetRadius(float dr);
        void ComputeViewProjectionMatrix(float* view, float* projection, float ratio);
        void SetFov(float val);

        Vec3 position;
        Vec3 up;
        Vec3 right;
        Vec3 forward;

        float focalDist;
        float aperture;
        float fov;
        bool isMoving;
        
    private:
        void UpdateCamera();

        Vec3 worldUp;
        Vec3 pivot;

        float pitch;
        float radius;
        float yaw;
    };
}