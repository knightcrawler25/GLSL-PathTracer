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
#include <tiny_obj_loader.h>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <stdio.h>

namespace GLSLPT
{
    static const float _PI = 3.14159265358979323846f;

    static const int kMaxLineLength = 2048;
    int(*Log)(const char* szFormat, ...) = printf;

    bool LoadSceneFromFile(const std::string &filename, Scene *scene, RenderOptions& renderOptions)
    {
        FILE* file;
        file = fopen(filename.c_str(), "r");

        if (!file)
        {
            Log("Couldn't open %s for reading\n", filename.c_str());
            return false;
        }

        Log("Loading Scene..\n");

        struct MaterialData
        {
            Material mat;
            int id;
        };

        std::map<std::string, MaterialData> materialMap;
        std::vector<std::string> albedoTex;
        std::vector<std::string> metallicRoughnessTex;
        std::vector<std::string> normalTex;

        int materialCount = 0;
        char line[kMaxLineLength];

        //Defaults
        Material defaultMat;
        scene->addMaterial(defaultMat);

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
                Material material;
                int albedoTexID, matRghTexID, nrmTexID;
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
                    sscanf(line, " materialType %f", &material.materialType);
                    sscanf(line, " metallic %f", &material.metallic);
                    sscanf(line, " roughness %f", &material.roughness);
                    sscanf(line, " ior %f", &material.ior);
                    //sscanf(line, " transmittance %f", &material.transmittance);

                    sscanf(line, " albedoTexture %s", &albedoTexName);
                    sscanf(line, " metallicRoughnessTexture %s", &metallicRoughnessTexName);
                    sscanf(line, " normalTexture %s", normalTexName);
                }

                // Albedo Texture
                if (strcmp(albedoTexName, "None") != 0)
                    material.albedoTexID = scene->addTexture(albedoTexName);
             
                // MetallicRoughness Texture
                if (strcmp(metallicRoughnessTexName, "None") != 0)
                    material.metallicRoughnessTexID = scene->addTexture(metallicRoughnessTexName);
    
                // Normal Map Texture
                if (strcmp(normalTexName, "None") != 0)
                    material.normalmapTexID = scene->addTexture(normalTexName);

                // add material to map
                if (materialMap.find(name) == materialMap.end()) // New material
                {
                    int id = scene->addMaterial(material);
                    materialMap[name] = MaterialData{ material, id };
                }
            }

            //--------------------------------------------
            // Light

            if (strstr(line, "light"))
            {
                Light light;
                glm::vec3 v1, v2;
                char light_type[20] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " position %f %f %f", &light.position.x, &light.position.y, &light.position.z);
                    sscanf(line, " emission %f %f %f", &light.emission.x, &light.emission.y, &light.emission.z);

                    sscanf(line, " radius %f", &light.radius);
                    sscanf(line, " v1 %f %f %f", &v1.x, &v1.y, &v1.z);
                    sscanf(line, " v2 %f %f %f", &v2.x, &v2.y, &v2.z);
                    sscanf(line, " type %s", light_type);
                }

               if (strcmp(light_type, "Quad") == 0)
                {
                    light.type = LightType::QuadLight;
                    light.u = v1 - light.position;
                    light.v = v2 - light.position;
                    light.area = glm::length(glm::cross(light.u, light.v));
                }
                else if (strcmp(light_type, "Sphere") == 0)
                {
                    light.type = LightType::SphereLight;
                    light.area = 4.0f * _PI * light.radius * light.radius;
                }

                scene->addLight(light);
            }

            //--------------------------------------------
            // Camera

            if (strstr(line, "Camera"))
            {
                glm::vec3 position;
                glm::vec3 lookAt;
                float fov;
                float aperture = 0, focalDist = 1;

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " position %f %f %f", &position.x, &position.y, &position.z);
                    sscanf(line, " lookAt %f %f %f", &lookAt.x, &lookAt.y, &lookAt.z);
                    sscanf(line, " aperture %f ", &aperture);
                    sscanf(line, " focaldist %f", &focalDist);
                    sscanf(line, " fov %f", &fov);
                }

                delete scene->camera;
                scene->addCamera(position, lookAt, fov);
                scene->camera->aperture = aperture;
                scene->camera->focalDist = focalDist;
                cameraAdded = true;
            }

            //--------------------------------------------
            // Renderer

            if (strstr(line, "Renderer"))
            {
                char envMap[200] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " envMap %s", &envMap);
                    sscanf(line, " resolution %d %d", &renderOptions.resolution.x, &renderOptions.resolution.y);
                    sscanf(line, " hdrMultiplier %f", &renderOptions.hdrMultiplier);
                    sscanf(line, " maxDepth %i", &renderOptions.maxDepth);
                    sscanf(line, " numTilesX %i", &renderOptions.numTilesX);
                    sscanf(line, " numTilesY %i", &renderOptions.numTilesY);
                }

                if (strcmp(envMap, "None") != 0)
                {
                    scene->addHDR(envMap);
                    renderOptions.useEnvMap = true;
                }
            }


            //--------------------------------------------
            // Mesh

            if (strstr(line, "mesh"))
            {
                std::string filename;
                int material_id = 0; // Default Material ID
                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    char file[2048];
                    char matName[100];

                    if (sscanf(line, " file %s", file) == 1)
                    {
                        //meshPath = std::string("./assets/") + path;
                        filename = file;
                    }

                    if (sscanf(line, " material %s", matName) == 1)
                    {
                        // look up material in dictionary
                        if (materialMap.find(matName) != materialMap.end())
                        {
                            material_id = float(materialMap[matName].id);
                        }
                        else
                        {
                            Log("Could not find material %s\n", matName);
                        }
                    }
                }
                if (!filename.empty())
                {
                    int mesh_id = scene->addMesh(filename);
                    if (mesh_id != -1)
                    {
                        glm::mat4 xform;
                        MeshInstance instance1(mesh_id, xform, material_id);
                        scene->addMeshInstance(instance1);
                    }
                }
            }
        }

        fclose(file);

        // Add default camera if none was specified
        if (!cameraAdded)
            scene->addCamera(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, -10.0f), 35.0f);

        scene->createAccelerationStructures();
    }
}