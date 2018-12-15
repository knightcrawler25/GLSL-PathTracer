#include <Scene.h>
#include <iostream>

void Scene::addCamera(glm::vec3 pos, glm::vec3 lookAt, float fov)
{
	camera = new Camera(pos, lookAt, fov);
}

void Scene::buildBVH()
{
	Array<GPUScene::Triangle> tris;
	Array<Vec3f> verts;
	tris.clear();
	verts.clear();

	GPUScene::Triangle newtri;

	// convert Triangle to GPUScene::Triangle
	int triCount = triangleIndices.size();
	for (unsigned int i = 0; i < triCount; i++) {
		GPUScene::Triangle newtri;
		newtri.vertices = Vec3i(triangleIndices[i].indices.x, triangleIndices[i].indices.y, triangleIndices[i].indices.z);
		tris.add(newtri);
	}

	// fill up Array of vertices
	int verCount = vertexData.size();
	for (unsigned int i = 0; i < verCount; i++) {
		verts.add(Vec3f(vertexData[i].vertex.x, vertexData[i].vertex.y, vertexData[i].vertex.z));
	}

	std::cout << "Building a new GPU Scene\n";
	GPUScene* gpuScene = new GPUScene(triCount, verCount, tris, verts);

	std::cout << "Building BVH with spatial splits\n";
	// create a default platform
	Platform defaultplatform;
	BVH::BuildParams defaultparams;
	BVH::Stats stats;
	BVH *myBVH = new BVH(gpuScene, defaultplatform, defaultparams);

	std::cout << "Building GPU-BVH\n";
	gpuBVH = new GPUBVH(myBVH);
	std::cout << "GPU-BVH successfully created\n";
}
