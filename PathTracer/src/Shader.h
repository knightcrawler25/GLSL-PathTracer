#pragma once

// third-party libraries
#include <GL/glew.h>
#include <string>

namespace GLSLPathTracer
{
    class Shader
    {
    private:
        GLuint _object;
    public:
        Shader(const std::string& filePath, GLuint shaderType);
        GLuint object() const;
    };
}