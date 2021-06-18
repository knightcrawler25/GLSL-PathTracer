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

#define INV_PI        0.31830988618379067153776752674503
#define INV_TWO_PI    0.15915494309189533576888376337251

#define TWO_PI_PI    19.739208802178717237668981999752

#define INFINITY  1000000.0
#define EPS 0.0001

#define REFL 0
#define REFR 1
#define SUBS 2

#define QUAD_LIGHT 0
#define SPHERE_LIGHT 1

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
    float specTrans;
    float ior;
    float atDistance;
    vec3 extinction;
    vec3 texIDs;
    // Roughness calculated from anisotropic param
    float ax;
    float ay;
};

struct Camera
{
    vec3 up;
    vec3 right;
    vec3 forward;
    vec3 position;
    float fov;
    float fovTAN;
    float fovTAN1;
    float focalDist;
    float aperture;
};

struct Light
{
    vec3 position;
    vec3 emission;
    vec3 u;
    vec3 v;
	vec3 nrm;
	vec3 uu;
	vec3 vv;
    float radius;
    float area;
    float type;
};

struct State
{
    int depth;
    float eta;
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
    vec3 L;
    vec3 f;
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

float rand()
{
    seed -= randomVector.xy;
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 FaceForward(vec3 a, vec3 b)
{
    return dot(a, b) < 0.0 ? -b : b;
}

uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }

float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }

float rand1()
{
	seed -= randomVector.xy;
	return random(seed);
}