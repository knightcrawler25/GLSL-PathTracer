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

#include <bbox.h>

namespace RadeonRays
{
	glm::vec3 bbox::center()  const { return 0.5f * (pmax + pmin); }
	glm::vec3 bbox::extents() const { return pmax - pmin; }

	float bbox::surface_area() const
	{
		glm::vec3 ext = extents();
		return 2.f * (ext.x * ext.y + ext.x * ext.z + ext.y * ext.z);
	}

	// Grow the bounding box by a point
	void bbox::grow(glm::vec3 const& p)
	{
		pmin = glm::min(pmin, p);
		pmax = glm::max(pmax, p);
	}
	// Grow the bounding box by a box
	void bbox::grow(bbox const& b)
	{
		pmin = glm::min(pmin, b.pmin);
		pmax = glm::max(pmax, b.pmax);
	}

	bool bbox::contains(glm::vec3 const& p) const
	{
		glm::vec3 radius = 0.5f * extents();
		return std::abs(center().x - p.x) <= radius.x &&
			fabs(center().y - p.y) <= radius.y &&
			fabs(center().z - p.z) <= radius.z;
	}

	bbox bboxunion(bbox const& box1, bbox const& box2)
	{
		bbox res;
		res.pmin = glm::min(box1.pmin, box2.pmin);
		res.pmax = glm::max(box1.pmax, box2.pmax);
		return res;
	}

	bbox intersection(bbox const& box1, bbox const& box2)
	{
		return bbox(glm::max(box1.pmin, box2.pmin), glm::min(box1.pmax, box2.pmax));
	}

	void intersection(bbox const& box1, bbox const& box2, bbox& box)
	{
		box.pmin = glm::max(box1.pmin, box2.pmin);
		box.pmax = glm::min(box1.pmax, box2.pmax);
	}

	#define BBOX_INTERSECTION_EPS 0.f

	bool intersects(bbox const& box1, bbox const& box2)
	{
		glm::vec3 b1c = box1.center();
		glm::vec3 b1r = 0.5f * box1.extents();
		glm::vec3 b2c = box2.center();
		glm::vec3 b2r = 0.5f * box2.extents();

		return (fabs(b2c.x - b1c.x) - (b1r.x + b2r.x)) <= BBOX_INTERSECTION_EPS &&
			(fabs(b2c.y - b1c.y) - (b1r.y + b2r.y)) <= BBOX_INTERSECTION_EPS &&
			(fabs(b2c.z - b1c.z) - (b1r.z + b2r.z)) <= BBOX_INTERSECTION_EPS;
	}

	bool contains(bbox const& box1, bbox const& box2)
	{
		return box1.contains(box2.pmin) && box1.contains(box2.pmax);
	}

	
}
