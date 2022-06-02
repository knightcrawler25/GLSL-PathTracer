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

#pragma once

#include <vector>
#include "Vec3.h"

namespace GLSLPT
{
    enum AlphaMode
    {
        Opaque,
        Blend,
        Mask
    };

    enum MediumType
    {
        None,
        Absorb,
        Scatter,
        Emissive
    };

    class Material
    {
    public:
        Material()
        {
            baseColor = Vec3(1.0f, 1.0f, 1.0f);
            anisotropic = 0.0f;

            emission = Vec3(0.0f, 0.0f, 0.0f);
            // padding1

            metallic     = 0.0f;
            roughness    = 0.5f;
            subsurface   = 0.0f;
            specularTint = 0.0f;

            sheen          = 0.0f;
            sheenTint      = 0.0f;
            clearcoat      = 0.0f;
            clearcoatGloss = 0.0f;

            specTrans        = 0.0f;
            ior              = 1.5f;
            mediumType       = 0.0f;
            mediumDensity    = 0.0f;

            mediumColor      = Vec3(1.0f, 1.0f, 1.0f);
            mediumAnisotropy = 0.0f;

            baseColorTexId         = -1.0f;
            metallicRoughnessTexID = -1.0f;
            normalmapTexID         = -1.0f;
            emissionmapTexID       = -1.0f;

            opacity     = 1.0f;
            alphaMode   = 0.0f;
            alphaCutoff = 0.0f;
            // padding2
        };

        Vec3 baseColor;
        float anisotropic;

        Vec3 emission;
        float padding1;

        float metallic;
        float roughness;
        float subsurface;
        float specularTint;

        float sheen;
        float sheenTint;
        float clearcoat;
        float clearcoatGloss;

        float specTrans;
        float ior;
        float mediumType;
        float mediumDensity;
        
        Vec3 mediumColor;
        float mediumAnisotropy;

        float baseColorTexId;
        float metallicRoughnessTexID;
        float normalmapTexID;
        float emissionmapTexID;

        float opacity;
        float alphaMode;
        float alphaCutoff;
        float padding2;
    };
}