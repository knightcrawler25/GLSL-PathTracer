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

#include <vector>
#include <Vec3.h>

namespace GLSLPT
{
    class Material
    {
    public:
        Material()
        {
            albedo   = Vec3(1.0f, 1.0f, 1.0f);
            specular = 0.5f;

            emission    = Vec3(0.0f, 0.0f, 0.0f);
            anisotropic = 0.0f;

            metallic     = 0.0f;
            roughness    = 0.5f;
            subsurface   = 0.0f;
            specularTint = 0.0f;
            
            sheen          = 0.0f;
            sheenTint      = 0.0f;
            clearcoat      = 0.0f;
            clearcoatGloss = 0.0f;

            transmission  = 0.0f;
            ior           = 1.45f;
            extinction    = Vec3(1.0f, 1.0f, 1.0f);

            albedoTexID             = -1.0f;
            metallicRoughnessTexID  = -1.0f;
            normalmapTexID          = -1.0f;
        };

        Vec3 albedo;
        float specular;

        Vec3 emission;
        float anisotropic;

        float metallic;
        float roughness;
        float subsurface;
        float specularTint;
        
        float sheen;
        float sheenTint;
        float clearcoat;
        float clearcoatGloss;

        float transmission;
        float ior;
        Vec3 extinction;

        float albedoTexID;
        float metallicRoughnessTexID;
        float normalmapTexID;
    };
}