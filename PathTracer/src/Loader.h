#pragma once

#include <string>

namespace GLSLPathTracer
{
    class Scene;

    void LoadModel(Scene *scene, const std::string &filename, float materialId);
    bool LoadScene(Scene *scene, const std::string &filename);
}