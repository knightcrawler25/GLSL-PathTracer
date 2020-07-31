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
#pragma once

#include "bvh.h"

namespace RadeonRays
{
    class SplitBvh : public Bvh
    {
    public:
        SplitBvh(float traversal_cost,
                 int num_bins,
                 int max_split_depth, 
                 float min_overlap,
                 float extra_refs_budget)
        : Bvh(traversal_cost, num_bins, true)
        , m_max_split_depth(max_split_depth)
        , m_min_overlap(min_overlap)
        , m_extra_refs_budget(extra_refs_budget)
        , m_num_nodes_required(0)
        , m_num_nodes_for_regular(0)
        , m_num_nodes_archived(0)
        {
        }

        ~SplitBvh() = default;

    protected:
        struct PrimRef;
        using PrimRefArray = std::vector<PrimRef>;
        
        enum class SplitType
        {
            kObject,
            kSpatial
        };

        // Build function
        void BuildImpl(bbox const* bounds, int numbounds) override;
        void BuildNode(SplitRequest& req, PrimRefArray& primrefs);
        
        SahSplit FindObjectSahSplit(SplitRequest const& req, PrimRefArray const& refs) const;
        SahSplit FindSpatialSahSplit(SplitRequest const& req, PrimRefArray const& refs) const;
        
        void SplitPrimRefs(SahSplit const& split, SplitRequest const& req, PrimRefArray& refs, int& extra_refs);
        bool SplitPrimRef(PrimRef const& ref, int axis, float split, PrimRef& leftref, PrimRef& rightref) const;

        // Print BVH statistics
        void PrintStatistics(std::ostream& os) const override;

    protected:
        Node* AllocateNode() override;
        void  InitNodeAllocator(size_t maxnum) override;

    private:

        int m_max_split_depth;
        float m_min_overlap;
        float m_extra_refs_budget;
        int m_num_nodes_required;
        int m_num_nodes_for_regular;

        // Node archive for memory management
        // As m_nodes fills up we archive it into m_node_archive
        // allocate new chunk and work there.

        // How many nodes have been archived so far
        int m_num_nodes_archived;
        // Container for archived chunks
        std::list<std::vector<Node>> m_node_archive;

        SplitBvh(SplitBvh const&) = delete;
        SplitBvh& operator = (SplitBvh const&) = delete;
    };
    
    struct SplitBvh::PrimRef
    {
        // Prim bounds
        bbox bounds;
        Vec3 center;
        int idx;
    };

}