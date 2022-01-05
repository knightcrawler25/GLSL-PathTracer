/*
 * MIT License
 *
 * Copyright(c) 2019 Asif Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdexcept>
#include "Program.h"

namespace GLSLPT
{
    Program::Program(const std::vector<Shader> shaders)
    {
        object = glCreateProgram();
        for (unsigned i = 0; i < shaders.size(); i++)
            glAttachShader(object, shaders[i].getObject());

        glLinkProgram(object);
        for (unsigned i = 0; i < shaders.size(); i++)
            glDetachShader(object, shaders[i].getObject());
        GLint success = 0;
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::string msg("Error while linking program\n");
            GLint logSize = 0;
            glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logSize);
            char* info = new char[logSize + 1];
            glGetShaderInfoLog(object, logSize, NULL, info);
            msg += info;
            delete[] info;
            glDeleteProgram(object);
            object = 0;
            printf("Error %s\n", msg.c_str());
            throw std::runtime_error(msg.c_str());
        }
    }

    Program::~Program()
    {
        glDeleteProgram(object);
    }

    void Program::Use()
    {
        glUseProgram(object);
    }

    void Program::StopUsing()
    {
        glUseProgram(0);
    }

    GLuint Program::getObject()
    {
        return object;
    }
}