#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include "Scene.h"

namespace GLSLPT
{
	void loadBoyTestScene(Scene* scene, RenderOptions &renderOptions)
	{
		renderOptions.maxDepth = 2;
		renderOptions.numTilesY = 5;
		renderOptions.numTilesX = 5;
		renderOptions.hdrMultiplier = 1.0f;
		renderOptions.useEnvMap = false;
		scene->addCamera(glm::vec3(0.3f, 0.11f, 0.0f), glm::vec3(0.2f, 0.095f, 0.0f), 35.0f);

		int mesh_id1 = scene->addMesh("./assets/Figurine/head.obj");
		int mesh_id2 = scene->addMesh("./assets/Figurine/body.obj");
		int mesh_id3 = scene->addMesh("./assets/Figurine/base.obj");
		int mesh_id4 = scene->addMesh("./assets/Figurine/background.obj");

		Material head;
		Material body;
		Material base;
		Material white;

		int headAlbedo = scene->addTexture("./assets/Figurine/textures/01_Head_Base_Color.png");
		int bodyAlbedo = scene->addTexture("./assets/Figurine/textures/02_Body_Base_Color.png");
		int baseAlbedo = scene->addTexture("./assets/Figurine/textures/03_Base_Base_Color.png");
		int bgAlbedo = scene->addTexture("./assets/Figurine/textures/grid.jpg");

		int headMatRgh = scene->addTexture("./assets/Figurine/textures/01_Head_MetallicRoughness.png");
		int bodyMatRgh = scene->addTexture("./assets/Figurine/textures/02_Body_MetallicRoughness.png");
		int baseMatRgh = scene->addTexture("./assets/Figurine/textures/03_Base_MetallicRoughness.png");

		head.albedoTexID = headAlbedo;
		head.metallicRoughnessTexID = headMatRgh;
		
		body.albedoTexID = bodyAlbedo;
		body.metallicRoughnessTexID = bodyMatRgh;

		base.albedoTexID = baseAlbedo;
		base.metallicRoughnessTexID = baseMatRgh;

		white.albedoTexID = bgAlbedo;

		int head_mat_id = scene->addMaterial(head);
		int body_mat_id = scene->addMaterial(body);
		int base_mat_id = scene->addMaterial(base);
		int white_mat_id = scene->addMaterial(white);

		Light light;
		light.type = LightType::AreaLight;
		light.position = glm::vec3(-0.103555f, 0.284840f, 0.606827f);
		light.u = glm::vec3(-0.103555f, 0.465656f, 0.521355f) - light.position;
		light.v = glm::vec3(0.096445f, 0.284840f, 0.606827f) - light.position;
		light.area = glm::length(glm::cross(light.u, light.v));
		light.emission = glm::vec3(40, 41, 41);

		Light light2;
		light2.type = LightType::AreaLight;
		light2.position = glm::vec3(0.303145f, 0.461806f, -0.450967f);
		light2.u = glm::vec3(0.362568f, 0.280251f, -0.510182f) - light2.position;
		light2.v = glm::vec3(0.447143f, 0.461806f, -0.306465f) - light2.position;
		light2.area = glm::length(glm::cross(light2.u, light2.v));
		light2.emission = glm::vec3(40, 41, 41);

		int light1_id = scene->addLight(light);
		int light2_id = scene->addLight(light2);

		glm::mat4 xform1;
		glm::mat4 xform2;
		glm::mat4 xform3;
		glm::mat4 xform4;
		glm::mat4 xform5;
		xform2 = glm::translate(glm::vec3(0, 0,-0.05));
		xform3 = glm::translate(glm::vec3(0, 0,+0.05));
		xform4 = glm::translate(glm::vec3(-0.1, 0.0, -0.15));
		//xform4 *= glm::rotate(90.0f, glm::vec3(0.0, 0, 1));
		xform5 = glm::translate(glm::vec3(-0.1, 0, +0.15));

		MeshInstance instance1(mesh_id4, xform1, white_mat_id);

		MeshInstance instance2(mesh_id1, xform2, head_mat_id);
		MeshInstance instance3(mesh_id2, xform2, body_mat_id);
		MeshInstance instance4(mesh_id3, xform2, base_mat_id);
		
		MeshInstance instance5(mesh_id1, xform3, head_mat_id);
		MeshInstance instance6(mesh_id2, xform3, body_mat_id);
		MeshInstance instance7(mesh_id3, xform3, base_mat_id);

		MeshInstance instance8(mesh_id1, xform4, head_mat_id);
		MeshInstance instance9(mesh_id2, xform4, body_mat_id);
		MeshInstance instance10(mesh_id3, xform4, base_mat_id);

		MeshInstance instance11(mesh_id1, xform5, head_mat_id);
		MeshInstance instance12(mesh_id2, xform5, body_mat_id);
		MeshInstance instance13(mesh_id3, xform5, base_mat_id);

		scene->addMeshInstance(instance1);
		scene->addMeshInstance(instance2);
		scene->addMeshInstance(instance3);
		scene->addMeshInstance(instance4);
		scene->addMeshInstance(instance5);
		scene->addMeshInstance(instance6);
		scene->addMeshInstance(instance7);
		scene->addMeshInstance(instance8);
		scene->addMeshInstance(instance9);
		scene->addMeshInstance(instance10);
		scene->addMeshInstance(instance11);
		scene->addMeshInstance(instance12);
		scene->addMeshInstance(instance13);

		scene->createAccelerationStructures();

	}
}