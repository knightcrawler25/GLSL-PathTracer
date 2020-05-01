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
		renderOptions.numTilesY = 5;
		renderOptions.numTilesX = 5;
		renderOptions.hdrMultiplier = 1.0f;
		renderOptions.useEnvMap = true;
		scene->addCamera(glm::vec3(0.276f, 0.275f, -0.75f), glm::vec3(0.276f, 0.275f, 0), 39.0f);

		int ceiling_mesh_id = scene->addMesh("./assets/cornell_box/cbox_ceiling.obj");
		int floor_mesh_id = scene->addMesh("./assets/cornell_box/cbox_floor.obj");
		int back_mesh_id = scene->addMesh("./assets/cornell_box/cbox_back.obj");
		int greenwall_mesh_id = scene->addMesh("./assets/cornell_box/cbox_greenwall.obj");
		int largebox_mesh_id = scene->addMesh("./assets/cornell_box/cbox_largebox.obj");
		int redwall_mesh_id = scene->addMesh("./assets/cornell_box/cbox_redwall.obj");
		int smallbox_mesh_id = scene->addMesh("./assets/cornell_box/cbox_smallbox.obj");

		Material white;
		white.albedo = glm::vec3(0.725f, 0.71f, 0.68f);

		Material red;
		red.albedo = glm::vec3(0.63f, 0.065f, 0.05f);

		Material green;
		green.albedo = glm::vec3(0.14f, 0.45f, 0.091f);

		int white_mat_id = scene->addMaterial(white);
		int red_mat_id = scene->addMaterial(red);
		int green_mat_id = scene->addMaterial(green);

		Light light;
		light.type = LightType::QuadLight;
		light.position = glm::vec3(.34299999f, .54779997f, .22700010f);
		light.u = glm::vec3(.34299999f, .54779997f, .33200008f) - light.position;
		light.v = glm::vec3(.21300001f, .54779997f, .22700010f) - light.position;
		light.area = glm::length(glm::cross(light.u, light.v));
		light.emission = glm::vec3(17, 12, 4);

		int light_id = scene->addLight(light);

		glm::mat4 xform = glm::scale(glm::vec3(0.01f));
		
		MeshInstance instance1(ceiling_mesh_id, xform, white_mat_id);
		MeshInstance instance2(floor_mesh_id, xform, white_mat_id);
		MeshInstance instance3(back_mesh_id, xform, white_mat_id);
		MeshInstance instance4(greenwall_mesh_id, xform, green_mat_id);
		MeshInstance instance5(largebox_mesh_id, xform, white_mat_id);
		MeshInstance instance6(redwall_mesh_id, xform, red_mat_id);
		MeshInstance instance7(smallbox_mesh_id, xform, white_mat_id);

		scene->addMeshInstance(instance1);
		scene->addMeshInstance(instance2);
		scene->addMeshInstance(instance3);
		scene->addMeshInstance(instance4);
		scene->addMeshInstance(instance5);
		scene->addMeshInstance(instance6);
		scene->addMeshInstance(instance7);

		scene->addHDR("./assets/HDR/sunset.hdr");

		scene->createAccelerationStructures();

	}
}