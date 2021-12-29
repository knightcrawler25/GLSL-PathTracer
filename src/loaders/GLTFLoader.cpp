/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
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
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GLTFLoader.h"
#include "tiny_gltf.h"

namespace GLSLPT
{
    void LoadMeshes(Scene* scene, tinygltf::Model& gltfModel)
    {
        for (int meshIdx = 0; meshIdx < gltfModel.meshes.size(); meshIdx++)
        {
            tinygltf::Mesh gltfMesh = gltfModel.meshes[meshIdx];

            for (int primIdx = 0; primIdx < gltfMesh.primitives.size(); primIdx++)
            {
                tinygltf::Primitive prim = gltfMesh.primitives[primIdx];

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
                const UINT8* positionBufferAddress = positionBuffer.data.data();
                int positionStride = tinygltf::GetComponentSizeInBytes(positionAccessor.componentType) * tinygltf::GetNumComponentsInType(positionAccessor.type);
                // TODO: Recheck
                if (positionBufferView.byteStride > 0)
                    positionStride = positionBufferView.byteStride;

                // Vertex indices
                tinygltf::Accessor indexAccessor = gltfModel.accessors[indicesIndex];
                tinygltf::BufferView indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer& indexBuffer = gltfModel.buffers[indexBufferView.buffer];
                const UINT8* indexBufferAddress = indexBuffer.data.data();
                int indexStride = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType) * tinygltf::GetNumComponentsInType(indexAccessor.type);

                // Normals
                tinygltf::Accessor normalAccessor;
                tinygltf::BufferView normalBufferView;
                const UINT8* normalBufferAddress = nullptr;
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
                const UINT8* uv0BufferAddress = nullptr;
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
                        const UINT8* address = positionBufferAddress + positionBufferView.byteOffset + positionAccessor.byteOffset + (vertexIndex * positionStride);
                        memcpy(&vertex, address, 12);
                    }

                    if (normalIndex > -1)
                    {
                        const UINT8* address = normalBufferAddress + normalBufferView.byteOffset + normalAccessor.byteOffset + (vertexIndex * normalStride);
                        memcpy(&normal, address, 12);
                    }

                    if (uv0Index > -1)
                    {
                        const UINT8* address = uv0BufferAddress + uv0BufferView.byteOffset + uv0Accessor.byteOffset + (vertexIndex * uv0Stride);
                        memcpy(&uv, address, 8);
                    }

                    vertices.push_back(vertex);
                    normals.push_back(normal);
                    uvs.push_back(uv);
                }

                // Get index data
                std::vector<int> indices(indexAccessor.count);
                const UINT8* baseAddress = indexBufferAddress + indexBufferView.byteOffset + indexAccessor.byteOffset;
                if (indexStride == 1)
                {
                    std::vector<UINT8> quarter;
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
                    std::vector<UINT16> half;
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
                scene->meshes.push_back(mesh);
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
            scene->AddTexture(texName, image.image.data(), image.width, image.height, image.component);
        }
    }

    void LoadMaterials(Scene* scene, tinygltf::Model& gltfModel)
    {
        for (size_t i = 0; i < gltfModel.materials.size(); i++)
        {
            const tinygltf::Material gltfMaterial = gltfModel.materials[i];
            const tinygltf::PbrMetallicRoughness pbr = gltfMaterial.pbrMetallicRoughness;

            // Convert glTF material
            Material material;

            // Albedo
            material.baseColor = Vec3((float)pbr.baseColorFactor[0], (float)pbr.baseColorFactor[1], (float)pbr.baseColorFactor[2]);
            material.baseColorTexId = pbr.baseColorTexture.index;

            // Roughness and Metallic
            material.roughness = (float)pbr.roughnessFactor;
            material.metallic = (float)pbr.metallicFactor;
            material.metallicRoughnessTexID = pbr.metallicRoughnessTexture.index;

            // Normal Map
            material.normalmapTexID = gltfMaterial.normalTexture.index;

            // Emission
            material.emission = Vec3((float)gltfMaterial.emissiveFactor[0], (float)gltfMaterial.emissiveFactor[1], (float)gltfMaterial.emissiveFactor[2]);
            material.emissionmapTexID = gltfMaterial.emissiveTexture.index;

            scene->AddMaterial(material);
        }

        // Default material
        if (scene->materials.size() == 0)
        {
            Material default;
            scene->materials.push_back(default);
        }
    }

    void LoadInstances(Scene* scene, tinygltf::Model& gltfModel)
    {
        // FIXME: Traverse nodes to get instances with correct transforms
        for (size_t i = 0; i < scene->meshes.size(); i++)
        {
            Mat4 xform;
            MeshInstance instance = MeshInstance("", i, xform, i);
            scene->AddMeshInstance(instance);
        }
    }

    bool LoadGLTF(const std::string &filename, Scene *scene, RenderOptions& renderOptions, bool binary)
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

        LoadMeshes(scene, gltfModel);
        LoadTextures(scene, gltfModel);
        LoadMaterials(scene, gltfModel);
        LoadInstances(scene, gltfModel);

        return true;
    }
}