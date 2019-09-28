#include "Program.h"

namespace GLSLPT
{
    Program::Program(const std::vector<Shader> shaders)
    {
        _object = glCreateProgram();
        for (unsigned i = 0; i < shaders.size(); i++)
            glAttachShader(_object, shaders[i].object());

        glLinkProgram(_object);

        for (unsigned i = 0; i < shaders.size(); i++)
            glDetachShader(_object, shaders[i].object());
        GLint success = 0;
        glGetProgramiv(_object, GL_LINK_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::string msg("Error while linking program\n");
            GLint logSize = 0;
            glGetProgramiv(_object, GL_INFO_LOG_LENGTH, &logSize);
            char *info = new char[logSize + 1];
            glGetShaderInfoLog(_object, logSize, NULL, info);
            msg += info;
            delete[] info;
            glDeleteProgram(_object);
            _object = 0;
            //std::cout << msg << std::endl; TODO
            throw std::runtime_error(msg);
        }
    }

    Program::~Program()
    {
        glDeleteProgram(_object);
    }

    void Program::use()
    {
        glUseProgram(_object);
    }

    void Program::stopUsing()
    {
        glUseProgram(0);
    }

    GLuint Program::object()
    {
        return _object;
    }
}