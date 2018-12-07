#include <Renderer.h>

Program *loadShaders(std::string vertex_shader_fileName, std::string frag_shader_fileName)
{
	std::vector<Shader> shaders;
	shaders.push_back(Shader(vertex_shader_fileName, GL_VERTEX_SHADER));
	shaders.push_back(Shader(frag_shader_fileName, GL_FRAGMENT_SHADER));
	return new Program(shaders);
}


GPUBVH* Renderer::buildBVH()
{
	Array<GPUScene::Triangle> tris;
	Array<Vec3f> verts;
	tris.clear();
	verts.clear();

	GPUScene::Triangle newtri;

	// convert Triangle to GPUScene::Triangle
	int triCount = scene->triangleIndices.size();
	for (unsigned int i = 0; i < triCount; i++) {
		GPUScene::Triangle newtri;
		newtri.vertices = Vec3i(scene->triangleIndices[i].indices.x, scene->triangleIndices[i].indices.y, scene->triangleIndices[i].indices.z);
		tris.add(newtri);
	}

	// fill up Array of vertices
	int verCount = scene->vertexData.size();
	for (unsigned int i = 0; i < verCount; i++) {
		verts.add(Vec3f(scene->vertexData[i].vertex.x, scene->vertexData[i].vertex.y, scene->vertexData[i].vertex.z));
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
	gpuBVH = new GPUBVH(myBVH, scene);
	std::cout << "GPU-BVH successfully created\n";
	return gpuBVH;
}

void Renderer::init()
{
	quad = new Quad();

	if (scene == NULL)
	{
		std::cout << "Error: No Scene Found";
		exit(0);
	}
	gpuBVH = buildBVH();

	std::cout << "Triangles: " << scene->triangleIndices.size() << std::endl;
	std::cout << "Triangle Indices: " << gpuBVH->bvhTriangleIndices.size() << std::endl;
	std::cout << "Vertices: " << scene->vertexData.size() << std::endl;

	//Create Texture for BVH Tree
	glGenBuffers(1, &BVHBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, BVHBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(GPUBVHNode) * gpuBVH->bvh->getNumNodes(), &gpuBVH->gpuNodes[0], GL_STATIC_DRAW);
	glGenTextures(1, &BVHTexture);
	glBindTexture(GL_TEXTURE_BUFFER, BVHTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, BVHBuffer);

	//Create Buffer and Texture for TriangleIndices
	glGenBuffers(1, &triangleBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, triangleBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(TriangleData) * gpuBVH->bvhTriangleIndices.size(), &gpuBVH->bvhTriangleIndices[0], GL_STATIC_DRAW);
	glGenTextures(1, &triangleIndicesTexture);
	glBindTexture(GL_TEXTURE_BUFFER, triangleIndicesTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, triangleBuffer);

	//Create Buffer and Texture for Vertices
	glGenBuffers(1, &verticesBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, verticesBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(VertexData) * scene->vertexData.size(), &scene->vertexData[0], GL_STATIC_DRAW);
	glGenTextures(1, &verticesTexture);
	glBindTexture(GL_TEXTURE_BUFFER, verticesTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, verticesBuffer);

	//Create Buffer and Normals and TexCoords
	glGenBuffers(1, &normalTexCoordBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, normalTexCoordBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(NormalTexData) * scene->normalTexData.size(), &scene->normalTexData[0], GL_STATIC_DRAW);
	glGenTextures(1, &normalsTexCoordsTexture);
	glBindTexture(GL_TEXTURE_BUFFER, normalsTexCoordsTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, normalTexCoordBuffer);

	//Create Buffer and Texture for Materials
	glGenBuffers(1, &materialArrayBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, materialArrayBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(MaterialData) * scene->materialData.size(), &scene->materialData[0], GL_STATIC_DRAW);
	glGenTextures(1, &materialsTexture);
	glBindTexture(GL_TEXTURE_BUFFER, materialsTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, materialArrayBuffer);

	//Create Buffer and Texture for Lights
	glGenBuffers(1, &lightArrayBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, lightArrayBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(LightData) * scene->lightData.size(), &scene->lightData[0], GL_STATIC_DRAW);
	glGenTextures(1, &lightsTexture);
	glBindTexture(GL_TEXTURE_BUFFER, lightsTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, lightArrayBuffer);

	// Albedo Texture
	if (scene->texData.albedoTexCount > 0)
	{
		glGenTextures(1, &albedoTextures);
		glBindTexture(GL_TEXTURE_2D_ARRAY, albedoTextures);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGB, scene->texData.albedoTextureSize.x, scene->texData.albedoTextureSize.y, scene->texData.albedoTexCount);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, scene->texData.albedoTextureSize.x, scene->texData.albedoTextureSize.y, scene->texData.albedoTexCount, 0, GL_RGB, GL_UNSIGNED_BYTE, scene->texData.albedoTextures);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	//Metallic
	if (scene->texData.metallicTexCount > 0)
	{
		glGenTextures(1, &metallicTextures);
		glBindTexture(GL_TEXTURE_2D_ARRAY, metallicTextures);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGB, scene->texData.metallicTextureSize.x, scene->texData.metallicTextureSize.y, scene->texData.metallicTexCount);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, scene->texData.metallicTextureSize.x, scene->texData.metallicTextureSize.y, scene->texData.metallicTexCount, 0, GL_RGB, GL_UNSIGNED_BYTE, scene->texData.metallicTextures);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	//Roughness
	if (scene->texData.roughnessTexCount > 0)
	{
		glGenTextures(1, &roughnessTextures);
		glBindTexture(GL_TEXTURE_2D_ARRAY, roughnessTextures);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGB, scene->texData.roughnessTextureSize.x, scene->texData.roughnessTextureSize.y, scene->texData.roughnessTexCount);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, scene->texData.roughnessTextureSize.x, scene->texData.roughnessTextureSize.y, scene->texData.roughnessTexCount, 0, GL_RGB, GL_UNSIGNED_BYTE, scene->texData.roughnessTextures);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	//NormalMap
	if (scene->texData.normalTexCount > 0)
	{
		glGenTextures(1, &normalTextures);
		glBindTexture(GL_TEXTURE_2D_ARRAY, normalTextures);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGB, scene->texData.normalTextureSize.x, scene->texData.normalTextureSize.y, scene->texData.normalTexCount);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, scene->texData.normalTextureSize.x, scene->texData.normalTextureSize.y, scene->texData.normalTexCount, 0, GL_RGB, GL_UNSIGNED_BYTE, scene->texData.normalTextures);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	numOfLights = scene->lightData.size();
}