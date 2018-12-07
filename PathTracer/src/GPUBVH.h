#pragma once

#include "BVH.h"
#include <glm/glm.hpp>
#include "Scene.h"

struct GPUBVHNode
{
	glm::vec3 BBoxMin;
	glm::vec3 BBoxMax;
	glm::vec3 LRLeaf;
};

class GPUBVH
{
public:
	GPUBVH(const BVH *bvh,const Scene *scene);
	void createGPUBVH();
	int traverseBVH(BVHNode *root);
	GPUBVHNode *gpuNodes;
	const BVH *bvh;
	const Scene *scene;
	std::vector<TriangleData> bvhTriangleIndices;
};
