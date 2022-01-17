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

    bool LoadSceneFromFile(const std::string& filename, Scene* scene, RenderOptions& renderOptions)
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
                char albedoTexName[100] = "none";
                char metallicRoughnessTexName[100] = "none";
                char normalTexName[100] = "none";
                char emissionTexName[100] = "none";
                char alphaMode[20] = "none";
                char mediumType[20] = "none";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " color %f %f %f", &material.baseColor.x, &material.baseColor.y, &material.baseColor.z);
                    sscanf(line, " opacity %f", &material.opacity);
                    sscanf(line, " alphamode %s", alphaMode);
                    sscanf(line, " alphacutoff %f", &material.alphaCutoff);
                    sscanf(line, " emission %f %f %f", &material.emission.x, &material.emission.y, &material.emission.z);
                    sscanf(line, " metallic %f", &material.metallic);
                    sscanf(line, " roughness %f", &material.roughness);
                    sscanf(line, " subsurface %f", &material.subsurface);
                    sscanf(line, " speculartint %f", &material.specularTint);
                    sscanf(line, " anisotropic %f", &material.anisotropic);
                    sscanf(line, " sheen %f", &material.sheen);
                    sscanf(line, " sheentint %f", &material.sheenTint);
                    sscanf(line, " clearcoat %f", &material.clearcoat);
                    sscanf(line, " clearcoatgloss %f", &material.clearcoatGloss);
                    sscanf(line, " spectrans %f", &material.specTrans);
                    sscanf(line, " ior %f", &material.ior);
                    sscanf(line, " albedotexture %s", albedoTexName);
                    sscanf(line, " metallicroughnesstexture %s", metallicRoughnessTexName);
                    sscanf(line, " normaltexture %s", normalTexName);
                    sscanf(line, " emissiontexture %s", emissionTexName);
                    sscanf(line, " mediumtype %s", mediumType);
                    sscanf(line, " mediumdensity %f", &material.mediumDensity);
                    sscanf(line, " mediumcolor %f %f %f", &material.mediumColor.x, &material.mediumColor.y, &material.mediumColor.z);
                    sscanf(line, " mediumanisotropy %f", &material.mediumAnisotropy);
                }

                // Albedo Texture
                if (strcmp(albedoTexName, "none") != 0)
                    material.baseColorTexId = scene->AddTexture(path + albedoTexName);

                // MetallicRoughness Texture
                if (strcmp(metallicRoughnessTexName, "none") != 0)
                    material.metallicRoughnessTexID = scene->AddTexture(path + metallicRoughnessTexName);

                // Normal Map Texture
                if (strcmp(normalTexName, "none") != 0)
                    material.normalmapTexID = scene->AddTexture(path + normalTexName);

                // Emission Map Texture
                if (strcmp(emissionTexName, "none") != 0)
                    material.emissionmapTexID = scene->AddTexture(path + emissionTexName);

                // AlphaMode
                if (strcmp(alphaMode, "opaque") == 0)
                    material.alphaMode = AlphaMode::Opaque;
                else if (strcmp(alphaMode, "blend") == 0)
                    material.alphaMode = AlphaMode::Blend;
                else if (strcmp(alphaMode, "mask") == 0)
                    material.alphaMode = AlphaMode::Mask;

                // MediumType
                if (strcmp(mediumType, "absorb") == 0)
                    material.mediumType = MediumType::Absorb;
                else if (strcmp(mediumType, "scatter") == 0)
                    material.mediumType = MediumType::Scatter;
                else if (strcmp(mediumType, "emissive") == 0)
                    material.mediumType = MediumType::Emissive;

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
                char lightType[20] = "none";

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

                if (strcmp(lightType, "quad") == 0)
                {
                    light.type = LightType::RectLight;
                    light.u = v1 - light.position;
                    light.v = v2 - light.position;
                    light.area = Vec3::Length(Vec3::Cross(light.u, light.v));
                }
                else if (strcmp(lightType, "sphere") == 0)
                {
                    light.type = LightType::SphereLight;
                    light.area = 4.0f * PI * light.radius * light.radius;
                }
                else if (strcmp(lightType, "distant") == 0)
                {
                    light.type = LightType::DistantLight;
                    light.area = 0.0f;
                }

                scene->AddLight(light);
            }

            //--------------------------------------------
            // Camera

            if (strstr(line, "camera"))
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
                    sscanf(line, " lookat %f %f %f", &lookAt.x, &lookAt.y, &lookAt.z);
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

            if (strstr(line, "renderer"))
            {
                char envMap[200] = "none";
                char enableRR[10] = "none";
                char enableAces[10] = "none";
                char openglNormalMap[10] = "none";
                char hideEmitters[10] = "none";
                char transparentBackground[10] = "none";
                char enableBackground[10] = "none";
                char independentRenderSize[10] = "none";
                char enableTonemap[10] = "none";
                char enableRoughnessMollification[10] = "none";
                char enableVolumeMIS[10] = "none";
                char enableUniformLight[10] = "none";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " envmapfile %s", envMap);
                    sscanf(line, " resolution %d %d", &renderOptions.renderResolution.x, &renderOptions.renderResolution.y);
                    sscanf(line, " windowresolution %d %d", &renderOptions.windowResolution.x, &renderOptions.windowResolution.y);
                    sscanf(line, " envmapintensity %f", &renderOptions.envMapIntensity);
                    sscanf(line, " maxdepth %i", &renderOptions.maxDepth);
                    sscanf(line, " maxspp %i", &renderOptions.maxSpp);
                    sscanf(line, " tilewidth %i", &renderOptions.tileWidth);
                    sscanf(line, " tileheight %i", &renderOptions.tileHeight);
                    sscanf(line, " enablerr %s", enableRR);
                    sscanf(line, " rrdepth %i", &renderOptions.RRDepth);
                    sscanf(line, " enabletonemap %s", enableTonemap);
                    sscanf(line, " enableaces %s", enableAces);
                    sscanf(line, " texarraywidth %i", &renderOptions.texArrayWidth);
                    sscanf(line, " texarrayheight %i", &renderOptions.texArrayHeight);
                    sscanf(line, " openglnormalmap %s", openglNormalMap);
                    sscanf(line, " hideemitters %s", hideEmitters);
                    sscanf(line, " enablebackground %s", enableBackground);
                    sscanf(line, " transparentbackground %s", transparentBackground);
                    sscanf(line, " backgroundcolor %f %f %f", &renderOptions.backgroundCol.x, &renderOptions.backgroundCol.y, &renderOptions.backgroundCol.z);
                    sscanf(line, " independentrendersize %s", independentRenderSize);
                    sscanf(line, " envmaprotation %f", &renderOptions.envMapRot);
                    sscanf(line, " enableroughnessmollification %s", enableRoughnessMollification);
                    sscanf(line, " roughnessmollificationamt %f", &renderOptions.roughnessMollificationAmt);
                    sscanf(line, " enablevolumemis %s", enableVolumeMIS);
                    sscanf(line, " enableuniformlight %s", enableUniformLight);
                    sscanf(line, " uniformlightcolor %f %f %f", &renderOptions.uniformLightCol.x, &renderOptions.uniformLightCol.y, &renderOptions.uniformLightCol.z);
                }

                if (strcmp(envMap, "none") != 0)
                {
                    scene->AddEnvMap(path + envMap);
                    renderOptions.enableEnvMap = true;
                }
                else
                    renderOptions.enableEnvMap = false;

                if (strcmp(enableAces, "false") == 0)
                    renderOptions.enableAces = false;
                else if (strcmp(enableAces, "true") == 0)
                    renderOptions.enableAces = true;

                if (strcmp(enableRR, "false") == 0)
                    renderOptions.enableRR = false;
                else if (strcmp(enableRR, "true") == 0)
                    renderOptions.enableRR = true;

                if (strcmp(openglNormalMap, "false") == 0)
                    renderOptions.openglNormalMap = false;
                else if (strcmp(openglNormalMap, "true") == 0)
                    renderOptions.openglNormalMap = true;

                if (strcmp(hideEmitters, "false") == 0)
                    renderOptions.hideEmitters = false;
                else if (strcmp(hideEmitters, "true") == 0)
                    renderOptions.hideEmitters = true;

                if (strcmp(enableBackground, "false") == 0)
                    renderOptions.enableBackground = false;
                else if (strcmp(enableBackground, "true") == 0)
                    renderOptions.enableBackground = true;

                if (strcmp(transparentBackground, "false") == 0)
                    renderOptions.transparentBackground = false;
                else if (strcmp(transparentBackground, "true") == 0)
                    renderOptions.transparentBackground = true;

                if (strcmp(independentRenderSize, "false") == 0)
                    renderOptions.independentRenderSize = false;
                else if (strcmp(independentRenderSize, "true") == 0)
                    renderOptions.independentRenderSize = true;

                if (strcmp(enableTonemap, "false") == 0)
                    renderOptions.enableTonemap = false;
                else if (strcmp(enableTonemap, "true") == 0)
                    renderOptions.enableTonemap = true;

                if (strcmp(enableRoughnessMollification, "false") == 0)
                    renderOptions.enableRoughnessMollification = false;
                else if (strcmp(enableRoughnessMollification, "true") == 0)
                    renderOptions.enableRoughnessMollification = true;

                if (strcmp(enableVolumeMIS, "false") == 0)
                    renderOptions.enableVolumeMIS = false;
                else if (strcmp(enableVolumeMIS, "true") == 0)
                    renderOptions.enableVolumeMIS = true;

                if (strcmp(enableUniformLight, "false") == 0)
                    renderOptions.enableUniformLight = false;
                else if (strcmp(enableUniformLight, "true") == 0)
                    renderOptions.enableUniformLight = true;

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
                char meshName[200] = "none";
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
                        filename = path + file;

                    if (sscanf(line, " material %s", matName) == 1)
                    {
                        // look up material in dictionary
                        if (materialMap.find(matName) != materialMap.end())
                            material_id = materialMap[matName].id;
                        else
                            printf("Could not find material %s\n", matName);
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
                    int mesh_id = scene->AddMesh(filename);
                    if (mesh_id != -1)
                    {
                        std::string instanceName;

                        if (strcmp(meshName, "none") != 0)
                            instanceName = std::string(meshName);
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
                        filename = path + file;

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
                        printf("Unable to load gltf %s\n", filename.c_str());
                        exit(0);
                    }
                }
            }
        }

        fclose(file);

        return true;
    }
}
