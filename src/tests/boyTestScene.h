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
    void loadBoyTestScene(Scene* scene, RenderOptions &renderOptions)
    {
        renderOptions.maxDepth = 2;
        renderOptions.tileHeight = 256;
        renderOptions.tileWidth = 144;
        renderOptions.hdrMultiplier = 1.0f;
        renderOptions.resolution = iVec2(1280, 720);
        renderOptions.useEnvMap = false;
        scene->AddCamera(Vec3(0.3f, 0.11f, 0.0f), Vec3(0.2f, 0.095f, 0.0f), 60.0f);
        scene->camera->aperture = 1e-6f; 
        scene->camera->focalDist = 0.262f;

        int mesh_id1 = scene->AddMesh("./assets/Figurine/head.obj");
        int mesh_id2 = scene->AddMesh("./assets/Figurine/body.obj");
        int mesh_id3 = scene->AddMesh("./assets/Figurine/base.obj");
        int mesh_id4 = scene->AddMesh("./assets/Figurine/background.obj");

        Material head;
        Material body;
        Material base;
        Material white;
        Material gold;
        Material red_plastic;

        int headAlbedo = scene->AddTexture("./assets/Figurine/textures/01_Head_Base_Color.png");
        int bodyAlbedo = scene->AddTexture("./assets/Figurine/textures/02_Body_Base_Color.png");
        int baseAlbedo = scene->AddTexture("./assets/Figurine/textures/03_Base_Base_Color.png");
        int bgAlbedo   = scene->AddTexture("./assets/Figurine/textures/grid.jpg");

        int headMatRgh = scene->AddTexture("./assets/Figurine/textures/01_Head_MetallicRoughness.png");
        int bodyMatRgh = scene->AddTexture("./assets/Figurine/textures/02_Body_MetallicRoughness.png");
        int baseMatRgh = scene->AddTexture("./assets/Figurine/textures/03_Base_MetallicRoughness.png");

        head.albedoTexID = headAlbedo;
        head.metallicRoughnessTexID = headMatRgh;
        
        body.albedoTexID = bodyAlbedo;
        body.metallicRoughnessTexID = bodyMatRgh;

        base.albedoTexID = baseAlbedo;
        base.metallicRoughnessTexID = baseMatRgh;

        white.albedoTexID = bgAlbedo;

        gold.albedo = Vec3(1.0f, 0.71f, 0.29f);
        gold.roughness = 0.2f;
        gold.metallic = 1.0f;

        red_plastic.albedo = Vec3(1.0f, 0.0f, 0.0f);
        red_plastic.roughness = 0.01f;
        red_plastic.metallic = 0.0f;

        int head_mat_id  = scene->AddMaterial(head);
        int body_mat_id  = scene->AddMaterial(body);
        int base_mat_id  = scene->AddMaterial(base);
        int white_mat_id = scene->AddMaterial(white);
        int gold_mat_id  = scene->AddMaterial(gold);
        int red_mat_id   = scene->AddMaterial(red_plastic);

        Light light;
        light.type = LightType::QuadLight;
        light.position = Vec3(-0.103555f, 0.284840f, 0.606827f);
        light.u = Vec3(-0.103555f, 0.465656f, 0.521355f) - light.position;
        light.v = Vec3(0.096445f, 0.284840f, 0.606827f) - light.position;
        light.area = Vec3::Length(Vec3::Cross(light.u, light.v));
        light.emission = Vec3(40, 41, 41);

        Light light2;
        light2.type = LightType::QuadLight;
        light2.position = Vec3(0.303145f, 0.461806f, -0.450967f);
        light2.u = Vec3(0.362568f, 0.280251f, -0.510182f) - light2.position;
        light2.v = Vec3(0.447143f, 0.461806f, -0.306465f) - light2.position;
        light2.area = Vec3::Length(Vec3::Cross(light2.u, light2.v));
        light2.emission = Vec3(40, 41, 41);

        int light1_id = scene->AddLight(light);
        int light2_id = scene->AddLight(light2);

        Mat4 xform_base = Mat4::Translate(Vec3(0, 0.0075, 0));
        Mat4 xform_body = Mat4::Translate(Vec3(0, 0.049, 0));
        Mat4 xform_head = Mat4::Translate(Vec3(0.017, 0.107, 0));

        Mat4 xform1;
        Mat4 xform2;
        Mat4 xform3;
        Mat4 xform4;
        Mat4 xform5;

        xform2 = Mat4::Translate(Vec3(0, 0, -0.05));
        xform3 = Mat4::Translate(Vec3(0, 0,  0.05));
        xform4 = Mat4::Translate(Vec3(-0.1, 0.0, -0.15));
        //xform4 Mat4::Rotate(90.0f, Vec3(0.0, 0, 1));
        xform5 = Mat4::Translate(Vec3(-0.1, 0, 0.15));

        MeshInstance instance1("background.obj",  mesh_id4, xform1, white_mat_id);

        MeshInstance instance2("head1.obj", mesh_id1, xform_head * xform2, head_mat_id);
        MeshInstance instance3("body1.obj", mesh_id2, xform_body * xform2, body_mat_id);
        MeshInstance instance4("base1.obj", mesh_id3, xform_base * xform2, base_mat_id);
        
        MeshInstance instance5("head2.obj", mesh_id1, xform_head * xform3, head_mat_id);
        MeshInstance instance6("body2.obj", mesh_id2, xform_body * xform3, body_mat_id);
        MeshInstance instance7("base2.obj", mesh_id3, xform_base * xform3, base_mat_id);

        MeshInstance instance8("head3.obj", mesh_id1, xform_head * xform4, head_mat_id);
        MeshInstance instance9("body3.obj", mesh_id2, xform_body * xform4, body_mat_id);
        MeshInstance instance10("base3.obj", mesh_id3, xform_base * xform4, base_mat_id);

        MeshInstance instance11("head4.obj", mesh_id1, xform_head * xform5, head_mat_id);
        MeshInstance instance12("body4.obj", mesh_id2, xform_body * xform5, body_mat_id);
        MeshInstance instance13("base4.obj", mesh_id3, xform_base * xform5, base_mat_id);

        scene->AddMeshInstance(instance1);
        scene->AddMeshInstance(instance2);
        scene->AddMeshInstance(instance3);
        scene->AddMeshInstance(instance4);
        scene->AddMeshInstance(instance5);
        scene->AddMeshInstance(instance6);
        scene->AddMeshInstance(instance7);
        scene->AddMeshInstance(instance8);
        scene->AddMeshInstance(instance9);
        scene->AddMeshInstance(instance10);
        scene->AddMeshInstance(instance11);
        scene->AddMeshInstance(instance12);
        scene->AddMeshInstance(instance13);

        scene->CreateAccelerationStructures();

    }
}