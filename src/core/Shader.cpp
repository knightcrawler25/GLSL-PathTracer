/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this softwareand associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace GLSLPT
{
    Shader::Shader(const ShaderInclude::ShaderSource& sourceObj, GLenum shaderType)
    {
        object = glCreateShader(shaderType);
        printf("Compiling Shader %s -> %d\n", sourceObj.path.c_str(), int(object));
        const GLchar *src = (const GLchar *)sourceObj.src.c_str();
        glShaderSource(object, 1, &src, 0);
        glCompileShader(object);
        GLint success = 0;
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::string msg;
            GLint logSize = 0;
            glGetShaderiv(object, GL_INFO_LOG_LENGTH, &logSize);
            char *info = new char[logSize + 1];
            glGetShaderInfoLog(object, logSize, NULL, info);
            msg += sourceObj.path;
            msg += "\n";
            msg += info;
            delete[] info;
            glDeleteShader(object);
            object = 0;
            printf("Shader compilation error %s\n", msg.c_str());
            throw std::runtime_error(msg.c_str());
        }
    }
    
    GLuint Shader::getObject() const
    {
        return object;
    }
}