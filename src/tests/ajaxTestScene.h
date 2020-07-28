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
    void loadAjaxTestScene(Scene* scene, RenderOptions &renderOptions)
    {
        renderOptions.maxDepth = 2;
        renderOptions.tileHeight = 128;
        renderOptions.tileWidth = 128;
        renderOptions.hdrMultiplier = 5.0f;
        renderOptions.useEnvMap = true;
        scene->AddCamera(glm::vec3(0.0f, 0.125f, -0.45f), glm::vec3(0.0f, 0.125f, 0.0f), 60.0f);

        int mesh_id = scene->AddMesh("./assets/ajax/ajax.obj");

        Material black;
        black.albedo = glm::vec3(0.1f, 0.1f, 0.1f);
        black.roughness = 0.01f;
        black.metallic = 1.0f;

        Material red_plastic;
        red_plastic.albedo = glm::vec3(1.0, 0.0, 0.0);
        red_plastic.roughness = 0.01;
        red_plastic.metallic = 0.0;

        Material gold;
        gold.albedo = glm::vec3(1.0, 0.71, 0.29);
        gold.roughness = 0.2;
        gold.metallic = 1.0;

        int black_mat_id = scene->AddMaterial(black);
        int red_mat_id = scene->AddMaterial(red_plastic);
        int gold_mat_id = scene->AddMaterial(gold);

        glm::mat4 xform;
        glm::mat4 xform1;
        glm::mat4 xform2;

        xform = glm::scale(glm::vec3(0.25f));
        xform1 = glm::scale(glm::vec3(0.25f)) * glm::translate(glm::vec3(0.6f, 0.0f, 0.0f));
        xform2 = glm::scale(glm::vec3(0.25f)) * glm::translate(glm::vec3(-0.6f, 0.0f, 0.0f));
        
        MeshInstance instance("Ajax", mesh_id, xform, black_mat_id);
        //MeshInstance instance1(mesh_id, xform1, gold_mat_id);
        //MeshInstance instance2(mesh_id, xform2, red_mat_id);

        scene->AddMeshInstance(instance);
        //scene->addMeshInstance(instance1);
        //scene->addMeshInstance(instance2);

        scene->AddHDR("./assets/HDR/vankleef.hdr");

        scene->CreateAccelerationStructures();

    }
}