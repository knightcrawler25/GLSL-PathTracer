#pragma once
#include "Config.h"

namespace GLSLPathTracer
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