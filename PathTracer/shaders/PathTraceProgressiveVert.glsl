#version 300 es

precision highp float;
precision highp int;
precision highp sampler2D;
precision highp samplerCube;
precision highp isampler2D;
precision highp sampler2DArray;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;

void main()
{
	gl_Position = vec4(position.x,position.y,0.0,1.0);
	TexCoords = texCoords;
}