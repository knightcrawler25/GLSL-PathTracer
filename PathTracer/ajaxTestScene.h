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
		renderOptions.numTilesY = 5;
		renderOptions.numTilesX = 5;
		renderOptions.hdrMultiplier = 5.0f;
		renderOptions.useEnvMap = true;
		scene->addCamera(glm::vec3(0.0f, 0.125f, -0.45f), glm::vec3(0.0f, 0.125f, 0.0f), 35.0f);

		int mesh_id = scene->addMesh("./assets/ajax/ajax.obj");

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

		int black_mat_id = scene->addMaterial(black);
		int red_mat_id = scene->addMaterial(red_plastic);
		int gold_mat_id = scene->addMaterial(gold);

		glm::mat4 xform;
		glm::mat4 xform1;
		glm::mat4 xform2;

		xform = glm::scale(glm::vec3(0.25f));
		xform1 = glm::scale(glm::vec3(0.25f)) * glm::translate(glm::vec3(0.6f, 0.0f, 0.0f));
		xform2 = glm::scale(glm::vec3(0.25f)) * glm::translate(glm::vec3(-0.6f, 0.0f, 0.0f));
		
		MeshInstance instance(mesh_id, xform, black_mat_id);
		//MeshInstance instance1(mesh_id, xform1, gold_mat_id);
		//MeshInstance instance2(mesh_id, xform2, red_mat_id);

		scene->addMeshInstance(instance);
		//scene->addMeshInstance(instance1);
		//scene->addMeshInstance(instance2);

		scene->addHDR("./assets/HDR/sunset.hdr");

		scene->createAccelerationStructures();

	}
}