#pragma once

#include <string>
#include "Scene.h"

namespace GLSLPT
{
    class Scene;

    bool LoadSceneFromFile(const std::string &filename, Scene *scene, RenderOptions& renderOptions);
    // logger function. might be set at init time
    extern int(*Log)(const char* szFormat, ...);
}