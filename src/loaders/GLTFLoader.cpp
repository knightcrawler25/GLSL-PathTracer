/*
 * MIT License
 *
 * Copyright(c) 2019 Asif Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

 // Much of this is from accompanying code for Ray Tracing Gems II, Chapter 14: The Reference Path Tracer
 // and was adapted for this project. See https://github.com/boksajak/referencePT for the original

#define TINYGLTF_IMPLEMENTATION

#include <map>
#include <cstdint>
#include "GLTFLoader.h"
#include "tiny_gltf.h"

namespace GLSLPT
{
    struct Primitive
    {
        int primitiveId;
        int materialId;
    };

    // Note: A GLTF mesh can contain multiple primitives and each primitive can potentially have a different material applied.
    // The two level BVH in this repo holds material ids per mesh and not per primitive, so this function loads each primitive from the gltf mesh as a new mesh
    void LoadMeshes(Scene* scene, tinygltf::Model& gltfModel, std::map<int, std::vector<Primitive>>& meshPrimMap)
    {
        for (int gltfMeshIdx = 0; gltfMeshIdx < gltfModel.meshes.size(); gltfMeshIdx++)
        {
            tinygltf::Mesh gltfMesh = gltfModel.meshes[gltfMeshIdx];

            for (int gltfPrimIdx = 0; gltfPrimIdx < gltfMesh.primitives.size(); gltfPrimIdx++)
            {
                tinygltf::Primitive prim = gltfMesh.primitives[gltfPrimIdx];

                // Skip points and lines
                if (prim.mode != TINYGLTF_MODE_TRIANGLES)
                    continue;

                int indicesIndex = prim.indices;
                int positionIndex = -1;
                int normalIndex = -1;
                int uv0Index = -1;

                if (prim.attributes.count("POSITION") > 0)
                {
                    positionIndex = prim.attributes["POSITION"];
                }

                if (prim.attributes.count("NORMAL") > 0)
                {
                    normalIndex = prim.attributes["NORMAL"];
                }

                if (prim.attributes.count("TEXCOORD_0") > 0)
                {
                    uv0Index = prim.attributes["TEXCOORD_0"];
                }

                // Vertex positions
                tinygltf::Accessor positionAccessor = gltfModel.accessors[positionIndex];
                tinygltf::BufferView positionBufferView = gltfModel.bufferViews[positionAccessor.bufferView];
                const tinygltf::Buffer& positionBuffer = gltfModel.buffers[positionBufferView.buffer];
                const uint8_t* positionBufferAddress = positionBuffer.data.data();
                int positionStride = tinygltf::GetComponentSizeInBytes(positionAccessor.componentType) * tinygltf::GetNumComponentsInType(positionAccessor.type);
                // TODO: Recheck
                if (positionBufferView.byteStride > 0)
                    positionStride = positionBufferView.byteStride;

                // FIXME: Some GLTF files like TriangleWithoutIndices.gltf have no indices
                // Vertex indices
                tinygltf::Accessor indexAccessor = gltfModel.accessors[indicesIndex];
                tinygltf::BufferView indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer& indexBuffer = gltfModel.buffers[indexBufferView.buffer];
                const uint8_t* indexBufferAddress = indexBuffer.data.data();
                int indexStride = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType) * tinygltf::GetNumComponentsInType(indexAccessor.type);

                // Normals
                tinygltf::Accessor normalAccessor;
                tinygltf::BufferView normalBufferView;
                const uint8_t* normalBufferAddress = nullptr;
                int normalStride = -1;
                if (normalIndex > -1)
                {
                    normalAccessor = gltfModel.accessors[normalIndex];
                    normalBufferView = gltfModel.bufferViews[normalAccessor.bufferView];
                    const tinygltf::Buffer& normalBuffer = gltfModel.buffers[normalBufferView.buffer];
                    normalBufferAddress = normalBuffer.data.data();
                    normalStride = tinygltf::GetComponentSizeInBytes(normalAccessor.componentType) * tinygltf::GetNumComponentsInType(normalAccessor.type);
                    if (normalBufferView.byteStride > 0)
                        normalStride = normalBufferView.byteStride;
                }

                // Texture coordinates
                tinygltf::Accessor uv0Accessor;
                tinygltf::BufferView uv0BufferView;
                const uint8_t* uv0BufferAddress = nullptr;
                int uv0Stride = -1;
                if (uv0Index > -1)
                {
                    uv0Accessor = gltfModel.accessors[uv0Index];
                    uv0BufferView = gltfModel.bufferViews[uv0Accessor.bufferView];
                    const tinygltf::Buffer& uv0Buffer = gltfModel.buffers[uv0BufferView.buffer];
                    uv0BufferAddress = uv0Buffer.data.data();
                    uv0Stride = tinygltf::GetComponentSizeInBytes(uv0Accessor.componentType) * tinygltf::GetNumComponentsInType(uv0Accessor.type);
                    if (uv0BufferView.byteStride > 0)
                        uv0Stride = uv0BufferView.byteStride;
                }

                std::vector<Vec3> vertices;
                std::vector<Vec3> normals;
                std::vector<Vec2> uvs;

                // Get vertex data
                for (size_t vertexIndex = 0; vertexIndex < positionAccessor.count; vertexIndex++)
                {
                    Vec3 vertex, normal;
                    Vec2 uv;

                    {
                        const uint8_t* address = positionBufferAddress + positionBufferView.byteOffset + positionAccessor.byteOffset + (vertexIndex * positionStride);
                        memcpy(&vertex, address, 12);
                    }

                    if (normalIndex > -1)
                    {
                        const uint8_t* address = normalBufferAddress + normalBufferView.byteOffset + normalAccessor.byteOffset + (vertexIndex * normalStride);
                        memcpy(&normal, address, 12);
                    }

                    if (uv0Index > -1)
                    {
                        const uint8_t* address = uv0BufferAddress + uv0BufferView.byteOffset + uv0Accessor.byteOffset + (vertexIndex * uv0Stride);
                        memcpy(&uv, address, 8);
                    }

                    vertices.push_back(vertex);
                    normals.push_back(normal);
                    uvs.push_back(uv);
                }

                // Get index data
                std::vector<int> indices(indexAccessor.count);
                const uint8_t* baseAddress = indexBufferAddress + indexBufferView.byteOffset + indexAccessor.byteOffset;
                if (indexStride == 1)
                {
                    std::vector<uint8_t> quarter;
                    quarter.resize(indexAccessor.count);

                    memcpy(quarter.data(), baseAddress, (indexAccessor.count * indexStride));

                    // Convert quarter precision indices to full precision
                    for (size_t i = 0; i < indexAccessor.count; i++)
                    {
                        indices[i] = quarter[i];
                    }
                }
                else if (indexStride == 2)
                {
                    std::vector<uint16_t> half;
                    half.resize(indexAccessor.count);

                    memcpy(half.data(), baseAddress, (indexAccessor.count * indexStride));

                    // Convert half precision indices to full precision
                    for (size_t i = 0; i < indexAccessor.count; i++)
                    {
                        indices[i] = half[i];
                    }
                }
                else
                {
                    memcpy(indices.data(), baseAddress, (indexAccessor.count * indexStride));
                }

                Mesh* mesh = new Mesh();

                // Get triangles from vertex indices
                for (int v = 0; v < indices.size(); v++)
                {
                    Vec3 pos = vertices[indices[v]];
                    Vec3 nrm = normals[indices[v]];
                    Vec2 uv = uvs[indices[v]];

                    mesh->verticesUVX.push_back(Vec4(pos.x, pos.y, pos.z, uv.x));
                    mesh->normalsUVY.push_back(Vec4(nrm.x, nrm.y, nrm.z, uv.y));
                }

                mesh->name = gltfMesh.name;
                int sceneMeshId = scene->meshes.size();
                scene->meshes.push_back(mesh);
                // Store a mapping for a gltf mesh and the loaded primitive data
                // This is used for creating instances based on the primitive
                int sceneMatIdx = prim.material + scene->materials.size();
                meshPrimMap[gltfMeshIdx].push_back(Primitive{ sceneMeshId, sceneMatIdx });
            }
        }
    }

    void LoadTextures(Scene* scene, tinygltf::Model& gltfModel)
    {
        for (size_t i = 0; i < gltfModel.textures.size(); ++i)
        {
            tinygltf::Texture& gltfTex = gltfModel.textures[i];
            tinygltf::Image& image = gltfModel.images[gltfTex.source];
            std::string texName = gltfTex.name;
            if (strcmp(gltfTex.name.c_str(), "") == 0)
                texName = image.uri;
            Texture* texture = new Texture(texName, image.image.data(), image.width, image.height, image.component);
            scene->textures.push_back(texture);
        }
    }

    void LoadMaterials(Scene* scene, tinygltf::Model& gltfModel)
    {
        int sceneTexIdx = scene->textures.size();
        // TODO: Support for KHR extensions
        for (size_t i = 0; i < gltfModel.materials.size(); i++)
        {
            const tinygltf::Material gltfMaterial = gltfModel.materials[i];
            const tinygltf::PbrMetallicRoughness pbr = gltfMaterial.pbrMetallicRoughness;

            // Convert glTF material
            Material material;

            // Albedo
            material.baseColor = Vec3((float)pbr.baseColorFactor[0], (float)pbr.baseColorFactor[1], (float)pbr.baseColorFactor[2]);
            if (pbr.baseColorTexture.index > -1)
                material.baseColorTexId = pbr.baseColorTexture.index + sceneTexIdx;

            // Opacity
            material.opacity = (float)pbr.baseColorFactor[3];

            // Alpha
            material.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
            if (strcmp(gltfMaterial.alphaMode.c_str(), "OPAQUE") == 0) material.alphaMode = AlphaMode::Opaque;
            else if (strcmp(gltfMaterial.alphaMode.c_str(), "BLEND") == 0) material.alphaMode = AlphaMode::Blend;
            else if (strcmp(gltfMaterial.alphaMode.c_str(), "MASK") == 0) material.alphaMode = AlphaMode::Mask;

            // Roughness and Metallic
            material.roughness = sqrtf((float)pbr.roughnessFactor); // Repo's disney material doesn't use squared roughness
            material.metallic = (float)pbr.metallicFactor;
            if (pbr.metallicRoughnessTexture.index > -1)
                material.metallicRoughnessTexID = pbr.metallicRoughnessTexture.index + sceneTexIdx;

            // Normal Map
            material.normalmapTexID = gltfMaterial.normalTexture.index + sceneTexIdx;

            // Emission
            material.emission = Vec3((float)gltfMaterial.emissiveFactor[0], (float)gltfMaterial.emissiveFactor[1], (float)gltfMaterial.emissiveFactor[2]);
            if (gltfMaterial.emissiveTexture.index > -1)
                material.emissionmapTexID = gltfMaterial.emissiveTexture.index + sceneTexIdx;

            // KHR_materials_transmission
            if (gltfMaterial.extensions.find("KHR_materials_transmission") != gltfMaterial.extensions.end())
            {
                const auto& ext = gltfMaterial.extensions.find("KHR_materials_transmission")->second;
                if (ext.Has("transmissionFactor"))
                    material.specTrans = (float)(ext.Get("transmissionFactor").Get<double>());
            }

            scene->AddMaterial(material);
        }

        // Default material
        if (scene->materials.size() == 0)
        {
            Material defaultMat;
            scene->materials.push_back(defaultMat);
        }
    }

    void TraverseNodes(Scene* scene, tinygltf::Model& gltfModel, int nodeIdx, Mat4& parentMat, std::map<int, std::vector<Primitive>>& meshPrimMap)
    {
        tinygltf::Node gltfNode = gltfModel.nodes[nodeIdx];

        Mat4 localMat;

        if (gltfNode.matrix.size() > 0)
        {
            localMat.data[0][0] = gltfNode.matrix[0];
            localMat.data[0][1] = gltfNode.matrix[1];
            localMat.data[0][2] = gltfNode.matrix[2];
            localMat.data[0][3] = gltfNode.matrix[3];

            localMat.data[1][0] = gltfNode.matrix[4];
            localMat.data[1][1] = gltfNode.matrix[5];
            localMat.data[1][2] = gltfNode.matrix[6];
            localMat.data[1][3] = gltfNode.matrix[7];

            localMat.data[2][0] = gltfNode.matrix[8];
            localMat.data[2][1] = gltfNode.matrix[9];
            localMat.data[2][2] = gltfNode.matrix[10];
            localMat.data[2][3] = gltfNode.matrix[11];

            localMat.data[3][0] = gltfNode.matrix[12];
            localMat.data[3][1] = gltfNode.matrix[13];
            localMat.data[3][2] = gltfNode.matrix[14];
            localMat.data[3][3] = gltfNode.matrix[15];
        }
        else
        {
            Mat4 translate, rot, scale;

            if (gltfNode.translation.size() > 0)
            {
                translate.data[3][0] = gltfNode.translation[0];
                translate.data[3][1] = gltfNode.translation[1];
                translate.data[3][2] = gltfNode.translation[2];
            }

            if (gltfNode.rotation.size() > 0)
            {
                rot = Mat4::QuatToMatrix(gltfNode.rotation[0], gltfNode.rotation[1], gltfNode.rotation[2], gltfNode.rotation[3]);
            }

            if (gltfNode.scale.size() > 0)
            {
                scale.data[0][0] = gltfNode.scale[0];
                scale.data[1][1] = gltfNode.scale[1];
                scale.data[2][2] = gltfNode.scale[2];
            }

            localMat = scale * rot * translate;
        }

        Mat4 xform = localMat * parentMat;

        // When at a leaf node, add an instance to the scene (if a mesh exists for it)
        if (gltfNode.children.size() == 0 && gltfNode.mesh != -1)
        {
            std::vector<Primitive> prims = meshPrimMap[gltfNode.mesh];

            // Write the instance data
            for (int i = 0; i < prims.size(); i++)
            {
                std::string name = gltfNode.name;
                // TODO: Better naming
                if (strcmp(name.c_str(), "") == 0)
                    name = "Mesh " + std::to_string(gltfNode.mesh) + " Prim" + std::to_string(prims[i].primitiveId);

                MeshInstance instance(name, prims[i].primitiveId, xform, prims[i].materialId < 0 ? 0 : prims[i].materialId);
                scene->AddMeshInstance(instance);
            }
        }

        for (size_t i = 0; i < gltfNode.children.size(); i++)
        {
            TraverseNodes(scene, gltfModel, gltfNode.children[i], xform, meshPrimMap);
        }
    }

    void LoadInstances(Scene* scene, tinygltf::Model& gltfModel, Mat4 xform, std::map<int, std::vector<Primitive>>& meshPrimMap)
    {
        const tinygltf::Scene gltfScene = gltfModel.scenes[gltfModel.defaultScene];

        for (int rootIdx = 0; rootIdx < gltfScene.nodes.size(); rootIdx++)
        {
            TraverseNodes(scene, gltfModel, gltfScene.nodes[rootIdx], xform, meshPrimMap);
        }
    }

    bool LoadGLTF(const std::string& filename, Scene* scene, RenderOptions& renderOptions, Mat4 xform, bool binary)
    {
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        printf("Loading GLTF %s\n", filename.c_str());

        bool ret;

        if (binary)
            ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filename);
        else
            ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);

        if (!ret)
        {
            printf("Unable to load file %s. Error: %s\n", filename.c_str(), err.c_str());
            return false;
        }

        std::map<int, std::vector<Primitive>> meshPrimMap;
        LoadMeshes(scene, gltfModel, meshPrimMap);
        LoadMaterials(scene, gltfModel);
        LoadTextures(scene, gltfModel);
        LoadInstances(scene, gltfModel, xform, meshPrimMap);

        return true;
    }
}
