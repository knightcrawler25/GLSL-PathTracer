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

/*
	Please see https://github.com/GPUOpen-LibrariesAndSDKs/RadeonRays_SDK for the original code
	Code is modfied for this project
*/

#include "bvh_translator.h"

#include <cassert>
#include <stack>
#include <iostream>

namespace RadeonRays
{
	int BvhTranslator::ProcessBLASNodes(const Bvh::Node *node)
	{
		RadeonRays::bbox bbox = node->bounds;

		bboxmin[curNode] = bbox.pmin;
		bboxmax[curNode] = bbox.pmax;
		nodes[curNode].leaf = 0;

		int index = curNode;

		if (node->type == RadeonRays::Bvh::NodeType::kLeaf)
		{
			nodes[curNode].leftIndex = curTriIndex + node->startidx;
			nodes[curNode].rightIndex = node->numprims;
			nodes[curNode].leaf = 1;
		}
		else
		{
			curNode++;
			nodes[index].leftIndex = ProcessBLASNodes(node->lc);
			nodes[index].leftIndex = ((nodes[index].leftIndex % nodeTexWidth) << 12) | (nodes[index].leftIndex / nodeTexWidth);
			curNode++;
			nodes[index].rightIndex = ProcessBLASNodes(node->rc);
			nodes[index].rightIndex = ((nodes[index].rightIndex % nodeTexWidth) << 12) | (nodes[index].rightIndex / nodeTexWidth);
		}
		return index;
	}

	int BvhTranslator::ProcessTLASNodes(const Bvh::Node *node)
	{
		RadeonRays::bbox bbox = node->bounds;

		bboxmin[curNode] = bbox.pmin;
		bboxmax[curNode] = bbox.pmax;
		nodes[curNode].leaf = 0;

		int index = curNode;

		if (node->type == RadeonRays::Bvh::NodeType::kLeaf)
		{
			int instanceIndex = TLBvh->m_packed_indices[node->startidx];
			int meshIndex = meshInstances[instanceIndex].meshID;
			int materialID = meshInstances[instanceIndex].materialID;

			nodes[curNode].leftIndex = (bvhRootStartIndices[meshIndex] % nodeTexWidth) << 12 | (bvhRootStartIndices[meshIndex] / nodeTexWidth);
			nodes[curNode].rightIndex = materialID;
			nodes[curNode].leaf = -instanceIndex - 1;
		}
		else
		{
			curNode++;
			nodes[index].leftIndex = ProcessTLASNodes(node->lc);
			nodes[index].leftIndex = ((nodes[index].leftIndex % nodeTexWidth) << 12) | (nodes[index].leftIndex / nodeTexWidth);
			curNode++;
			nodes[index].rightIndex = ProcessTLASNodes(node->rc);
			nodes[index].rightIndex = ((nodes[index].rightIndex % nodeTexWidth) << 12) | (nodes[index].rightIndex / nodeTexWidth);
		}
		return index;
	}
	
	void BvhTranslator::ProcessBLAS()
	{
		int nodeCnt = 0;

		for (int i = 0; i < meshes.size(); i++)
			nodeCnt += meshes[i]->bvh->m_nodecnt;
		topLevelIndex = nodeCnt;

		// reserve space for top level nodes
		nodeCnt += 2 * meshInstances.size();
		nodeTexWidth = (int)(sqrt(nodeCnt) + 1);

		// Resize to power of 2
		bboxmin.resize(nodeTexWidth * nodeTexWidth);
		bboxmax.resize(nodeTexWidth * nodeTexWidth);
		nodes.resize(nodeTexWidth * nodeTexWidth);

		int bvhRootIndex = 0;
		curTriIndex = 0;

		for (int i = 0; i < meshes.size(); i++)
		{
			GLSLPT::Mesh *mesh = meshes[i];
			curNode = bvhRootIndex;

			bvhRootStartIndices.push_back(bvhRootIndex);
			bvhRootIndex += mesh->bvh->m_nodecnt;
			
			ProcessBLASNodes(mesh->bvh->m_root);
			curTriIndex += mesh->bvh->GetNumIndices();
		}
	}

	void BvhTranslator::ProcessTLAS()
	{
		curNode = topLevelIndex;
		topLevelIndexPackedXY = ((topLevelIndex % nodeTexWidth) << 12) | (topLevelIndex / nodeTexWidth);
		ProcessTLASNodes(TLBvh->m_root);
	}

	void BvhTranslator::UpdateTLAS(const Bvh *topLevelBvh, const std::vector<GLSLPT::MeshInstance> &sceneInstances)
	{
		TLBvh = topLevelBvh;
		meshInstances = sceneInstances;
		curNode = topLevelIndex;
		ProcessTLASNodes(TLBvh->m_root);
	}

	void BvhTranslator::Process(const Bvh *topLevelBvh, const std::vector<GLSLPT::Mesh*> &sceneMeshes,const std::vector<GLSLPT::MeshInstance> &sceneInstances)
	{
		TLBvh = topLevelBvh;
		meshes = sceneMeshes;
		meshInstances = sceneInstances;
		ProcessBLAS();
		ProcessTLAS();
	}
}
