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
    void loadAjaxTestScene(Scene* scene, RenderOptions &renderOptions)
    {
        renderOptions.maxDepth = 2;
        renderOptions.tileHeight = 128;
        renderOptions.tileWidth = 128;
        renderOptions.hdrMultiplier = 5.0f;
        renderOptions.useEnvMap = true;
        scene->AddCamera(Vec3(0.0f, 0.125f, -0.45f), Vec3(0.0f, 0.125f, 0.0f), 60.0f);

        int mesh_id = scene->AddMesh("./assets/ajax/ajax.obj");

        Material black;
        black.albedo = Vec3(0.1f, 0.1f, 0.1f);
        black.roughness = 0.01f;
        black.metallic = 1.0f;

        Material red_plastic;
        red_plastic.albedo = Vec3(1.0f, 0.0f, 0.0f);
        red_plastic.roughness = 0.01f;
        red_plastic.metallic = 0.0f;

        Material gold;
        gold.albedo = Vec3(1.0f, 0.71f, 0.29f);
        gold.roughness = 0.2f;
        gold.metallic = 1.0f;

        int black_mat_id = scene->AddMaterial(black);
        int red_mat_id   = scene->AddMaterial(red_plastic);
        int gold_mat_id  = scene->AddMaterial(gold);

        Mat4 xform;
        Mat4 xform1;
        Mat4 xform2;

        xform  = Mat4::Scale(Vec3(0.25f, 0.25f, 0.25f));
        xform1 = Mat4::Scale(Vec3(0.25f, 0.25f, 0.25f)) * Mat4::Translate(Vec3(0.2f, 0.0f, 0.0f));
        xform2 = Mat4::Scale(Vec3(0.25f, 0.25f, 0.25f)) * Mat4::Translate(Vec3(-0.2f, 0.0f, 0.0f));
        
        MeshInstance instance("Ajax Black", mesh_id, xform,  black_mat_id);
        MeshInstance instance1("Ajax Gold", mesh_id, xform1, gold_mat_id);
        MeshInstance instance2("Ajax Red",  mesh_id, xform2, red_mat_id);

        scene->AddMeshInstance(instance);
        scene->AddMeshInstance(instance1);
        scene->AddMeshInstance(instance2);

        scene->AddHDR("./assets/HDR/sunset.hdr");

        scene->CreateAccelerationStructures();

    }
}