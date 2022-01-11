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

#pragma once

#include <string>
#include <vector>
#include <map>
#include "EnvironmentMap.h"
#include "bvh.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Camera.h"
#include "bvh_translator.h"
#include "Texture.h"
#include "Material.h"

namespace GLSLPT
{
    class Camera;

    enum LightType
    {
        RectLight,
        SphereLight,
        DistantLight
    };

    struct Light
    {
        Vec3 position;
        Vec3 emission;
        Vec3 u;
        Vec3 v;
        float radius;
        float area;
        float type;
    };

    struct Indices
    {
        int x, y, z;
    };

    class Scene
    {
    public:
        Scene() : camera(nullptr), envMap(nullptr), initialized(false), dirty(true) {
            sceneBvh = new RadeonRays::Bvh(10.0f, 64, false);
        }
        ~Scene();

        int AddMesh(const std::string& filename);
        int AddTexture(const std::string& filename);
        int AddMaterial(const Material& material);
        int AddMeshInstance(const MeshInstance& meshInstance);
        int AddLight(const Light& light);

        void AddCamera(Vec3 eye, Vec3 lookat, float fov);
        void AddEnvMap(const std::string& filename);

        void ProcessScene();
        void RebuildInstances();

        // Options
        RenderOptions renderOptions;

        // Meshes
        std::vector<Mesh*> meshes;

        // Scene Mesh Data 
        std::vector<Indices> vertIndices;
        std::vector<Vec4> verticesUVX; // Vertex + texture Coord (u/s)
        std::vector<Vec4> normalsUVY; // Normal + texture Coord (v/t)
        std::vector<Mat4> transforms;

        // Materials
        std::vector<Material> materials;

        // Instances
        std::vector<MeshInstance> meshInstances;

        // Lights
        std::vector<Light> lights;

        // Environment Map
        EnvironmentMap* envMap;

        // Camera
        Camera* camera;

        // Bvh
        RadeonRays::BvhTranslator bvhTranslator; // Produces a flat bvh array for GPU consumption
        RadeonRays::bbox sceneBounds;

        // Texture Data
        std::vector<Texture*> textures;
        std::vector<unsigned char> textureMapsArray;

        bool initialized;
        bool dirty;
        // To check if scene elements need to be resent to GPU
        bool instancesModified = false;
        bool envMapModified = false;

    private:
        RadeonRays::Bvh* sceneBvh;
        void createBLAS();
        void createTLAS();
    };
}
