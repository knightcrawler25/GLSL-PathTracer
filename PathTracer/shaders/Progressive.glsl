#version 300 es

precision highp float;
precision highp int;
precision highp sampler2D;
precision highp samplerCube;
precision highp isampler2D;
precision highp sampler2DArray;

out vec3 color;
in vec2 TexCoords;

#include common/Uniforms.glsl
#include common/Globals.glsl
#include common/Intersection.glsl
#include common/Sampling.glsl
#include common/Anyhit.glsl
#include common/Closesthit.glsl
#include common/UE4BRDF.glsl
#include common/GlassBSDF.glsl
#include common/Pathtrace.glsl

void main(void)
{
	seed = gl_FragCoord.xy;

	float r1 = 2.0 * rand();
	float r2 = 2.0 * rand();

	vec2 jitter;

	jitter.x = r1 < 1.0 ? sqrt(r1) - 1.0 : 1.0 - sqrt(2.0 - r1);
	jitter.y = r2 < 1.0 ? sqrt(r2) - 1.0 : 1.0 - sqrt(2.0 - r2);
	jitter /= (screenResolution * 0.5);

	float scale = tan(camera.fov * 0.5);
	vec2 d = (2.0 * TexCoords - 1.0) + jitter;
	d.x *= screenResolution.x / screenResolution.y * scale;
	d.y *= scale;
	vec3 rayDir = normalize(d.x * camera.right + d.y * camera.up + camera.forward);

	vec3 focalPoint = camera.focalDist * rayDir;
	float cam_r1 = rand() * TWO_PI; 
	float cam_r2 = rand() * camera.aperture;
	vec3 randomAperturePos = (cos(cam_r1) * camera.right + sin(cam_r1) * camera.up) * sqrt(cam_r2);
	vec3 finalRayDir = normalize(focalPoint - randomAperturePos);

	Ray ray = Ray(camera.position + randomAperturePos, finalRayDir);

	vec3 pixelColor = PathTrace(ray);

	color = pixelColor;
}