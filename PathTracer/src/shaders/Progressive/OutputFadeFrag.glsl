#version 430

out vec4 color;
in vec2 TexCoords;

uniform sampler2D pathTraceTextureHalf;
uniform sampler2D pathTraceTexture;

uniform float invSampleCounter;
uniform float fadeAmt;

vec4 ToneMap(in vec4 c, float limit)
{
	float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;

	return c * 1.0/(1.0 + luminance/limit);
}

void main()
{
	vec4 color1 = texture(pathTraceTextureHalf, TexCoords);
	vec4 color2 = texture(pathTraceTexture, TexCoords) * invSampleCounter;

	color1 = pow(ToneMap(color1, 1.5), vec4(1.0 / 2.2));
	color2 = pow(ToneMap(color2, 1.5), vec4(1.0 / 2.2));

	color = mix(color1, color2, fadeAmt);
}