#include "Shader.h"
#include "ShaderIncludes.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace GLSLPT
{
    Shader::Shader(const std::string& filePath, GLenum shaderType)
    {
		std::string source = GLSLPT::ShaderInclude::load(filePath);

        _object = glCreateShader(shaderType);
		printf("Compiling Shader %s -> %d\n", filePath.c_str(), int(_object));
        const GLchar *src = (const GLchar *)source.c_str();
        glShaderSource(_object, 1, &src, 0);
        glCompileShader(_object);
        GLint success = 0;
        glGetShaderiv(_object, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
			std::string msg;
            GLint logSize = 0;
            glGetShaderiv(_object, GL_INFO_LOG_LENGTH, &logSize);
            char *info = new char[logSize + 1];
            glGetShaderInfoLog(_object, logSize, NULL, info);
			msg += filePath;
			msg += "\n";
            msg += info;
            delete[] info;
            glDeleteShader(_object);
            _object = 0;
			printf("Shader compilation error %s\n", msg.c_str());
            throw std::runtime_error(msg);
        }
    }
	
    GLuint Shader::object() const
    {
        return _object;
    }
}