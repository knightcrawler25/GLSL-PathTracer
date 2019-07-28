#include "Shader.h"
#include "Loader.h"

#include <iostream>
#include <fstream>
#include <sstream>

namespace GLSLPathTracer
{
    Shader::Shader(const std::string& filePath, GLenum shaderType)
    {
        std::ifstream f;
        f.open(filePath.c_str(), std::ios::in | std::ios::binary);
        if (!f.is_open())
        {
            Log("Failed to open file: %s\n", filePath.c_str());
            return;
        }

        //read whole file into stringstream buffer
        std::stringstream buffer;
        buffer << f.rdbuf();

        std::string source = buffer.str();
        _object = glCreateShader(shaderType);
        const GLchar *src = (const GLchar *)source.c_str();
        glShaderSource(_object, 1, &src, 0);
        glCompileShader(_object);
        GLint success = 0;
        glGetShaderiv(_object, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::string msg("Error while compiling shader\n");
            GLint logSize = 0;
            glGetShaderiv(_object, GL_INFO_LOG_LENGTH, &logSize);
            char *info = new char[logSize + 1];
            glGetShaderInfoLog(_object, logSize, NULL, info);
            msg += info;
            delete[] info;
            glDeleteShader(_object);
            _object = 0;
            Log("Shader compilation error %s\n", msg.c_str());
            throw std::runtime_error(msg);
        }
    }

    GLuint Shader::object() const
    {
        return _object;
    }
}