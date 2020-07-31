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

#ifndef BBOX_H
#define BBOX_H

#include <cmath>
#include <algorithm>
#include <limits>

#include <Config.h>
#include <Mat4.h>
#include <Vec2.h>
#include <Vec3.h>
#include <Vec4.h>

using namespace GLSLPT;

namespace RadeonRays
{
    class bbox
    {
    public:
        bbox()
            : pmin(Vec3(std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max()))
            , pmax(Vec3(-std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max()))
        {
        }

        bbox(Vec3 const& p)
            : pmin(p)
            , pmax(p)
        {
        }

        bbox(Vec3 const& p1, Vec3 const& p2)
            : pmin(Vec3::Min(p1, p2))
            , pmax(Vec3::Max(p1, p2))
        {
        }

		Vec3 center()  const;
		Vec3 extents() const;

        bool contains(Vec3 const& p) const;

		inline int maxdim() const
		{
			Vec3 ext = extents();

			if (ext.x >= ext.y && ext.x >= ext.z)
				return 0;
			if (ext.y >= ext.x && ext.y >= ext.z)
				return 1;
			if (ext.z >= ext.x && ext.z >= ext.y)
				return 2;

			return 0;
		}

		float surface_area() const;

        // TODO: this is non-portable, optimization trial for fast intersection test
        Vec3 const& operator [] (int i) const { return *(&pmin + i); }

        // Grow the bounding box by a point
		void grow(Vec3 const& p);
        // Grow the bounding box by a box
		void grow(bbox const& b);

        Vec3 pmin;
        Vec3 pmax;
    };

	bbox bboxunion(bbox const& box1, bbox const& box2);
	bbox intersection(bbox const& box1, bbox const& box2);
	void intersection(bbox const& box1, bbox const& box2, bbox& box);
	bool intersects(bbox const& box1, bbox const& box2);
	bool contains(bbox const& box1, bbox const& box2);
}

#endif