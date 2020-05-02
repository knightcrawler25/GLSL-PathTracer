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

float map(float value, float low1, float high1, float low2, float high2)
{
	return low2 + ((value - low1) * (high2 - low2)) / (high1 - low1);
}

void main(void)
{
	seed = gl_FragCoord.xy;

	float r1 = 2.0 * rand();
	float r2 = 2.0 * rand();

	vec2 coords = TexCoords;

	float xoffset = -1.0 + 2.0 * invTileWidth * float(tileX);
	float yoffset = -1.0 + 2.0 * invTileHeight * float(tileY);

	coords.x = map(coords.x, 0.0, 1.0, xoffset, xoffset + 2.0 * invTileWidth);
	coords.y = map(coords.y, 0.0, 1.0, yoffset, yoffset + 2.0 * invTileHeight);

	vec2 jitter;
	jitter.x = r1 < 1.0 ? sqrt(r1) - 1.0 : 1.0 - sqrt(2.0 - r1);
	jitter.y = r2 < 1.0 ? sqrt(r2) - 1.0 : 1.0 - sqrt(2.0 - r2);

	jitter /= (screenResolution * 0.5);
	vec2 d = coords + jitter;

	float scale = tan(camera.fov * 0.5);
	d.y *= screenResolution.y / screenResolution.x * scale;
	d.x *= scale;
	vec3 rayDir = normalize(d.x * camera.right + d.y * camera.up + camera.forward);

	vec3 focalPoint = camera.focalDist * rayDir;
	float cam_r1 = rand() * TWO_PI;
	float cam_r2 = rand() * camera.aperture;
	vec3 randomAperturePos = (cos(cam_r1) * camera.right + sin(cam_r1) * camera.up) * sqrt(cam_r2);
	vec3 finalRayDir = normalize(focalPoint - randomAperturePos);

	Ray ray = Ray(camera.position + randomAperturePos, finalRayDir);

	coords.x = map(TexCoords.x, 0.0, 1.0, invTileWidth * float(tileX), invTileWidth * float(tileX) + invTileWidth);
	coords.y = map(TexCoords.y, 0.0, 1.0, invTileHeight * float(tileY), invTileHeight * float(tileY) + invTileHeight);

	vec3 accumColor = texture(accumTexture, coords).xyz;

	if (isCameraMoving)
		accumColor = vec3(0.);

	vec3 pixelColor = PathTrace(ray);

	color = pixelColor + accumColor;
}