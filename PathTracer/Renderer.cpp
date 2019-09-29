#include "Config.h"
#include "Renderer.h"
#include "Scene.h"

namespace GLSLPT
{
    Program *loadShaders(const std::string &vertex_shader_fileName, const std::string &frag_shader_fileName)
    {
        std::vector<Shader> shaders;
        shaders.push_back(Shader(vertex_shader_fileName, GL_VERTEX_SHADER));
        shaders.push_back(Shader(frag_shader_fileName, GL_FRAGMENT_SHADER));
        return new Program(shaders);
    }

    Renderer::Renderer(const Scene *scene, const std::string& shadersDirectory) : textureMapsArrayTex(0)
		, hdrTex(0)
		, hdrMarginalDistTex(0)
		, hdrConditionalDistTex(0)
        , initialized(false)
        , scene(scene)
        , screenSize(scene->renderOptions.resolution)
        , shadersDirectory(shadersDirectory)
    {
    }
    Renderer::~Renderer()
    {
        if (initialized)
            this->finish();
    }

    void Renderer::finish()
    {
        if (!initialized)
            return;

		glDeleteTextures(1, &BVHTex);
		glDeleteTextures(1, &BBoxminTex);
		glDeleteTextures(1, &BBoxmaxTex);
		glDeleteTextures(1, &vertexIndicesTex);
		glDeleteTextures(1, &verticesTex);
		glDeleteTextures(1, &normalsTex);
		glDeleteTextures(1, &materialsTex);
		glDeleteTextures(1, &transformsTex);
		glDeleteTextures(1, &lightsTex);
		glDeleteTextures(1, &textureMapsArrayTex);
		glDeleteTextures(1, &hdrTex);
		glDeleteTextures(1, &hdrMarginalDistTex);
		glDeleteTextures(1, &hdrConditionalDistTex);

        initialized = false;
		printf("Renderer finished!\n");
    }

    void Renderer::init()
    {
        if (initialized)
            return;

        if (scene == nullptr)
        {
			printf("Error: No Scene Found\n");
            return ;
        }

        quad = new Quad();

		//Create texture for BVH Tree
		glGenTextures(1, &BVHTex);
		glBindTexture(GL_TEXTURE_2D, BVHTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32I, scene->bvhTranslator.nodeTexWidth, scene->bvhTranslator.nodeTexWidth, 0, GL_RGB_INTEGER, GL_INT, &scene->bvhTranslator.nodes[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		//Create texture for Bounding boxes
		glGenTextures(1, &BBoxminTex);
		glBindTexture(GL_TEXTURE_2D, BBoxminTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, scene->bvhTranslator.nodeTexWidth, scene->bvhTranslator.nodeTexWidth, 0, GL_RGB, GL_FLOAT, &scene->bvhTranslator.bboxmin[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenTextures(1, &BBoxmaxTex);
		glBindTexture(GL_TEXTURE_2D, BBoxmaxTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, scene->bvhTranslator.nodeTexWidth, scene->bvhTranslator.nodeTexWidth, 0, GL_RGB, GL_FLOAT, &scene->bvhTranslator.bboxmax[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		//Create texture for VertexIndices
		glGenTextures(1, &vertexIndicesTex);
		glBindTexture(GL_TEXTURE_2D, vertexIndicesTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32I, scene->indicesTexWidth, scene->indicesTexWidth, 0, GL_RGB_INTEGER, GL_INT, &scene->vertIndices[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		//Create texture for Vertices
		glGenTextures(1, &verticesTex);
		glBindTexture(GL_TEXTURE_2D, verticesTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, scene->triDataTexWidth, scene->triDataTexWidth, 0, GL_RGBA, GL_FLOAT, &scene->vertices_uvx[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		//Create texture for Normals
		glGenTextures(1, &normalsTex);
		glBindTexture(GL_TEXTURE_2D, normalsTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, scene->triDataTexWidth, scene->triDataTexWidth, 0, GL_RGBA, GL_FLOAT, &scene->normals_uvy[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		//Create texture for Materials
		glGenTextures(1, &materialsTex);
		glBindTexture(GL_TEXTURE_2D, materialsTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (sizeof(Material) / sizeof(glm::vec4)) * scene->materials.size(), 1, 0, GL_RGBA, GL_FLOAT, &scene->materials[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		//Create texture for Transforms
		glGenTextures(1, &transformsTex);
		glBindTexture(GL_TEXTURE_2D, transformsTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (sizeof(glm::mat4) / sizeof(glm::vec4)) * scene->transforms.size(), 1, 0, GL_RGBA, GL_FLOAT, &scene->transforms[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		//Create Buffer and Texture for Lights
		numOfLights = int(scene->lights.size());

		if (numOfLights > 0)
		{
			//Create texture for lights
			glGenTextures(1, &lightsTex);
			glBindTexture(GL_TEXTURE_2D, lightsTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, (sizeof(Light) / sizeof(glm::vec3)) * scene->lights.size(), 1, 0, GL_RGB, GL_FLOAT, &scene->lights[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		if (scene->textures.size() > 0)
		{
			glGenTextures(1, &textureMapsArrayTex);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, textureMapsArrayTex);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, scene->texWidth, scene->texHeight, scene->textures.size(), 0, GL_RGB, GL_UNSIGNED_BYTE, &scene->textureMapsArray[0]);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		}

		// Environment Map
		if (scene->hdrData != nullptr)
		{
			glGenTextures(1, &hdrTex);
			glBindTexture(GL_TEXTURE_2D, hdrTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, scene->hdrData->width, scene->hdrData->height, 0, GL_RGB, GL_FLOAT, scene->hdrData->cols);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			glGenTextures(1, &hdrMarginalDistTex);
			glBindTexture(GL_TEXTURE_2D, hdrMarginalDistTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, scene->hdrData->height, 1, 0, GL_RG, GL_FLOAT, scene->hdrData->marginalDistData);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glBindTexture(GL_TEXTURE_2D, 0);

			glGenTextures(1, &hdrConditionalDistTex);
			glBindTexture(GL_TEXTURE_2D, hdrConditionalDistTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, scene->hdrData->width, scene->hdrData->height, 0, GL_RG, GL_FLOAT, scene->hdrData->conditionalDistData);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

        initialized = true;
    }
}