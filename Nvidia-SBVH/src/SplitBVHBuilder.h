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

#pragma once
#include "BVH.h"
#include "Timer.h"

class SplitBVHBuilder
{
private:
	enum
	{
		MaxDepth = 64,
		MaxSpatialDepth = 48,
		NumSpatialBins = 32,
	};

	struct Reference   /// a AABB bounding box enclosing 1 triangle, a reference can be duplicated by a split to be contained in 2 AABB boxes
	{
		S32                 triIdx;
		AABB                bounds;

		Reference(void) : triIdx(-1) {}  /// constructor
	};

	struct NodeSpec
	{
		S32                 numRef;   // number of references contained by node
		AABB                bounds;

		NodeSpec(void) : numRef(0) {}
	};

	struct ObjectSplit
	{
		F32                 sah;   // cost
		S32                 sortDim;  // axis along which triangles are sorted
		S32                 numLeft;  // number of triangles (references) in left child
		AABB                leftBounds;
		AABB                rightBounds;

		ObjectSplit(void) : sah(FW_F32_MAX), sortDim(0), numLeft(0) {}
	};

	struct SpatialSplit
	{
		F32                 sah;
		S32                 dim;   /// split axis
		F32                 pos;   /// position of split along axis (dim)

		SpatialSplit(void) : sah(FW_F32_MAX), dim(0), pos(0.0f) {}
	};

	struct SpatialBin
	{
		AABB                bounds;
		S32                 enter;
		S32                 exit;
	};

public:
	SplitBVHBuilder(BVH& bvh, const BVH::BuildParams& params);
	~SplitBVHBuilder(void);

	BVHNode*                run(int &numNodes);

private:
	static int              sortCompare(void* data, int idxA, int idxB);
	static void             sortSwap(void* data, int idxA, int idxB);

	BVHNode*                buildNode(const NodeSpec& spec, int level, F32 progressStart, F32 progressEnd);
	BVHNode*                createLeaf(const NodeSpec& spec);

	ObjectSplit             findObjectSplit(const NodeSpec& spec, F32 nodeSAH);
	void                    performObjectSplit(NodeSpec& left, NodeSpec& right, const NodeSpec& spec, const ObjectSplit& split);

	SpatialSplit            findSpatialSplit(const NodeSpec& spec, F32 nodeSAH);
	void                    performSpatialSplit(NodeSpec& left, NodeSpec& right, const NodeSpec& spec, const SpatialSplit& split);
	void                    splitReference(Reference& left, Reference& right, const Reference& ref, int dim, F32 pos);

private:
	SplitBVHBuilder(const SplitBVHBuilder&); // forbidden
	SplitBVHBuilder&        operator=           (const SplitBVHBuilder&); // forbidden

private:
	BVH&                    m_bvh;
	const Platform&         m_platform;
	const BVH::BuildParams& m_params;

	Array<Reference>      m_refStack;
	F32                     m_minOverlap;
	Array<AABB>           m_rightBounds;
	S32                     m_sortDim;
	SpatialBin              m_bins[3][NumSpatialBins];

	FW::Timer               m_progressTimer;
	S32                     m_numDuplicates;
	int						m_numNodes;
};

	