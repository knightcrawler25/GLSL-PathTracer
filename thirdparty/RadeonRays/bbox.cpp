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

#include "bbox.h"

namespace RadeonRays
{
	Vec3 bbox::center()  const { return (pmax + pmin) * 0.5f; }
	Vec3 bbox::extents() const { return pmax - pmin; }

	float bbox::surface_area() const
	{
		Vec3 ext = extents();
		return 2.f * (ext.x * ext.y + ext.x * ext.z + ext.y * ext.z);
	}

	// Grow the bounding box by a point
	void bbox::grow(Vec3 const& p)
	{
		pmin = Vec3::Min(pmin, p);
		pmax = Vec3::Max(pmax, p);
	}
	// Grow the bounding box by a box
	void bbox::grow(bbox const& b)
	{
		pmin = Vec3::Min(pmin, b.pmin);
		pmax = Vec3::Max(pmax, b.pmax);
	}

	bool bbox::contains(Vec3 const& p) const
	{
		Vec3 radius = extents() * 0.5f;
		return std::abs(center().x - p.x) <= radius.x &&
			fabs(center().y - p.y) <= radius.y &&
			fabs(center().z - p.z) <= radius.z;
	}

	bbox bboxunion(bbox const& box1, bbox const& box2)
	{
		bbox res;
		res.pmin = Vec3::Min(box1.pmin, box2.pmin);
		res.pmax = Vec3::Max(box1.pmax, box2.pmax);
		return res;
	}

	bbox intersection(bbox const& box1, bbox const& box2)
	{
		return bbox(Vec3::Max(box1.pmin, box2.pmin), Vec3::Min(box1.pmax, box2.pmax));
	}

	void intersection(bbox const& box1, bbox const& box2, bbox& box)
	{
		box.pmin = Vec3::Max(box1.pmin, box2.pmin);
		box.pmax = Vec3::Min(box1.pmax, box2.pmax);
	}

	#define BBOX_INTERSECTION_EPS 0.f

	bool intersects(bbox const& box1, bbox const& box2)
	{
		Vec3 b1c = box1.center();
		Vec3 b1r = box1.extents() * 0.5f;
		Vec3 b2c = box2.center();
		Vec3 b2r = box2.extents() * 0.5f;

		return (fabs(b2c.x - b1c.x) - (b1r.x + b2r.x)) <= BBOX_INTERSECTION_EPS &&
			(fabs(b2c.y - b1c.y) - (b1r.y + b2r.y)) <= BBOX_INTERSECTION_EPS &&
			(fabs(b2c.z - b1c.z) - (b1r.z + b2r.z)) <= BBOX_INTERSECTION_EPS;
	}

	bool contains(bbox const& box1, bbox const& box2)
	{
		return box1.contains(box2.pmin) && box1.contains(box2.pmax);
	}

	
}
