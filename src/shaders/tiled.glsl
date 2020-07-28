/*
 * MIT License
 *
 * Copyright(c) 2019-2020 Asif Ali
 *
 * Authors/Contributors:
 *
 * Asif Ali
 * Cedric Guillemet
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

#version 300 es

precision highp float;
precision highp int;
precision highp sampler2D;
precision highp samplerCube;
precision highp isampler2D;
precision highp sampler2DArray;

out vec3 color;
in vec2 TexCoords;

#include common/uniforms.glsl
#include common/globals.glsl
#include common/intersection.glsl
#include common/sampling.glsl
#include common/anyhit.glsl
#include common/closest_hit.glsl
#include common/brdf.glsl
#include common/glass.glsl
#include common/pathtrace.glsl

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

    float xoffset = -1.0 + 2.0 * invNumTilesX * float(tileX);
    float yoffset = -1.0 + 2.0 * invNumTilesY * float(tileY);

    coords.x = map(coords.x, 0.0, 1.0, xoffset, xoffset + 2.0 * invNumTilesX);
    coords.y = map(coords.y, 0.0, 1.0, yoffset, yoffset + 2.0 * invNumTilesY);

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

    coords.x = map(TexCoords.x, 0.0, 1.0, invNumTilesX * float(tileX), invNumTilesX * float(tileX) + invNumTilesX);
    coords.y = map(TexCoords.y, 0.0, 1.0, invNumTilesY * float(tileY), invNumTilesY * float(tileY) + invNumTilesY);

    vec3 accumColor = texture(accumTexture, coords).xyz;

    if (isCameraMoving)
        accumColor = vec3(0.);

    vec3 pixelColor = PathTrace(ray);

    color = pixelColor + accumColor;
}