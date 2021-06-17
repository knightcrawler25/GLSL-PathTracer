/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this softwareand associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "Mesh.h"
#include <iostream>

namespace GLSLPT
{
    bool Mesh::LoadFromFile(const std::string &filename)
    {
		clock_t time1, time2;
		
		time1 = clock();
		
        name = filename;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), 0, true);

        if (!ret)
        {
            printf("Unable to load model\n");
            return false;
        }

        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++) 
        {
            // Loop over faces(polygon)
            size_t index_offset = 0;

            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
            {
                int fv = shapes[s].mesh.num_face_vertices[f];
                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++)
                {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					
                    size_t idx3 = idx.vertex_index + idx.vertex_index + idx.vertex_index;
                    tinyobj::real_t vx = attrib.vertices[idx3];
                    tinyobj::real_t vy = attrib.vertices[idx3 + 1];
                    tinyobj::real_t vz = attrib.vertices[idx3 + 2];
					
					idx3 = idx.normal_index + idx.normal_index + idx.normal_index;
                    tinyobj::real_t nx = attrib.normals[idx3];
                    tinyobj::real_t ny = attrib.normals[idx3 + 1];
                    tinyobj::real_t nz = attrib.normals[idx3 + 2];

                    tinyobj::real_t tx, ty;
                    
                    // temporary fix
                    if (!attrib.texcoords.empty())
                    {
                        tx = attrib.texcoords[2 * idx.texcoord_index];
                        ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                    }
                    else
                    {
                        tx = ty = 0;
                    }

                    verticesUVX.push_back(Vec4(vx, vy, vz, tx));
                    normalsUVY.push_back(Vec4(nx, ny, nz, ty));
                }
    
                index_offset += fv;
            }
        }
		
		time2 = clock();
		printf("%.1fs\n", (float)(time2-time1)/(float)CLOCKS_PER_SEC);

        return true;
    }

    size_t Mesh::BuildBVH()
    {		
		clock_t time1, time2;
		
		time1 = clock();
		
        const size_t numTris = verticesUVX.size() / 3;
		
		printf("%ld tris\n", numTris);
		
        std::vector<RadeonRays::bbox> bounds(numTris);
		
		size_t i3 = 0;

        #pragma omp parallel for
        for (size_t i = 0; i < numTris; ++i)
        {
            const Vec3 v1 = Vec3(verticesUVX[i3]);
            const Vec3 v2 = Vec3(verticesUVX[i3 + 1]);
            const Vec3 v3 = Vec3(verticesUVX[i3 + 2]);

            bounds[i].grow(v1);
            bounds[i].grow(v2);
            bounds[i].grow(v3);
			
			i3 += 3;
        }

        bvh->Build(&bounds[0], numTris);		
		
		time2 = clock();
		printf("%.1fs\n", (float)(time2-time1)/(float)CLOCKS_PER_SEC);

        return numTris;
    }
}