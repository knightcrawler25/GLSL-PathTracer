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

#version 330

out vec4 color;
in vec2 TexCoords;

#include common/uniforms.glsl
#include common/globals.glsl
#include common/intersection.glsl
#include common/sampling.glsl
#include common/envmap.glsl
#include common/anyhit.glsl
#include common/closest_hit.glsl
#include common/disney.glsl
#include common/lambert.glsl
#include common/pathtrace.glsl

void main(void)
{
    vec2 coordsTile = mix(tileOffset, tileOffset + invNumTiles, TexCoords);

    InitRNG(gl_FragCoord.xy, frameNum);

    float r1 = 2.0 * rand();
    float r2 = 2.0 * rand();

    vec2 jitter;
    jitter.x = r1 < 1.0 ? sqrt(r1) - 1.0 : 1.0 - sqrt(2.0 - r1);
    jitter.y = r2 < 1.0 ? sqrt(r2) - 1.0 : 1.0 - sqrt(2.0 - r2);

    jitter /= (resolution * 0.5);
    vec2 d = (coordsTile * 2.0 - 1.0) + jitter;

    float scale = tan(camera.fov * 0.5);
    d.y *= resolution.y / resolution.x * scale;
    d.x *= scale;
    vec3 rayDir = normalize(d.x * camera.right + d.y * camera.up + camera.forward);

    vec3 focalPoint = camera.focalDist * rayDir;
    float cam_r1 = rand() * TWO_PI;
    float cam_r2 = rand() * camera.aperture;
    vec3 randomAperturePos = (cos(cam_r1) * camera.right + sin(cam_r1) * camera.up) * sqrt(cam_r2);
    vec3 finalRayDir = normalize(focalPoint - randomAperturePos);

    Ray ray = Ray(camera.position + randomAperturePos, finalRayDir);

    vec4 accumColor = texture(accumTexture, coordsTile);

    vec4 pixelColor = PathTrace(ray);

    color = pixelColor + accumColor;
}