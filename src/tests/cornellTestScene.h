/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
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
        renderOptions.resolution.x = 800;
        renderOptions.resolution.y = 800;
        scene->AddCamera(Vec3(0.276f, 0.275f, -0.75f), Vec3(0.276f, 0.275f, 0), 40.0f);

        int ceiling_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_ceiling.obj");
        int floor_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_floor.obj");
        int back_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_back.obj");
        int greenwall_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_greenwall.obj");
        int largebox_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_largebox.obj");
        int redwall_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_redwall.obj");
        int smallbox_mesh_id = scene->AddMesh("./assets/cornell_box/cbox_smallbox.obj");

        Material white;
        white.albedo = Vec3(0.725f, 0.71f, 0.68f);

        Material red;
        red.albedo = Vec3(0.63f, 0.065f, 0.05f);

        Material green;
        green.albedo = Vec3(0.14f, 0.45f, 0.091f);

        int white_mat_id = scene->AddMaterial(white);
        int red_mat_id   = scene->AddMaterial(red);
        int green_mat_id = scene->AddMaterial(green);

        Light light;
        light.type = LightType::QuadLight;
        light.position = Vec3(.34299999f, .54779997f, .22700010f);
        light.u = Vec3(.34299999f, .54779997f, .33200008f) - light.position;
        light.v = Vec3(.21300001f, .54779997f, .22700010f) - light.position;
        light.area = Vec3::Length(Vec3::Cross(light.u, light.v));
        light.emission = Vec3(17, 12, 4);

        int light_id = scene->AddLight(light);

        Mat4 xform = Mat4::Scale(Vec3(0.01f, 0.01f, 0.01f));
        
        MeshInstance instance1("Ceiling", ceiling_mesh_id, xform * Mat4::Translate(Vec3(0.278f, 0.5488f, .27955f)), white_mat_id);
        MeshInstance instance2("Floor", floor_mesh_id, xform * Mat4::Translate(Vec3(0.2756f, 0.0f, 0.2796f)), white_mat_id);
        MeshInstance instance3("Back Wall", back_mesh_id, xform * Mat4::Translate(Vec3(0.2764f, 0.2744f, 0.5592f)), white_mat_id);
        MeshInstance instance4("Left Wall", greenwall_mesh_id, xform * Mat4::Translate(Vec3(0.0f, 0.2744f, 0.2796f)), green_mat_id);
        MeshInstance instance5("Large Box", largebox_mesh_id, xform * Mat4::Translate(Vec3(0.3685f, 0.165f, 0.35125f)), white_mat_id);
        MeshInstance instance6("Right Wall", redwall_mesh_id, xform * Mat4::Translate(Vec3(0.5536f, 0.2744f, 0.2796f)), red_mat_id);
        MeshInstance instance7("Small Box", smallbox_mesh_id, xform * Mat4::Translate(Vec3(0.1855f, 0.0825f, 0.169f)), white_mat_id);

        scene->AddMeshInstance(instance1);
        scene->AddMeshInstance(instance2);
        scene->AddMeshInstance(instance3);
        scene->AddMeshInstance(instance4);
        scene->AddMeshInstance(instance5);
        scene->AddMeshInstance(instance6);
        scene->AddMeshInstance(instance7);

        scene->AddHDR("./assets/HDR/sunset.hdr");

        scene->CreateAccelerationStructures();

    }
}