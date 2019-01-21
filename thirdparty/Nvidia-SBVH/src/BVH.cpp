/*
*  Copyright (c) 2009-2011, NVIDIA Corporation
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*      * Redistributions of source code must retain the above copyright
*        notice, this list of conditions and the following disclaimer.
*      * Redistributions in binary form must reproduce the above copyright
*        notice, this list of conditions and the following disclaimer in the
*        documentation and/or other materials provided with the distribution.
*      * Neither the name of NVIDIA Corporation nor the
*        names of its contributors may be used to endorse or promote products
*        derived from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
*  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
*  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
*  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstdio>

#include "BVH.h"
#include "SplitBVHBuilder.h"


BVH::BVH(GPUScene* scene, const Platform& platform, const BuildParams& params)
{
	FW_ASSERT(scene);
	m_scene = scene;
	m_platform = platform;

	if (params.enablePrints)
		printf("BVH builder: %d tris, %d vertices\n", scene->getNumTriangles(), scene->getNumVertices());

	// SplitBVHBuilder() builds the actual BVH
	m_root = SplitBVHBuilder(*this, params).run(m_numNodes);

	if (params.enablePrints)
		printf("BVH: Scene bounds: (%.1f,%.1f,%.1f) - (%.1f,%.1f,%.1f)\n", m_root->m_bounds.min().x, m_root->m_bounds.min().y, m_root->m_bounds.min().z,
		m_root->m_bounds.max().x, m_root->m_bounds.max().y, m_root->m_bounds.max().z);

	float sah = 0.f;
	m_root->computeSubtreeProbabilities(m_platform, 1.f, sah);
	if (params.enablePrints)
		printf("top-down sah: %.2f\n", sah);

	if (params.stats)
	{
		params.stats->SAHCost = sah;
		params.stats->branchingFactor = 2;
		params.stats->numLeafNodes = m_root->getSubtreeSize(BVH_STAT_LEAF_COUNT);
		params.stats->numInnerNodes = m_root->getSubtreeSize(BVH_STAT_INNER_COUNT);
		params.stats->numTris = m_root->getSubtreeSize(BVH_STAT_TRIANGLE_COUNT);
		params.stats->numChildNodes = m_root->getSubtreeSize(BVH_STAT_CHILDNODE_COUNT);
	}
}
