/*

Copyright (c) 2018 Miles Macklin

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgement in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

*/

/* 
	This is modified version of the original code 
	Link to original code: https://github.com/mmacklin/tinsel
*/

#include "Loader.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <SOIL.h>
#include "linear_math.h"
#include "Scene.h"
#include "Camera.h"

namespace GLSLPathTracer
{

    static const float M_PI = 3.14159265358979323846f;

    static const int kMaxLineLength = 2048;
    int(*Log)(const char* szFormat, ...) = printf;

    void LoadModel(Scene *scene, const std::string &filename, float materialId)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), 0, true);

        if (!ret)
        {
            printf_s("Unable to load model\n");
            exit(0);
        }

        // Load vertices
        int vertCount = int(attrib.vertices.size() / 3);
        size_t vertStartIndex = scene->vertexData.size();
        for (int i = 0; i < vertCount; i++)
            scene->vertexData.push_back(VertexData{ glm::vec3(attrib.vertices[3 * i + 0], attrib.vertices[3 * i + 1], attrib.vertices[3 * i + 2]) });

        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++)
        {
            // Loop over faces(polygon)
            int index_offset = 0;

            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            {
                glm::vec4 indices;
                glm::vec3 v[3], n[3], t[3];
                bool hasNormals = true;
                float vx, vy, vz, nx, ny, nz, tx, ty;

                int fv = shapes[s].mesh.num_face_vertices[f];
                for (int i = 0; i < fv; i++)
                {
                    tinyobj::index_t idx = tinyobj::index_t(shapes[s].mesh.indices[index_offset + i]);
                    indices[i] = float(vertStartIndex + (3 * idx.vertex_index + i) / 3);

                    vx = attrib.vertices[3 * idx.vertex_index + 0];
                    vy = attrib.vertices[3 * idx.vertex_index + 1];
                    vz = attrib.vertices[3 * idx.vertex_index + 2];

                    //Normals
                    if (idx.normal_index != -1)
                    {
                        nx = attrib.normals[3 * idx.normal_index + 0];
                        ny = attrib.normals[3 * idx.normal_index + 1];
                        nz = attrib.normals[3 * idx.normal_index + 2];
                    }
                    else
                    {
                        hasNormals = false;
                    }

                    //TexCoords
                    if (idx.texcoord_index != -1)
                    {
                        tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                        ty = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
                    }
                    else
                    {
                        //If model has no texture Coords then default to 0
                        tx = ty = 0;
                    }

                    n[i] = glm::vec3(nx, ny, nz);
                    t[i] = glm::vec3(tx, ty, materialId);
                }
                if (!hasNormals)
                {
                    //If model has no normals then Generate flat normals
                    glm::vec3 flatNormal = glm::normalize(glm::cross(v[1] - v[0], v[2] - v[0]));
                    n[0] = n[1] = n[2] = flatNormal;
                }

                scene->triangleIndices.push_back(TriangleData{ indices });
                scene->normalTexData.push_back(NormalTexData{ n[0], n[1], n[2], t[0], t[1], t[2] });

                index_offset += fv;
            }
        }
    }

    bool LoadScene(Scene *scene, const std::string &filename)
    {
        FILE* file;
        fopen_s(&file, filename.c_str(), "r");

        if (!file)
        {
            Log("Couldn't open %s for reading\n", filename.c_str());
            return false;
        }

        Log("Loading Scene..\n");

        struct Material
        {
            MaterialData materialData;
            int id;
        };

        std::map<std::string, Material> materialMap;
        std::vector<std::string> albedoTex;
        std::vector<std::string> metallicRoughnessTex;
        std::vector<std::string> normalTex;

        int materialCount = 0;
        char line[kMaxLineLength];

        //Defaults
        MaterialData defaultMat;
        scene->materialData.push_back(defaultMat);
        materialCount++;
        Camera *defaultCamera = new Camera(glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), 35.0f);
        bool cameraAdded = false;

        while (fgets(line, kMaxLineLength, file))
        {
            // skip comments
            if (line[0] == '#')
                continue;

            // name used for materials and meshes
            char name[kMaxLineLength] = { 0 };

            //--------------------------------------------
            // Material

            if (sscanf(line, " material %s", name) == 1)
            {
                MaterialData material;
                int noMetallic = 0;
                int noRoughness = 0;
                char albedoTexName[100] = "None";
                char metallicRoughnessTexName[100] = "None";
                char normalTexName[100] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " name %s", name);
                    sscanf(line, " color %f %f %f", &material.albedo.x, &material.albedo.y, &material.albedo.z);
                    sscanf(line, " emission %f %f %f", &material.emission.x, &material.emission.y, &material.emission.z);
                    sscanf(line, " materialType %f", &material.albedo.w);
                    sscanf(line, " metallic %f", &material.params.x);
                    sscanf(line, " roughness %f", &material.params.y);
                    sscanf(line, " ior %f", &material.params.z);
                    sscanf(line, " transmittance %f", &material.params.w);

                    sscanf(line, " albedoTexture %s", &albedoTexName);
                    sscanf(line, " metallicRoughnessTexture %s", &metallicRoughnessTexName);
                    sscanf(line, " noRoughness %i", &noRoughness);
                    sscanf(line, " normalTexture %s", normalTexName);
                }

                // Albedo Texture
                if (strcmp(albedoTexName, "None") != 0)
                {
                    ptrdiff_t pos = std::distance(albedoTex.begin(), find(albedoTex.begin(), albedoTex.end(), albedoTexName));
                    if (pos == albedoTex.size()) // New texture
                    {
                        albedoTex.push_back(albedoTexName);
                        material.texIDs.x = float(albedoTex.size() - 1);
                    }
                    else
                        material.texIDs.x = float(int(pos));
                }
                else
                    material.texIDs.x = -1;

                // MetallicRoughness Texture
                if (strcmp(metallicRoughnessTexName, "None") != 0)
                {
                    ptrdiff_t pos = std::distance(metallicRoughnessTex.begin(), find(metallicRoughnessTex.begin(), metallicRoughnessTex.end(), metallicRoughnessTexName));
                    if (pos == metallicRoughnessTex.size())
                    {
                        metallicRoughnessTex.push_back(metallicRoughnessTexName);
                        material.texIDs.y = float(metallicRoughnessTex.size() - 1);
                    }
                    else
                    {
                        material.texIDs.y = float(int(pos));
                    }
                }
                else
                {
                    material.texIDs.y = -1;
                }

                // Normal Map Texture
                if (strcmp(normalTexName, "None") != 0)
                {
                    ptrdiff_t pos = std::distance(normalTex.begin(), find(normalTex.begin(), normalTex.end(), normalTexName));
                    if (pos == normalTex.size())
                    {
                        normalTex.push_back(normalTexName);
                        material.texIDs.z = float(normalTex.size() - 1);
                    }
                    else
                        material.texIDs.z = float(int(pos));
                }
                else
                    material.texIDs.z = -1;

                // add material to map
                if (materialMap.find(name) == materialMap.end()) // New material
                {
                    materialMap[name] = Material{ material, materialCount++ };
                    scene->materialData.push_back(material);
                }
            }

            //--------------------------------------------
            // Light

            if (strstr(line, "light"))
            {
                LightData light;
                glm::vec3 v1, v2;
                char light_type[20] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " position %f %f %f", &light.position.x, &light.position.y, &light.position.z);
                    sscanf(line, " emission %f %f %f", &light.emission.x, &light.emission.y, &light.emission.z);

                    sscanf(line, " radius %f", &light.radiusAreaType.x);
                    sscanf(line, " v1 %f %f %f", &v1.x, &v1.y, &v1.z);
                    sscanf(line, " v2 %f %f %f", &v2.x, &v2.y, &v2.z);
                    sscanf(line, " type %s", light_type);
                }

                if (strcmp(light_type, "Quad") == 0)
                {
                    light.radiusAreaType.z = 0;
                    light.u = v1 - light.position;
                    light.v = v2 - light.position;
                    light.radiusAreaType.y = glm::length(glm::cross(light.u, light.v));
                }
                else if (strcmp(light_type, "Sphere") == 0)
                {
                    light.radiusAreaType.z = 1;
                    light.radiusAreaType.y = 4.0f * M_PI * light.radiusAreaType.x * light.radiusAreaType.x;
                }

                scene->lightData.push_back(light);
            }

            //--------------------------------------------
            // Camera

            if (strstr(line, "Camera"))
            {
                glm::vec3 position;
                glm::vec3 lookAt;
                float fov;

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " position %f %f %f", &position.x, &position.y, &position.z);
                    sscanf(line, " lookAt %f %f %f", &lookAt.x, &lookAt.y, &lookAt.z);
                    sscanf(line, " fov %f", &fov);
                }

                scene->camera = new Camera(position, lookAt, fov);
                cameraAdded = true;
            }

            //--------------------------------------------
            // Renderer

            if (strstr(line, "Renderer"))
            {
                char rendererType[20] = "None";
                char envMap[200] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " rendererType %s", &rendererType);
                    sscanf(line, " resolution %d %d", &scene->renderOptions.resolution.x, &scene->renderOptions.resolution.y);
                    sscanf(line, " envMap %s", &envMap);
                    sscanf(line, " hdrMultiplier %f", &scene->renderOptions.hdrMultiplier);
                    sscanf(line, " maxDepth %i", &scene->renderOptions.maxDepth);
                    sscanf(line, " maxSamples %i", &scene->renderOptions.maxSamples);
                    sscanf(line, " numTilesX %i", &scene->renderOptions.numTilesX);
                    sscanf(line, " numTilesY %i", &scene->renderOptions.numTilesY);

                    if (strcmp(envMap, "None") != 0)
                    {
                        HDRLoader hdrLoader;
                        hdrLoader.load(envMap, scene->hdrLoaderRes);
                        scene->renderOptions.useEnvMap = true;
                    }
                    scene->renderOptions.rendererType = std::string(rendererType);
                }
            }


            //--------------------------------------------
            // Mesh

            if (strstr(line, "mesh"))
            {
                std::string meshPath;
                float materialId = 0.0f; // Default Material ID
                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    char path[2048];

                    if (sscanf(line, " file %s", path) == 1)
                    {
                        //meshPath = std::string("./assets/") + path;
                        meshPath = path;
                    }

                    if (sscanf(line, " material %s", path) == 1)
                    {
                        // look up material in dictionary
                        if (materialMap.find(path) != materialMap.end())
                        {
                            materialId = float(materialMap[path].id);
                        }
                        else
                        {
                            printf_s("Could not find material %s\n", path);
                        }
                    }
                }
                if (!meshPath.empty())
                {
                    Log("Loading Model: %s\n", meshPath.c_str());
                    LoadModel(scene, meshPath, materialId);
                }
            }
        }

        if (!cameraAdded)
            scene->camera = defaultCamera;

        //Load all textures 
        //(Assume for now that all textures have same width and height as their group (albedo, metallic etc))
        int width, height;

        scene->texData.albedoTexCount = int(albedoTex.size());
        scene->texData.metallicRoughnessTexCount = int(metallicRoughnessTex.size());
        scene->texData.normalTexCount = int(normalTex.size());

        //Load albedo Textures
        for (size_t i = 0; i < albedoTex.size(); i++)
        {
            Log("Loading Texture: %s\n", albedoTex[i].c_str());
            unsigned char * texture = SOIL_load_image(albedoTex[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
            if (i == 0) // Alloc memory based on first texture size
                scene->texData.albedoTextures = new unsigned char[width * height * 3 * albedoTex.size()];
            memcpy(&(scene->texData.albedoTextures[width * height * 3 * i]), &texture[0], width * height * 3);
            delete texture;
        }
        scene->texData.albedoTextureSize = glm::vec2(width, height);

        //Load MetallicRoughness textures
        for (size_t i = 0; i < metallicRoughnessTex.size(); i++)
        {
            Log("Loading Texture: %s\n", metallicRoughnessTex[i].c_str());
            unsigned char * texture = SOIL_load_image(metallicRoughnessTex[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
            if (i == 0) // Alloc memory based on first texture size
                scene->texData.metallicRoughnessTextures = new unsigned char[width * height * 3 * metallicRoughnessTex.size()];
            memcpy(&(scene->texData.metallicRoughnessTextures[width * height * 3 * i]), &texture[0], width * height * 3);
            delete texture;
        }
        scene->texData.metallicRoughnessTextureSize = glm::vec2(width, height);

        //Load normal textures
        for (size_t i = 0; i < normalTex.size(); i++)
        {
            Log("Loading Texture: %s\n", normalTex[i].c_str());
            unsigned char * texture = SOIL_load_image(normalTex[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
            if (i == 0) // Alloc memory based on first texture size
                scene->texData.normalTextures = new unsigned char[width * height * 3 * normalTex.size()];
            memcpy(&(scene->texData.normalTextures[width * height * 3 * i]), &texture[0], width * height * 3);
            delete texture;
        }
        scene->texData.normalTextureSize = glm::vec2(width, height);

        return true;
    }
}