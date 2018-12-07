#include "GPUBVH.h"
#include <iostream>

static int current;

int GPUBVH::traverseBVH(BVHNode *root)
{
	AABB *cbox = &root->m_bounds;
	gpuNodes[current].BBoxMin[0] = cbox->min().x;
	gpuNodes[current].BBoxMin[1] = cbox->min().y;
	gpuNodes[current].BBoxMin[2] = cbox->min().z;

	gpuNodes[current].BBoxMax[0] = cbox->max().x;
	gpuNodes[current].BBoxMax[1] = cbox->max().y;
	gpuNodes[current].BBoxMax[2] = cbox->max().z;

	gpuNodes[current].LRLeaf[2] = 0.0f;

	int index = current;

	if (root->isLeaf())
	{
		const LeafNode* leaf = reinterpret_cast<const LeafNode*>(root);
		int start = bvhTriangleIndices.size();

		gpuNodes[current].LRLeaf[0] = start;
		gpuNodes[current].LRLeaf[1] = leaf->m_hi - leaf->m_lo;
		gpuNodes[current].LRLeaf[2] = 1.0f;
		
		for (int i = leaf->m_lo; i < leaf->m_hi; i++)
		{
			int index = bvh->getTriIndices()[i];
			const Vec3i& vtxInds = bvh->getScene()->getTriangle(index).vertices;
			bvhTriangleIndices.push_back(TriangleData{ glm::vec4(vtxInds.x,vtxInds.y,vtxInds.z, index) });
		}
	}
	else
	{
		current++;
		gpuNodes[index].LRLeaf[0] = traverseBVH(root->getChildNode(0));
		current++;
		gpuNodes[index].LRLeaf[1] = traverseBVH(root->getChildNode(1));
	}
	return index;
}

GPUBVH::GPUBVH(const BVH* bvh, const Scene *scene)
{
	this->bvh = bvh;
	this->scene = scene;
	createGPUBVH();
}

void GPUBVH::createGPUBVH()
{
	gpuNodes = new GPUBVHNode[bvh->getNumNodes()];
	traverseBVH(bvh->getRoot());
}
