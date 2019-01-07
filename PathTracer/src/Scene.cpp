#include <iostream>

#include "Scene.h"
#include "Camera.h"

namespace GLSLPathTracer
{
    void Scene::addCamera(glm::vec3 pos, glm::vec3 lookAt, float fov)
    {
        delete camera;
        camera = new Camera(pos, lookAt, fov);
    }

    Scene::~Scene()
    {
        delete camera;
        delete gpuBVH;
		delete gpuScene;
		delete bvh;
    }
    void Scene::buildBVH()
    {
        Array<GPUScene::Triangle> tris;
        Array<Vec3f> verts;
        tris.clear();
        verts.clear();

        GPUScene::Triangle newtri;

        // convert Triangle to GPUScene::Triangle
        int triCount = int(triangleIndices.size());
        for (int i = 0; i < triCount; i++) {
            GPUScene::Triangle newtri;
            newtri.vertices = Vec3i(int(triangleIndices[i].indices.x), int(triangleIndices[i].indices.y), int(triangleIndices[i].indices.z));
            tris.add(newtri);
        }

        // fill up Array of vertices
        int verCount = int(vertexData.size());
        for (int i = 0; i < verCount; i++) {
            verts.add(Vec3f(vertexData[i].vertex.x, vertexData[i].vertex.y, vertexData[i].vertex.z));
        }

        std::cout << "Building a new GPU Scene\n";
        gpuScene = new GPUScene(triCount, verCount, tris, verts);

        std::cout << "Building BVH with spatial splits\n";
        // create a default platform
        Platform defaultplatform;
        BVH::BuildParams defaultparams;
        BVH::Stats stats;
        bvh = new BVH(gpuScene, defaultplatform, defaultparams);

        std::cout << "Building GPU-BVH\n";
        gpuBVH = new GPUBVH(bvh);
        std::cout << "GPU-BVH successfully created\n";
    }
}