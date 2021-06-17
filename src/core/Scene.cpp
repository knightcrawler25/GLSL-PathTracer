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

#include <iostream>

#include "Scene.h"
#include "Camera.h"

#include <ctime>

namespace GLSLPT
{
    void Scene::AddCamera(Vec3 pos, Vec3 lookAt, float fov)
    {
        delete camera;
        camera = new Camera(pos, lookAt, fov);
    }

    int Scene::AddMesh(const std::string& filename)
    {
        int id = -1;
        // Check if mesh was already loaded
        for (int i = 0; i < meshes.size(); i++)
            if (meshes[i]->name == filename)
                return i;
            
        id = meshes.size();

        Mesh* mesh = new Mesh;
		
        printf("Loading %s ...\n", filename.c_str());
        if (mesh->LoadFromFile(filename))
        {
            meshes.push_back(mesh);
            printf("Model %s loaded\n", filename.c_str());
        }
        else
            id = -1;
        return id;
    }

    int Scene::AddTexture(const std::string& filename)
    {
        int id = -1;
        // Check if texture was already loaded
        for (int i = 0; i < textures.size(); i++)
            if (textures[i]->name == filename)
                return i;

        id = textures.size();

        Texture* texture = new Texture;

        if (texture->LoadTexture(filename))
        {
            textures.push_back(texture);
            printf("Texture %s loaded\n", filename.c_str());
        }
        else
            id = -1;

        return id;
    }

    int Scene::AddMaterial(const Material& material)
    {
        int id = materials.size();
        materials.push_back(material);
        return id;
    }

    void Scene::AddHDR(const std::string& filename)
    {
        delete hdrData;
		
		clock_t time1, time2;
		
		time1 = clock();
		
        hdrData = HDRLoader::load(filename.c_str());
        if (hdrData == nullptr)
            printf("Unable to load HDR\n");
        else
        {
            printf("HDR %s loaded\n", filename.c_str());
            renderOptions.useEnvMap = true;
        }
		
		time2 = clock();
		printf("%.1fs\n", (float)(time2-time1)/(float)CLOCKS_PER_SEC);
    }

    int Scene::AddMeshInstance(const MeshInstance &meshInstance)
    {
        int id = meshInstances.size();
        meshInstances.push_back(meshInstance);
        return id;
    }

    int Scene::AddLight(const Light &light)
    {
        int id = lights.size();
        lights.push_back(light);
        return id;
    }

    void Scene::createTLAS()
    {
        // Loop through all the mesh Instances and build a Top Level BVH
        std::vector<RadeonRays::bbox> bounds;
        bounds.resize(meshInstances.size());
		
		printf("mesh Instances %ld\n", meshInstances.size());

        #pragma omp parallel for
        for (int i = 0; i < meshInstances.size(); i++)
        {
            RadeonRays::bbox bbox = meshes[meshInstances[i].meshID]->bvh->Bounds();
            Mat4 matrix = meshInstances[i].transform;

            Vec3 minBound = bbox.pmin;
            Vec3 maxBound = bbox.pmax;

            Vec3 right       = Vec3(matrix[0][0], matrix[0][1], matrix[0][2]);
            Vec3 up          = Vec3(matrix[1][0], matrix[1][1], matrix[1][2]);
            Vec3 forward     = Vec3(matrix[2][0], matrix[2][1], matrix[2][2]);
            Vec3 translation = Vec3(matrix[3][0], matrix[3][1], matrix[3][2]);

            Vec3 xa = right * minBound.x;
            Vec3 xb = right * maxBound.x;

            Vec3 ya = up * minBound.y;
            Vec3 yb = up * maxBound.y;

            Vec3 za = forward * minBound.z;
            Vec3 zb = forward * maxBound.z;

            minBound = Vec3::Min(xa, xb) + Vec3::Min(ya, yb) + Vec3::Min(za, zb) + translation;
            maxBound = Vec3::Max(xa, xb) + Vec3::Max(ya, yb) + Vec3::Max(za, zb) + translation;

            RadeonRays::bbox bound;
            bound.pmin = minBound;
            bound.pmax = maxBound;

            bounds[i] = bound;
        }
        sceneBvh->Build(&bounds[0], bounds.size());
        sceneBounds = sceneBvh->Bounds();
    }

    void Scene::createBLAS()
    {
		totalTris = 0;
		
        // Loop through all meshes and build BVHs
        #pragma omp parallel for
        for (int i = 0; i < meshes.size(); i++)
        {
            printf("Building BVH for %s\n", meshes[i]->name.c_str());
            totalTris += meshes[i]->BuildBVH();
        }	
		
		printf("Total %ld tris loaded\n", totalTris);
    }
    
    void Scene::RebuildInstances()
    {
        delete sceneBvh;
        sceneBvh = new RadeonRays::Bvh(10.0f, 64, false);

        createTLAS();
        bvhTranslator.UpdateTLAS(sceneBvh, meshInstances);

        //Copy transforms
        for (int i = 0; i < meshInstances.size(); i++)
            transforms[i] = meshInstances[i].transform;

        instancesModified = true;
    }

    void Scene::CreateAccelerationStructures()
    {
		clock_t time1, time2;
		
		time1 = clock();
		
        createBLAS();

        printf("Building scene BVH\n");
        createTLAS();

        // Flatten BVH
        bvhTranslator.Process(sceneBvh, meshes, meshInstances);

        int verticesCnt = 0;

        //Copy mesh data
        for (int i = 0; i < meshes.size(); i++)
        {
            // Copy indices from BVH and not from Mesh
            int numIndices = meshes[i]->bvh->GetNumIndices();
            const int * triIndices = meshes[i]->bvh->GetIndices();

            for (int j = 0; j < numIndices; j++)
            {
                int index = triIndices[j];
                int index3 = index + index + index;
                int v1 = (index3) + verticesCnt;
                int v2 = (index3 + 1) + verticesCnt;
                int v3 = (index3 + 2) + verticesCnt;

                vertIndices.push_back(Indices{ v1, v2, v3 });
            }

            verticesUVX.insert(verticesUVX.end(), meshes[i]->verticesUVX.begin(), meshes[i]->verticesUVX.end());
            normalsUVY.insert(normalsUVY.end(), meshes[i]->normalsUVY.begin(), meshes[i]->normalsUVY.end());

            verticesCnt += meshes[i]->verticesUVX.size();
        }

        //Copy transforms
        transforms.resize(meshInstances.size());
        #pragma omp parallel for
        for (int i = 0; i < meshInstances.size(); i++)
            transforms[i] = meshInstances[i].transform;

        //Copy Textures
        for (int i = 0; i < textures.size(); i++)
        {
            texWidth = textures[i]->width;
            texHeight = textures[i]->height;
            int texSize = texWidth * texHeight;
            textureMapsArray.insert(textureMapsArray.end(), &textures[i]->texData[0], &textures[i]->texData[texSize * 3]);
        }
		
		time2 = clock();
		printf("%.1fs\n", (float)(time2-time1)/(float)CLOCKS_PER_SEC);
    }
}