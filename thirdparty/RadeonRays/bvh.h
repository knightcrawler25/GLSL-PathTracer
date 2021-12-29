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

#ifndef BVH_H
#define BVH_H

#include <memory>
#include <vector>
#include <list>
#include <atomic>
#include <iostream>
#include "bbox.h"

namespace RadeonRays
{
    ///< The class represents bounding volume hierarachy
    ///< intersection accelerator
    ///<
    class Bvh
    {
    public:
        Bvh(float traversal_cost, int num_bins = 64, bool usesah = false)
            : m_root(nullptr)
            , m_num_bins(num_bins)
            , m_usesah(usesah)
            , m_height(0)
            , m_traversal_cost(traversal_cost)
        {
        }

        ~Bvh() = default;

        // World space bounding box
        bbox const& Bounds() const;

        // Build function
        // bounds is an array of bounding boxes
        void Build(bbox const* bounds, int numbounds);

        // Get tree height
        int GetHeight() const;

        // Get reordered prim indices Nodes are pointing to
        virtual int const* GetIndices() const;

        // Get number of indices.
        // This number can differ from numbounds passed to Build function for
        // some BVH implementations (like SBVH)
        virtual size_t GetNumIndices() const;

        // Print BVH statistics
        virtual void PrintStatistics(std::ostream& os) const;
    protected:
        // Build function
        virtual void BuildImpl(bbox const* bounds, int numbounds);
        // BVH node
        struct Node;
        // Node allocation
        virtual Node* AllocateNode();
        virtual void  InitNodeAllocator(size_t maxnum);

        struct SplitRequest
        {
            // Starting index of a request
            int startidx;
            // Number of primitives
            int numprims;
            // Root node
            Node** ptr;
            // Bounding box
            bbox bounds;
            // Centroid bounds
            bbox centroid_bounds;
            // Level
            int level;
            // Node index
            int index;
        };

        struct SahSplit
        {
            int dim;
            float split;
            float sah;
            float overlap;
        };

        void BuildNode(SplitRequest const& req, bbox const* bounds, Vec3 const* centroids, int* primindices);

        SahSplit FindSahSplit(SplitRequest const& req, bbox const* bounds, Vec3 const* centroids, int* primindices) const;

        // Enum for node type
        enum NodeType
        {
            kInternal,
            kLeaf
        };

        // Bvh nodes
        std::vector<Node> m_nodes;
        // Identifiers of leaf primitives
        std::vector<int> m_indices;
        // Node allocator counter, atomic for thread safety
        std::atomic<int> m_nodecnt;

        // Identifiers of leaf primitives
        std::vector<int> m_packed_indices;

        // Bounding box containing all primitives
        bbox m_bounds;
        // Root node
        Node* m_root;
        // SAH flag
        bool m_usesah;
        // Tree height
        int m_height;
        // Node traversal cost
        float m_traversal_cost;
        // Number of spatial bins to use for SAH
        int m_num_bins;


    private:
        Bvh(Bvh const&) = delete;
        Bvh& operator = (Bvh const&) = delete;

		friend class BvhTranslator;
    };

    struct Bvh::Node
    {
        // Node bounds in world space
        bbox bounds;
        // Type of the node
        NodeType type;
        // Node index in a complete tree
        int index;

        union
        {
            // For internal nodes: left and right children
            struct
            {
                Node* lc;
                Node* rc;
            };

            // For leaves: starting primitive index and number of primitives
            struct
            {
                int startidx;
                int numprims;
            };
        };
    };

    inline int const* Bvh::GetIndices() const
    {
        return &m_packed_indices[0];
    }

    inline size_t Bvh::GetNumIndices() const
    {
        return m_packed_indices.size();
    }

    inline int Bvh::GetHeight() const
    {
        return m_height;
    }
}

#endif // BVH_H
