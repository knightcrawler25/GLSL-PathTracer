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

#include <Vec4.h>
#include <MathUtils.h>

namespace GLSLPT
{
    struct Vec3
    {
    public:
        Vec3();
        Vec3(float x, float y, float z);
        Vec3(const Vec4& b);

        Vec3 operator*(const Vec3& b) const;
        Vec3 operator+(const Vec3& b) const;
        Vec3 operator-(const Vec3& b) const;
        Vec3 operator*(float b) const;

        float operator[](int i) const;
        float& operator[](int i);

        static Vec3 Min(const Vec3& a, const Vec3& b);
        static Vec3 Max(const Vec3& a, const Vec3& b);
        static Vec3 Cross(const Vec3& a, const Vec3& b);
        static float Length(const Vec3& a);
        static float Distance(const Vec3& a, const Vec3& b);
        static float Dot(const Vec3& a, const Vec3& b);
        static Vec3 Clamp(const Vec3& a, const Vec3& min, const Vec3& max);
        static Vec3 Normalize(const Vec3& a);

        float x, y, z;
    };

    inline Vec3::Vec3()
    {
        x = y = z = 0;
    };

    inline Vec3::Vec3(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    };

    inline Vec3::Vec3(const Vec4& b)
    {
        x = b.x;
        y = b.y;
        z = b.z;
    };

    inline Vec3 Vec3::operator*(const Vec3& b) const
    {
        return Vec3(x * b.x, y * b.y, z * b.z);
    };

    inline Vec3 Vec3::operator+(const Vec3& b) const
    {
        return Vec3(x + b.x, y + b.y, z + b.z);
    };

    inline Vec3 Vec3::operator-(const Vec3& b) const
    {
        return Vec3(x - b.x, y - b.y, z - b.z);
    };

    inline Vec3 Vec3::operator*(float b) const
    {
        return Vec3(x * b, y * b, z * b);
    };

    inline float Vec3::operator[](int i) const
    {
        if (i == 0)
            return x;
        else if (i == 1)
            return y;
        else
            return z;
    };

    inline float& Vec3::operator[](int i)
    {
        if (i == 0)
            return x;
        else if (i == 1)
            return y;
        else
            return z;
    };

    inline Vec3 Vec3::Min(const Vec3& a, const Vec3& b)
    {
        Vec3 out;
        out.x = std::min(a.x, b.x);
        out.y = std::min(a.y, b.y);
        out.z = std::min(a.z, b.z);
        return out;
    };

    inline Vec3 Vec3::Max(const Vec3& a, const Vec3& b)
    {
        Vec3 out;
        out.x = std::max(a.x, b.x);
        out.y = std::max(a.y, b.y);
        out.z = std::max(a.z, b.z);
        return out;
    };

    inline Vec3 Vec3::Cross(const Vec3& a, const Vec3& b)
    {
        return Vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    };

    inline float Vec3::Length(const Vec3& a)
    {
        return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    };

    inline float Vec3::Dot(const Vec3& a, const Vec3& b)
    {
        return (a.x * b.x + a.y * b.y + a.z * b.z);
    };

    inline float Vec3::Distance(const Vec3& a, const Vec3& b)
    {
        Vec3 t = a;
        return Length(t - b);
    }

    inline Vec3 Vec3::Clamp(const Vec3& a, const Vec3& min, const Vec3& max)
    {
        return Vec3(
            Math::Clamp(a.x, min.x, max.x),
            Math::Clamp(a.y, min.y, max.y),
            Math::Clamp(a.z, min.z, max.z)
        );
    }

    inline Vec3 Vec3::Normalize(const Vec3& a)
    {
        float l = Length(a);
        return Vec3(a.x / l, a.y / l, a.z / l);
    };
}