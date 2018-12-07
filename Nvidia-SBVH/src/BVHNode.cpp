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

#include "BVHNode.h"
#include "Array.h"


int BVHNode::getSubtreeSize(BVH_STAT stat) const  // recursively counts some type of nodes (either leafnodes, innernodes, childnodes) or unmber of triangles
	{
		int cnt;

		switch (stat)
		{
		default: FW_ASSERT(0);  // unknown mode			
		case BVH_STAT_NODE_COUNT:      cnt = 1; break; // counts all nodes including leafnodes
		case BVH_STAT_LEAF_COUNT:      cnt = isLeaf() ? 1 : 0; break; // counts only leafnodes
		case BVH_STAT_INNER_COUNT:     cnt = isLeaf() ? 0 : 1; break; // counts only innernodes
		case BVH_STAT_TRIANGLE_COUNT:  cnt = isLeaf() ? reinterpret_cast<const LeafNode*>(this)->getNumTriangles() : 0; break; // counts all triangles
		case BVH_STAT_CHILDNODE_COUNT: cnt = getNumChildNodes(); break; ///counts only childnodes
		}

		if (!isLeaf()) // if current node is not a leaf node, continue counting its childnodes recursively
		{
			for (int i = 0; i<getNumChildNodes(); i++)
				cnt += getChildNode(i)->getSubtreeSize(stat); 
		}

		return cnt;
	}


	void BVHNode::deleteSubtree()
	{
		for (int i = 0; i<getNumChildNodes(); i++)
			getChildNode(i)->deleteSubtree(); 

		delete this;
	}


	void BVHNode::computeSubtreeProbabilities(const Platform& p, float probability, float& sah)
	{
		sah += probability * p.getCost(this->getNumChildNodes(), this->getNumTriangles());

		m_probability = probability;

		// recursively compute probabilities and add to SAH
		for (int i = 0; i<getNumChildNodes(); i++)
		{
			BVHNode* child = getChildNode(i);
			child->m_parentProbability = probability;           /// childnode area / parentnode area
			child->computeSubtreeProbabilities(p, probability * child->m_bounds.area() / this->m_bounds.area(), sah);
		}
	}


	// TODO: requires valid probabilities...
	float BVHNode::computeSubtreeSAHCost(const Platform& p) const
	{
		float SAH = m_probability * p.getCost(getNumChildNodes(), getNumTriangles());

		for (int i = 0; i<getNumChildNodes(); i++)
			SAH += getChildNode(i)->computeSubtreeSAHCost(p);

		return SAH;
	}

	//-------------------------------------------------------------

	void assignIndicesDepthFirstRecursive(BVHNode* node, S32& index, bool includeLeafNodes)
	{
		if (node->isLeaf() && !includeLeafNodes)
			return;

		node->m_index = index++;
		for (int i = 0; i<node->getNumChildNodes(); i++)
			assignIndicesDepthFirstRecursive(node->getChildNode(i), index, includeLeafNodes);
	}

	void BVHNode::assignIndicesDepthFirst(S32 index, bool includeLeafNodes)
	{
		assignIndicesDepthFirstRecursive(this, index, includeLeafNodes); 
	}

	//-------------------------------------------------------------

	void BVHNode::assignIndicesBreadthFirst(S32 index, bool includeLeafNodes)
	{
		Array<BVHNode*> nodes;  // array acts like a stack 
		nodes.add(this);
		S32 head = 0;

		while (head < nodes.getSize())
		{
			// pop
			BVHNode* node = nodes[head++];

			// discard
			if (node->isLeaf() && !includeLeafNodes)
				continue;

			// assign
			node->m_index = index++;

			// push children
			for (int i = 0; i<node->getNumChildNodes(); i++)
				nodes.add(node->getChildNode(i));
		}
	}

