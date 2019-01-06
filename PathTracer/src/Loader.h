#pragma once

#include <string>

namespace GLSLPathTracer
{
    class Scene;

    bool LoadModel(Scene *scene, const std::string &filename, float materialId);
    Scene* LoadScene(const std::string &filename);
    // logger function. might be set at init time
    extern int(*Log)(const char* szFormat, ...);
}