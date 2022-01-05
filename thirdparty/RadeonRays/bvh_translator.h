/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

//	Modified version of code from https://github.com/GPUOpen-LibrariesAndSDKs/RadeonRays_SDK 

#pragma once

#ifndef BVH_TRANSLATOR_H
#define BVH_TRANSLATOR_H

#include <map>
#include "bvh.h"
#include "Mesh.h"

namespace RadeonRays
{
    /// This class translates pointer based BVH representation into
    /// index based one suitable for feeding to GPU or any other accelerator
    //
    class BvhTranslator
    {
    public:
        // Constructor
        BvhTranslator() = default;

        struct Node
        {
            Vec3 bboxmin;
            Vec3 bboxmax;
            Vec3 LRLeaf;
        };

        void ProcessBLAS();
        void ProcessTLAS();
        void UpdateTLAS(const Bvh* topLevelBvh, const std::vector<GLSLPT::MeshInstance>& instances);
        void Process(const Bvh* topLevelBvh, const std::vector<GLSLPT::Mesh*>& meshes, const std::vector<GLSLPT::MeshInstance>& instances);
        int topLevelIndex = 0;
        std::vector<Node> nodes;
        int nodeTexWidth;

    private:
        int curNode = 0;
        int curTriIndex = 0;
        std::vector<int> bvhRootStartIndices;
        int ProcessBLASNodes(const Bvh::Node* root);
        int ProcessTLASNodes(const Bvh::Node* root);
        std::vector<GLSLPT::MeshInstance> meshInstances;
        std::vector<GLSLPT::Mesh*> meshes;
        const Bvh* topLevelBvh;
    };
}

#endif // BVH_TRANSLATOR_H
