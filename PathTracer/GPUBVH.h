#pragma once

#include "BVH.h"
#include <glm/glm.hpp>
#include <vector>

namespace GLSLPathTracer
{
    struct GPUBVHNode
    {
        glm::vec3 BBoxMin;
        glm::vec3 BBoxMax;
        glm::vec3 LRLeaf;
    };

    struct TriIndexData
    {
        glm::vec4 indices;
    };

    class GPUBVH
    {
    public:
        GPUBVH(const BVH *bvh);
		~GPUBVH();
        void createGPUBVH();
        int traverseBVH(BVHNode *root);
        GPUBVHNode *gpuNodes;
        const BVH *bvh;
		int current;
        std::vector<TriIndexData> bvhTriangleIndices;
    };
}