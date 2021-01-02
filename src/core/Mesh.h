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
#include "split_bvh.h"

namespace GLSLPT
{    
    class Mesh
    {
    public:
        Mesh()
        { 
            bvh = new RadeonRays::SplitBvh(2.0f, 64, 0, 0.001f, 0); 
            //bvh = new RadeonRays::Bvh(2.0f, 64, false);
        }
        ~Mesh() { delete bvh; }

        void BuildBVH();
        bool LoadFromFile(const std::string& filename);
        
        std::vector<Vec4> verticesUVX; // Vertex Data + x coord of uv 
        std::vector<Vec4> normalsUVY;  // Normal Data + y coord of uv

        RadeonRays::Bvh *bvh;
        std::string name;
    };

    class MeshInstance
    {

    public:
        MeshInstance(std::string name, int meshId, Mat4 xform, int matId)
            : name(name)
            , meshID(meshId)
            , transform(xform) 
            , materialID(matId) 
        {
        }
        ~MeshInstance() {}

        Mat4 transform;
        std::string name;

        int materialID;
        int meshID;
    };
}
