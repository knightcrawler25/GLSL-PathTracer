#ifndef __LOADER_H_
#define __LOADER_H_


#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <Scene.h>

bool LoadModel(Scene *scene, std::string filename, float materialId);
bool LoadScene(Scene *scene, const char* filename);

#endif