#pragma once
#include "Config.h"

namespace GLSLPT
{
    class Program;

    class Quad
    {
    public:
        Quad();
        void Draw(Program *);
    private:
        GLuint vao, vbo;
    };
}