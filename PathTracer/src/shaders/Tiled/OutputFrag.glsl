#version 330

out vec4 color;
in vec2 TexCoords;

uniform sampler2D pathTraceTexture;
uniform float invSampleCounter;

vec4 ToneMap(in vec4 c, float limit)
{
	float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;

	return c * 1.0/(1.0 + luminance/limit);
}

void main()
{
	color = texture(pathTraceTexture, TexCoords) * invSampleCounter;
}