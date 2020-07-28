/*
 * MIT License
 *
 * Copyright(c) 2019-2020 Asif Ali
 *
 * Authors/Contributors:
 *
 * Asif Ali
 * Cedric Guillemet
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this softwareand associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include "Scene.h"

namespace GLSLPT
{
    void loadCornellTestScene(Scene* scene, RenderOptions &renderOptions)
    {
        renderOptions.maxDepth = 2;
        renderOptions.tileHeight = 200;
        renderOptions.tileWidth = 200;
        renderOptions.hdrMultiplier = 1.0f;
        renderOptions.useEnvMap = true;
        scene->AddCamera(glm::vec3(0.276f, 0.275f, -0.75f), glm::vec3(0.276f, 0.275f, 0), 65.0f);

        int ceiling_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_ceiling.obj");
        int floor_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_floor.obj");
        int back_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_back.obj");
        int greenwall_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_greenwall.obj");
        int largebox_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_largebox.obj");
        int redwall_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_redwall.obj");
        int smallbox_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_smallbox.obj");

        Material white;
        white.albedo = glm::vec3(0.725f, 0.71f, 0.68f);

        Material red;
        red.albedo = glm::vec3(0.63f, 0.065f, 0.05f);

        Material green;
        green.albedo = glm::vec3(0.14f, 0.45f, 0.091f);

        int white_mat_id = scene->AddMaterial(white);
        int red_mat_id = scene->AddMaterial(red);
        int green_mat_id = scene->AddMaterial(green);

        Light light;
        light.type = LightType::QuadLight;
        light.position = glm::vec3(.34299999f, .54779997f, .22700010f);
        light.u = glm::vec3(.34299999f, .54779997f, .33200008f) - light.position;
        light.v = glm::vec3(.21300001f, .54779997f, .22700010f) - light.position;
        light.area = glm::length(glm::cross(light.u, light.v));
        light.emission = glm::vec3(17, 12, 4);

        int light_id = scene->AddLight(light);

        glm::mat4 xform = glm::scale(glm::vec3(0.01f));
        
        MeshInstance instance1("Ceiling", ceiling_mesh_id, xform, white_mat_id);
        MeshInstance instance2("Floor", floor_mesh_id, xform, white_mat_id);
        MeshInstance instance3("Back Wall", back_mesh_id, xform, white_mat_id);
        MeshInstance instance4("Left Wall", greenwall_mesh_id, xform, green_mat_id);
        MeshInstance instance5("Large Box", largebox_mesh_id, xform, white_mat_id);
        MeshInstance instance6("Right Wall", redwall_mesh_id, xform, red_mat_id);
        MeshInstance instance7("Small Box", smallbox_mesh_id, xform, white_mat_id);

        scene->AddMeshInstance(instance1);
        scene->AddMeshInstance(instance2);
        scene->AddMeshInstance(instance3);
        scene->AddMeshInstance(instance4);
        scene->AddMeshInstance(instance5);
        scene->AddMeshInstance(instance6);
        scene->AddMeshInstance(instance7);

        scene->AddHDR("./assets/HDR/vankleef.hdr");

        scene->CreateAccelerationStructures();

    }
}