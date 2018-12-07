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
#include "Array.h"
#include "Util.h"

enum BVH_STAT
{
	BVH_STAT_NODE_COUNT,
	BVH_STAT_INNER_COUNT,
	BVH_STAT_LEAF_COUNT,
	BVH_STAT_TRIANGLE_COUNT,
	BVH_STAT_CHILDNODE_COUNT,
};

class AABB
{
public:													
	inline                    AABB(void) : m_mn(FW_F32_MAX, FW_F32_MAX, FW_F32_MAX), m_mx(-FW_F32_MAX, -FW_F32_MAX, -FW_F32_MAX) {}
	inline                    AABB(const Vec3f& mn, const Vec3f& mx) : m_mn(mn), m_mx(mx) {}

	inline    void            grow(const Vec3f& pt)   { m_mn = min3f(m_mn, pt); m_mx = max3f(m_mx, pt); } // grows bounds to include 3d point pt
	inline    void            grow(const AABB& aabb)  { grow(aabb.m_mn); grow(aabb.m_mx); }
	inline    void            intersect(const AABB& aabb)  { m_mn = max3f(m_mn, aabb.m_mn); m_mx = min3f(m_mx, aabb.m_mx); } /// box formed by intersection of 2 AABB boxes
	inline    float           volume(void) const      { if (!valid()) return 0.0f; return (m_mx.x - m_mn.x) * (m_mx.y - m_mn.y) * (m_mx.z - m_mn.z); } /// volume = AABB side along X-axis * side along Y * side along Z
	inline    float           area(void) const        { if (!valid()) return 0.0f; Vec3f d = m_mx - m_mn; return (d.x*d.y + d.y*d.z + d.z*d.x)*2.0f; }
	inline    bool            valid(void) const       { return m_mn.x <= m_mx.x && m_mn.y <= m_mx.y && m_mn.z <= m_mx.z; }
	inline    Vec3f           midPoint(void) const    { return (m_mn + m_mx)*0.5f; } // AABB centroid or midpoint
	inline    const Vec3f&    min(void) const         { return m_mn; }
	inline    const Vec3f&    max(void) const         { return m_mx; }
	inline    Vec3f&          min(void)               { return m_mn; }
	inline    Vec3f&          max(void)               { return m_mx; }

	inline    AABB            operator+   (const AABB& aabb) const { AABB u(*this); u.grow(aabb); return u; }

private:
	Vec3f           m_mn; // AABB min bound 
	Vec3f           m_mx; // AABB max bound
};

class BVHNode
{
public:
	BVHNode() : m_probability(1.f), m_parentProbability(1.f), m_treelet(-1), m_index(-1) {} 
	virtual bool        isLeaf() const = 0;               
	virtual S32         getNumChildNodes() const = 0;
	virtual BVHNode*    getChildNode(S32 i) const = 0;
	virtual S32         getNumTriangles() const { return 0; }

	float       getArea() const     { return m_bounds.area(); }

	AABB        m_bounds;

	// These are somewhat experimental, for some specific test and may be invalid...
	float       m_probability;          // probability of coming here (widebvh uses this)
	float       m_parentProbability;    // probability of coming to parent (widebvh uses this)

	int         m_treelet;              // for queuing tests (qmachine uses this)
	int         m_index;                // in linearized tree (qmachine uses this)

	// Subtree functions
	int     getSubtreeSize(BVH_STAT stat = BVH_STAT_NODE_COUNT) const;
	void    computeSubtreeProbabilities(const Platform& p, float parentProbability, float& sah);
	float   computeSubtreeSAHCost(const Platform& p) const;     // NOTE: assumes valid probabilities
	void    deleteSubtree();

	void    assignIndicesDepthFirst(S32 index = 0, bool includeLeafNodes = true);
	void    assignIndicesBreadthFirst(S32 index = 0, bool includeLeafNodes = true);
};


class InnerNode : public BVHNode
{
public:
	InnerNode(const AABB& bounds, BVHNode* child0, BVHNode* child1)   { m_bounds = bounds; m_children[0] = child0; m_children[1] = child1; }

	bool        isLeaf() const                  { return false; }
	S32         getNumChildNodes() const        { return 2; }
	BVHNode*    getChildNode(S32 i) const       { FW_ASSERT(i >= 0 && i<2); return m_children[i]; }

	BVHNode*    m_children[2];  
};


class LeafNode : public BVHNode
{
public:
	LeafNode(const AABB& bounds, int lo, int hi)  { m_bounds = bounds; m_lo = lo; m_hi = hi; } 
	LeafNode(const LeafNode& s)                 { *this = s; }

	bool        isLeaf() const                  { return true; }
	S32         getNumChildNodes() const        { return 0; }  // leafnode has 0 children
	BVHNode*    getChildNode(S32) const         { return NULL; }

	S32         getNumTriangles() const         { return m_hi - m_lo; }
	S32         m_lo;  // lower index in triangle list
	S32         m_hi;  // higher index in triangle list
};

