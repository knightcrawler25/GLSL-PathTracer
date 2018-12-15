#include <Scene.h>
#include <iostream>

void Scene::addCamera(glm::vec3 pos, glm::vec3 lookAt, float fov)
{
	camera = new Camera(pos, lookAt, fov);
}
