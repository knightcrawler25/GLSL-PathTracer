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
    This is a modified version of the original code 
    Link to original code: https://github.com/mmacklin/tinsel
*/

#include <cstring>
#include "Loader.h"
#include "GLTFLoader.h"

namespace GLSLPT
{
    static const int kMaxLineLength = 2048;

    bool LoadSceneFromFile(const std::string &filename, Scene *scene, RenderOptions& renderOptions)
    {
        FILE* file;
        file = fopen(filename.c_str(), "r");

        if (!file)
        {
            printf("Couldn't open %s for reading\n", filename.c_str());
            return false;
        }

        printf("Loading Scene..\n");

        struct MaterialData
        {
            Material mat;
            int id;
        };

        std::map<std::string, MaterialData> materialMap;
        std::vector<std::string> albedoTex;
        std::vector<std::string> metallicRoughnessTex;
        std::vector<std::string> normalTex;
        std::string path = filename.substr(0, filename.find_last_of("/\\")) + "/";

        int materialCount = 0;
        char line[kMaxLineLength];

        //Defaults
        Material defaultMat;
        scene->AddMaterial(defaultMat);

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
                char albedoTexName[100] = "None";
                char metallicRoughnessTexName[100] = "None";
                char normalTexName[100] = "None";
                char emissionTexName[100] = "None";
                char alphaMode[20] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " color %f %f %f", &material.baseColor.x, &material.baseColor.y, &material.baseColor.z);
                    sscanf(line, " opacity %f", &material.opacity);
                    sscanf(line, " alphaMode %s", alphaMode);
                    sscanf(line, " alphaCutoff %f", &material.alphaCutoff);
                    sscanf(line, " emission %f %f %f", &material.emission.x, &material.emission.y, &material.emission.z);
                    sscanf(line, " metallic %f", &material.metallic);
                    sscanf(line, " roughness %f", &material.roughness);
                    sscanf(line, " subsurface %f", &material.subsurface);
                    sscanf(line, " specularTint %f", &material.specularTint);
                    sscanf(line, " anisotropic %f", &material.anisotropic);
                    sscanf(line, " sheen %f", &material.sheen);
                    sscanf(line, " sheenTint %f", &material.sheenTint);
                    sscanf(line, " clearcoat %f", &material.clearcoat);
                    sscanf(line, " clearcoatGloss %f", &material.clearcoatGloss);
                    sscanf(line, " transmission %f", &material.specTrans);
                    sscanf(line, " ior %f", &material.ior);
                    sscanf(line, " extinction %f %f %f", &material.extinction.x, &material.extinction.y, &material.extinction.z);
                    sscanf(line, " atDistance %f", &material.atDistance);
                    sscanf(line, " albedoTexture %s", albedoTexName);
                    sscanf(line, " metallicRoughnessTexture %s", metallicRoughnessTexName);
                    sscanf(line, " normalTexture %s", normalTexName);
                    sscanf(line, " emissionTexture %s", emissionTexName);
                }

                // Albedo Texture
                if (strcmp(albedoTexName, "None") != 0)
                    material.baseColorTexId = scene->AddTexture(path + albedoTexName);
             
                // MetallicRoughness Texture
                if (strcmp(metallicRoughnessTexName, "None") != 0)
                    material.metallicRoughnessTexID = scene->AddTexture(path + metallicRoughnessTexName);
    
                // Normal Map Texture
                if (strcmp(normalTexName, "None") != 0)
                    material.normalmapTexID = scene->AddTexture(path + normalTexName);

                // Emission Map Texture
                if (strcmp(emissionTexName, "None") != 0)
                    material.emissionmapTexID = scene->AddTexture(path + emissionTexName);

                // AlphaMode
                if (strcmp(alphaMode, "Opaque") == 0)
                    material.alphaMode = AlphaMode::Opaque;
                else if (strcmp(alphaMode, "Blend") == 0)
                    material.alphaMode = AlphaMode::Blend;
                else if (strcmp(alphaMode, "Mask") == 0)
                    material.alphaMode = AlphaMode::Mask;

                // add material to map
                if (materialMap.find(name) == materialMap.end()) // New material
                {
                    int id = scene->AddMaterial(material);
                    materialMap[name] = MaterialData{ material, id };
                }
            }

            //--------------------------------------------
            // Light

            if (strstr(line, "light"))
            {
                Light light;
                Vec3 v1, v2;
                char lightType[20] = "None";

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
                    sscanf(line, " type %s", lightType);
                }

                if (strcmp(lightType, "Quad") == 0)
                {
                    light.type = LightType::RectLight;
                    light.u = v1 - light.position;
                    light.v = v2 - light.position;
                    light.area = Vec3::Length(Vec3::Cross(light.u, light.v));
                }
                else if (strcmp(lightType, "Sphere") == 0)
                {
                    light.type = LightType::SphereLight;
                    light.area = 4.0f * PI * light.radius * light.radius;
                }
                else if (strcmp(lightType, "Distant") == 0)
                {
                    light.type = LightType::DistantLight;
                    light.area = 0.0f;
                }

                scene->AddLight(light);
            }

            //--------------------------------------------
            // Camera

            if (strstr(line, "Camera"))
            {
                Mat4 xform;
                Vec3 position;
                Vec3 lookAt;
                float fov;
                float aperture = 0, focalDist = 1;
                bool matrixProvided = false;

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

                    if (sscanf(line, " matrix %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                        &xform[0][0], &xform[1][0], &xform[2][0], &xform[3][0],
                        &xform[0][1], &xform[1][1], &xform[2][1], &xform[3][1],
                        &xform[0][2], &xform[1][2], &xform[2][2], &xform[3][2],
                        &xform[0][3], &xform[1][3], &xform[2][3], &xform[3][3]
                    ) != 0)
                        matrixProvided = true;
                }

                delete scene->camera;

                if (matrixProvided)
                {
                    Vec3 forward = Vec3(xform[2][0], xform[2][1], xform[2][2]);
                    position = Vec3(xform[3][0], xform[3][1], xform[3][2]);
                    lookAt = position + forward;
                }

                scene->AddCamera(position, lookAt, fov);
                scene->camera->aperture = aperture;
                scene->camera->focalDist = focalDist;
            }

            //--------------------------------------------
            // Renderer

            if (strstr(line, "Renderer"))
            {
                char envMap[200] = "None";
                char enableRR[10] = "None";
                char useAces[10] = "None";
                char openglNormalMap[10] = "None";
                char hideEmitters[10] = "None";
                char transparentBackground[10] = "None";
                char enableBackground[10] = "None";
                char independentRenderSize[10] = "None";
                char enableTonemap[10] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " envMap %s", envMap);
                    sscanf(line, " resolution %d %d", &renderOptions.renderResolution.x, &renderOptions.renderResolution.y);
                    sscanf(line, " windowResolution %d %d", &renderOptions.windowResolution.x, &renderOptions.windowResolution.y);
                    sscanf(line, " hdrMultiplier %f", &renderOptions.envMapIntensity); // TODO: Rename option in all scene files
                    sscanf(line, " maxDepth %i", &renderOptions.maxDepth);
                    sscanf(line, " maxSpp %i", &renderOptions.maxSpp);
                    sscanf(line, " tileWidth %i", &renderOptions.tileWidth);
                    sscanf(line, " tileHeight %i", &renderOptions.tileHeight);
                    sscanf(line, " enableRR %s", enableRR);
                    sscanf(line, " RRDepth %i", &renderOptions.RRDepth);
                    sscanf(line, " enableTonemap %s", enableTonemap);
                    sscanf(line, " useAces %s", useAces);
                    sscanf(line, " texArrayWidth %i", &renderOptions.texArrayWidth);
                    sscanf(line, " texArrayHeight %i", &renderOptions.texArrayHeight);
                    sscanf(line, " openglNormalMap %s", openglNormalMap);
                    sscanf(line, " hideEmitters %s", hideEmitters);
                    sscanf(line, " enableBackground %s", enableBackground);
                    sscanf(line, " transparentBackground %s", transparentBackground);
                    sscanf(line, " backgroundColor %f %f %f", &renderOptions.backgroundCol.x, &renderOptions.backgroundCol.y, &renderOptions.backgroundCol.z);
                    sscanf(line, " independentRenderSize %s", independentRenderSize);
                    sscanf(line, " envMapRotation %f", &renderOptions.envMapRot);
                }

                if (strcmp(envMap, "None") != 0)
                {
                    scene->AddEnvMap(path + envMap);
                    renderOptions.useEnvMap = true;
                }
                else
                    renderOptions.useEnvMap = false;
                    
                if (strcmp(useAces, "False") == 0)
                    renderOptions.useAces = false;
                else if (strcmp(useAces, "True") == 0)
                    renderOptions.useAces = true;

                if (strcmp(enableRR, "False") == 0)
                    renderOptions.enableRR = false;
                else if (strcmp(enableRR, "True") == 0)
                    renderOptions.enableRR = true;

                if (strcmp(openglNormalMap, "False") == 0)
                    renderOptions.openglNormalMap = false;
                else if (strcmp(openglNormalMap, "True") == 0)
                    renderOptions.openglNormalMap = true;

                if (strcmp(hideEmitters, "False") == 0)
                    renderOptions.hideEmitters = false;
                else if (strcmp(hideEmitters, "True") == 0)
                    renderOptions.hideEmitters = true;

                if (strcmp(enableBackground, "False") == 0)
                    renderOptions.enableBackground = false;
                else if (strcmp(enableBackground, "True") == 0)
                    renderOptions.enableBackground = true;

                if (strcmp(transparentBackground, "False") == 0)
                    renderOptions.transparentBackground = false;
                else if (strcmp(transparentBackground, "True") == 0)
                    renderOptions.transparentBackground = true;

                if (strcmp(independentRenderSize, "False") == 0)
                    renderOptions.independentRenderSize = false;
                else if (strcmp(independentRenderSize, "True") == 0)
                    renderOptions.independentRenderSize = true;

                if (strcmp(enableTonemap, "False") == 0)
                    renderOptions.enableTonemap = false;
                else if (strcmp(enableTonemap, "True") == 0)
                    renderOptions.enableTonemap = true;

                if (!renderOptions.independentRenderSize)
                    renderOptions.windowResolution = renderOptions.renderResolution;
            }


            //--------------------------------------------
            // Mesh

            if (strstr(line, "mesh"))
            {
                std::string filename;
                Vec4 rotQuat;
                Mat4 xform, translate, rot, scale;
                int material_id = 0; // Default Material ID
                char meshName[200] = "None";
                bool matrixProvided = false;

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    char file[2048];
                    char matName[100];

                    sscanf(line, " name %[^\t\n]s", meshName);

                    if (sscanf(line, " file %s", file) == 1)
                    {
                        filename = path + file;
                    }

                    if (sscanf(line, " material %s", matName) == 1)
                    {
                        // look up material in dictionary
                        if (materialMap.find(matName) != materialMap.end())
                        {
                            material_id = materialMap[matName].id;
                        }
                        else
                        {
                            printf("Could not find material %s\n", matName);
                        }
                    }

                    if (sscanf(line, " matrix %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                        &xform[0][0], &xform[1][0], &xform[2][0], &xform[3][0],
                        &xform[0][1], &xform[1][1], &xform[2][1], &xform[3][1],
                        &xform[0][2], &xform[1][2], &xform[2][2], &xform[3][2],
                        &xform[0][3], &xform[1][3], &xform[2][3], &xform[3][3]
                    ) != 0)
                        matrixProvided = true;

                    sscanf(line, " position %f %f %f", &translate.data[3][0], &translate.data[3][1], &translate.data[3][2]);
                    sscanf(line, " scale %f %f %f", &scale.data[0][0], &scale.data[1][1], &scale.data[2][2]);
                    if(sscanf(line, " rotation %f %f %f %f", &rotQuat.x, &rotQuat.y, &rotQuat.z, &rotQuat.w) != 0)
                        rot = Mat4::QuatToMatrix(rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w);
                }

                if (!filename.empty())
                {
                    int mesh_id = scene->AddMesh(filename);
                    if (mesh_id != -1)
                    {
                        std::string instanceName;

                        if (strcmp(meshName, "None") != 0)
                        {
                            instanceName = std::string(meshName);
                        }
                        else
                        {
                            std::size_t pos = filename.find_last_of("/\\");
                            instanceName = filename.substr(pos + 1);
                        }
                        
                        Mat4 transformMat;

                        if (matrixProvided)
                            transformMat = xform;
                        else
                            transformMat = scale * rot * translate;

                        MeshInstance instance(instanceName, mesh_id, transformMat, material_id);
                        scene->AddMeshInstance(instance);
                    }
                }
            }

            //--------------------------------------------
            // GLTF

            if (strstr(line, "gltf"))
            {
                std::string filename;
                Vec4 rotQuat;
                Mat4 xform, translate, rot, scale;
                bool matrixProvided = false;

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    char file[2048];

                    if (sscanf(line, " file %s", file) == 1)
                    {
                        filename = path + file;
                    }

                    if (sscanf(line, " matrix %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                        &xform[0][0], &xform[1][0], &xform[2][0], &xform[3][0],
                        &xform[0][1], &xform[1][1], &xform[2][1], &xform[3][1],
                        &xform[0][2], &xform[1][2], &xform[2][2], &xform[3][2],
                        &xform[0][3], &xform[1][3], &xform[2][3], &xform[3][3]
                    ) != 0)
                        matrixProvided = true;

                    sscanf(line, " position %f %f %f", &translate.data[3][0], &translate.data[3][1], &translate.data[3][2]);
                    sscanf(line, " scale %f %f %f", &scale.data[0][0], &scale.data[1][1], &scale.data[2][2]);
                    if (sscanf(line, " rotation %f %f %f %f", &rotQuat.x, &rotQuat.y, &rotQuat.z, &rotQuat.w) != 0)
                        rot = Mat4::QuatToMatrix(rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w);
                }

                if (!filename.empty())
                {
                    std::string ext = filename.substr(filename.find_last_of(".") + 1);

                    bool success = false;
                    Mat4 transformMat;

                    if (matrixProvided)
                        transformMat = xform;
                    else
                        transformMat = scale * rot * translate;

                    // TODO: Add support for instancing.
                    // If the same gltf is loaded multiple times then mesh data gets duplicated
                    if (ext == "gltf")
                        success = LoadGLTF(filename, scene, renderOptions, transformMat, false);
                    else if (ext == "glb")
                        success = LoadGLTF(filename, scene, renderOptions, transformMat, true);

                    if (!success)
                    {
                        printf("Unable to load gltf %s\n", filename);
                        exit(0);
                    }
                }
            }
        }

        fclose(file);

        return true;
    }
}
