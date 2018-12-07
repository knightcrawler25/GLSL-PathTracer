#include <Scene.h>
#include <iostream>

void Scene::addCamera(glm::vec3 pos, glm::vec3 lookAt)
{
	camera = new Camera(pos, lookAt);
}
