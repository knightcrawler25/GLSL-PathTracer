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

#define PI        3.14159265358979323
#define TWO_PI    6.28318530717958648
#define INFINITY  1000000.0
#define EPS 0.00001

mat4 transform;

vec2 seed;
vec3 tempTexCoords;

struct Ray 
{ 
    vec3 origin; 
    vec3 direction; 
};

struct Material
{
    vec3 albedo; 
    float specular; 
    vec3 emission; 
    float anisotropic; 
    float metallic; 
    float roughness; 
    float subsurface; 
    float specularTint; 
    float sheen; 
    float sheenTint; 
    float clearcoat; 
    float clearcoatGloss; 
    float transmission; 
    float ior; 
    vec3 extinction;
    vec3 texIDs;
};

struct Camera 
{ 
    vec3 up; 
    vec3 right;
    vec3 forward; 
    vec3 position; 
    float fov; 
    float focalDist;
    float aperture; 
};

struct Light 
{ 
    vec3 position; 
    vec3 emission; 
    vec3 u; 
    vec3 v; 
    vec3 radiusAreaType; 
};

struct State 
{ 
    int depth;
    float hitDist;
    vec3 fhp;

    vec3 normal; 
    vec3 ffnormal; 
    vec3 tangent; 
    vec3 bitangent;
    
    bool isEmitter;
    bool specularBounce;

    vec2 texCoord; 
    vec3 bary; 
    ivec3 triID; 
    int matID;
    Material mat;
};

struct BsdfSampleRec 
{ 
    vec3 bsdfDir; 
    float pdf;
};

struct LightSampleRec 
{ 
    vec3 surfacePos; 
    vec3 normal; 
    vec3 emission; 
    float pdf; 
};

uniform Camera camera;

//-----------
float rand()
//-----------
{
    seed -= randomVector.xy;
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}