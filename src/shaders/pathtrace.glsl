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

uniform sampler2D accumTex;
uniform isamplerBuffer vertexIndicesTex;
uniform samplerBuffer verticesTex;
uniform samplerBuffer normalsTex;
uniform vec2 resolution;
uniform int topBVHIndex;
uniform int numIndices;

uniform vec3 up;
uniform vec3 right;
uniform vec3 forward;
uniform vec3 position;
uniform float fov;

#define INF 10000.0

bool ClosestHit(vec3 origin, vec3 direction, out vec3 normal)
{
    float t = INF;

    ivec3 triID = ivec3(-1);
    vec3 bary;

    for (int i = 0; i < numIndices; i++) // Loop through tris
    {
        ivec3 vertIndices = ivec3(texelFetch(vertexIndicesTex, i).xyz);

        vec4 v0 = texelFetch(verticesTex, vertIndices.x);
        vec4 v1 = texelFetch(verticesTex, vertIndices.y);
        vec4 v2 = texelFetch(verticesTex, vertIndices.z);

        vec3 e0 = v1.xyz - v0.xyz;
        vec3 e1 = v2.xyz - v0.xyz;
        vec3 pv = cross(direction, e1);
        float det = dot(e0, pv);

        vec3 tv = origin - v0.xyz;
        vec3 qv = cross(tv, e0);

        vec4 uvt;
        uvt.x = dot(tv, pv);
        uvt.y = dot(direction, qv);
        uvt.z = dot(e1, qv);
        uvt.xyz = uvt.xyz / det;
        uvt.w = 1.0 - uvt.x - uvt.y;

        if (all(greaterThanEqual(uvt, vec4(0.0))) && uvt.z < t)
        {
            t = uvt.z;
            triID = vertIndices;
            bary = uvt.wxy;
        }
    }

    if (t == INF)
        return false;
    else
    {
        vec4 n0 = texelFetch(normalsTex, triID.x);
        vec4 n1 = texelFetch(normalsTex, triID.y);
        vec4 n2 = texelFetch(normalsTex, triID.z);

        normal = normalize(n0.xyz * bary.x + n1.xyz * bary.y + n2.xyz * bary.z);
    }

    return true;
}

vec4 PathTrace(vec3 origin, vec3 direction)
{
    vec3 normal;
    bool hit = ClosestHit(origin, direction, normal);

    if (!hit)
        return vec4(0.5);
    else
        return vec4(normal, 1.0);
}

void main(void)
{
    vec2 d = (2.0 * TexCoords - 1.0);

    float scale = tan(fov * 0.5);
    d.y *= resolution.y / resolution.x * scale;
    d.x *= scale;
    vec3 dir = normalize(d.x * right + d.y * up + forward);

    vec4 accumColor = texture(accumTex, TexCoords);

    color = PathTrace(position, dir) + accumColor;
}