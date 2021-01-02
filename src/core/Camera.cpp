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

#include <Camera.h>
#include <iostream>
#include <cstring>

namespace GLSLPT
{

    void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16)
    {
        float temp, temp2, temp3, temp4;
        temp = 2.0f * znear;
        temp2 = right - left;
        temp3 = top - bottom;
        temp4 = zfar - znear;
        m16[0] = temp / temp2;
        m16[1] = 0.0;
        m16[2] = 0.0;
        m16[3] = 0.0;
        m16[4] = 0.0;
        m16[5] = temp / temp3;
        m16[6] = 0.0;
        m16[7] = 0.0;
        m16[8] = (right + left) / temp2;
        m16[9] = (top + bottom) / temp3;
        m16[10] = (-zfar - znear) / temp4;
        m16[11] = -1.0f;
        m16[12] = 0.0;
        m16[13] = 0.0;
        m16[14] = (-temp * zfar) / temp4;
        m16[15] = 0.0;
    }

    void Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float* m16)
    {
        float ymax, xmax;
        ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
        xmax = ymax * aspectRatio;
        Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
    }

    void Cross(const float* a, const float* b, float* r)
    {
        r[0] = a[1] * b[2] - a[2] * b[1];
        r[1] = a[2] * b[0] - a[0] * b[2];
        r[2] = a[0] * b[1] - a[1] * b[0];
    }

    float Dot(const float* a, const float* b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    void Normalize(const float* a, float* r)
    {
        float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
        r[0] = a[0] * il;
        r[1] = a[1] * il;
        r[2] = a[2] * il;
    }

    void LookAt(const float* eye, const float* at, const float* up, float* m16)
    {
        float X[3], Y[3], Z[3], tmp[3];

        tmp[0] = eye[0] - at[0];
        tmp[1] = eye[1] - at[1];
        tmp[2] = eye[2] - at[2];

        Normalize(tmp, Z);
        Normalize(up, Y);

        Cross(Y, Z, tmp);
        Normalize(tmp, X);

        Cross(Z, X, tmp);
        Normalize(tmp, Y);

        m16[0] = X[0];
        m16[1] = Y[0];
        m16[2] = Z[0];
        m16[3] = 0.0f;
        m16[4] = X[1];
        m16[5] = Y[1];
        m16[6] = Z[1];
        m16[7] = 0.0f;
        m16[8] = X[2];
        m16[9] = Y[2];
        m16[10] = Z[2];
        m16[11] = 0.0f;
        m16[12] = -Dot(X, eye);
        m16[13] = -Dot(Y, eye);
        m16[14] = -Dot(Z, eye);
        m16[15] = 1.0f;
    }

    Camera::Camera(Vec3 eye, Vec3 lookat, float fov)
    {
        position = eye;
        pivot = lookat;
        worldUp = Vec3(0, 1, 0);

        Vec3 dir = Vec3::Normalize(pivot - position);
        pitch = Math::Degrees(asin(dir.y));
        yaw = Math::Degrees(atan2(dir.z, dir.x));

        radius = Vec3::Distance(eye, lookat);

        this->fov = Math::Radians(fov);
        focalDist = 0.1f;
        aperture = 0.0;
        UpdateCamera();
    }

    Camera::Camera(const Camera& other)
    {
        *this = other;
    }

    Camera& Camera::operator = (const Camera& other)
    {
        ptrdiff_t l = (unsigned char*)&isMoving - (unsigned char*)&position.x;
        isMoving = memcmp(&position.x, &other.position.x, l) != 0;
        memcpy(&position.x, &other.position.x, l);
        return *this;
    }

    void Camera::OffsetOrientation(float dx, float dy)
    {
        pitch -= dy;
        yaw += dx;
        UpdateCamera();
    }

    void Camera::Strafe(float dx, float dy)
    {
        Vec3 translation =  right * -dx + up * dy;
        pivot = pivot + translation;
        UpdateCamera();
    }

    void Camera::SetRadius(float dr)
    {
        radius += dr;
        UpdateCamera();
    }

    void Camera::SetFov(float val)
    {
        fov = Math::Radians(val);
    }

    void Camera::UpdateCamera()
    {
        Vec3 forward_temp;
        forward_temp.x = cos(Math::Radians(yaw)) * cos(Math::Radians(pitch));
        forward_temp.y = sin(Math::Radians(pitch));
        forward_temp.z = sin(Math::Radians(yaw)) * cos(Math::Radians(pitch));

        forward = Vec3::Normalize(forward_temp);
        position = pivot + (forward * -1.0f) * radius;

        right = Vec3::Normalize(Vec3::Cross(forward, worldUp));
        up = Vec3::Normalize(Vec3::Cross(right, forward));
    }

    void Camera::ComputeViewProjectionMatrix(float* view, float* projection, float ratio)
    {
        Vec3 at = position + forward;
        LookAt(&position.x, &at.x, &up.x, view);
        const float fov_v = (1.f / ratio) * tanf(fov / 2.f);
        Perspective(Math::Degrees(fov_v), ratio, 0.1f, 1000.f, projection);
    }
}
