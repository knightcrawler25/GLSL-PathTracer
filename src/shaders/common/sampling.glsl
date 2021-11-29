/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
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

float GTR1(float NDotH, float a)
{
    if (a >= 1.0)
        return INV_PI;
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NDotH * NDotH;
    return (a2 - 1.0) / (PI * log(a2) * t);
}

vec3 SampleGTR1(float rgh, float r1, float r2)
{
    float a = max(0.001, rgh);
    float a2 = a * a;

    float phi = r1 * TWO_PI;

    float cosTheta = sqrt((1.0 - pow(a2, 1.0 - r1)) / (1.0 - a2));
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);

    return vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

float GTR2(float NDotH, float a)
{
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NDotH * NDotH;
    return a2 / (PI * t * t);
}

vec3 SampleGTR2(float rgh, float r1, float r2)
{
    float a = max(0.001, rgh);

    float phi = r1 * TWO_PI;

    float cosTheta = sqrt((1.0 - r2) / (1.0 + (a * a - 1.0) * r2));
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);

    return vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

float GTR2Aniso(float NDotH, float HDotX, float HDotY, float ax, float ay)
{
    float a = HDotX / ax;
    float b = HDotY / ay;
    float c = a * a + b * b + NDotH * NDotH;
    return 1.0 / (PI * ax * ay * c * c);
}

vec3 SampleGTR2Aniso(float ax, float ay, float r1, float r2)
{
    float phi = r1 * TWO_PI;

    float sinPhi = ay * sin(phi);
    float cosPhi = ax * cos(phi);
    float tanTheta = sqrt(r2 / (1 - r2));

    return vec3(tanTheta * cosPhi, tanTheta * sinPhi, 1.0);
}

float SmithG(float NDotV, float alphaG)
{
    float a = alphaG * alphaG;
    float b = NDotV * NDotV;
    return 1.0 / (NDotV + sqrt(a + b - a * b));
}

float SmithGAniso(float NDotV, float VDotX, float VDotY, float ax, float ay)
{
    float a = VDotX * ax;
    float b = VDotY * ay;
    float c = NDotV;
    return 1.0 / (NDotV + sqrt(a * a + b * b + c * c));
}

float SchlickFresnel(float u)
{
    float m = clamp(1.0 - u, 0.0, 1.0);
    float m2 = m * m;
    return m2 * m2 * m;
}

float DielectricFresnel(float cosThetaI, float eta)
{
    float sinThetaTSq = eta * eta * (1.0f - cosThetaI * cosThetaI);

    // Total internal reflection
    if (sinThetaTSq > 1.0)
        return 1.0;

    float cosThetaT = sqrt(max(1.0 - sinThetaTSq, 0.0));

    float rs = (eta * cosThetaT - cosThetaI) / (eta * cosThetaT + cosThetaI);
    float rp = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);

    return 0.5f * (rs * rs + rp * rp);
}

vec3 CosineSampleHemisphere(float r1, float r2)
{
    vec3 dir;
    float r = sqrt(r1);
    float phi = TWO_PI * r2;
    dir.x = r * cos(phi);
    dir.y = r * sin(phi);
    dir.z = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.y * dir.y));
    return dir;
}

vec3 UniformSampleHemisphere(float r1, float r2)
{
    float r = sqrt(max(0.0, 1.0 - r1 * r1));
    float phi = TWO_PI * r2;
    return vec3(r * cos(phi), r * sin(phi), r1);
}

vec3 UniformSampleSphere(float r1, float r2)
{
    float z = 1.0 - 2.0 * r1;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float phi = TWO_PI * r2;
    return vec3(r * cos(phi), r * sin(phi), z);
}

float PowerHeuristic(float a, float b)
{
    float t = a * a;
    return t / (b * b + t);
}

void Onb(in vec3 N, inout vec3 T, inout vec3 B)
{
    vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

void SampleSphereLight(in Light light, in vec3 surfacePos, inout LightSampleRec lightSampleRec)
{
    float r1 = rand();
    float r2 = rand();

    vec3 sphereCentertoSurface = surfacePos - light.position;
    float distToSphereCenter = length(sphereCentertoSurface);
    vec3 sampledDir;

    // TODO: Fix this. Currently assumes the light will be hit only from the outside
    sphereCentertoSurface /= distToSphereCenter;
    sampledDir = UniformSampleHemisphere(r1, r2);
    vec3 T, B;
    Onb(sphereCentertoSurface, T, B);
    sampledDir = T * sampledDir.x + B * sampledDir.y + sphereCentertoSurface * sampledDir.z;

    vec3 lightSurfacePos = light.position + sampledDir * light.radius;

    lightSampleRec.direction = lightSurfacePos - surfacePos;
    lightSampleRec.dist = length(lightSampleRec.direction);
    float distSq = lightSampleRec.dist * lightSampleRec.dist;

    lightSampleRec.direction /= lightSampleRec.dist;
    lightSampleRec.normal = normalize(lightSurfacePos - light.position);
    lightSampleRec.emission = light.emission * float(numOfLights);
    lightSampleRec.pdf = distSq / (light.area * 0.5 * abs(dot(lightSampleRec.normal, lightSampleRec.direction)));
}

void SampleRectLight(in Light light, in vec3 surfacePos, inout LightSampleRec lightSampleRec)
{
    float r1 = rand();
    float r2 = rand();

    vec3 lightSurfacePos = light.position + light.u * r1 + light.v * r2;
    lightSampleRec.direction = lightSurfacePos - surfacePos;
    lightSampleRec.dist = length(lightSampleRec.direction);
    float distSq = lightSampleRec.dist * lightSampleRec.dist;
    lightSampleRec.direction /= lightSampleRec.dist;
    lightSampleRec.normal = normalize(cross(light.u, light.v));
    lightSampleRec.emission = light.emission * float(numOfLights);
    lightSampleRec.pdf = distSq / (light.area * abs(dot(lightSampleRec.normal, lightSampleRec.direction)));
}

void SampleDistantLight(in Light light, in vec3 surfacePos, inout LightSampleRec lightSampleRec)
{
    lightSampleRec.direction = normalize(light.position - vec3(0.0));
    lightSampleRec.normal = normalize(surfacePos - light.position);
    lightSampleRec.emission = light.emission * float(numOfLights);
    lightSampleRec.dist = INF;
    lightSampleRec.pdf = 1.0;
}

void SampleOneLight(in Light light, in vec3 surfacePos, inout LightSampleRec lightSampleRec)
{
    int type = int(light.type);

    if (type == QUAD_LIGHT)
        SampleRectLight(light, surfacePos, lightSampleRec);
    else if (type == SPHERE_LIGHT)
        SampleSphereLight(light, surfacePos, lightSampleRec);
    else
        SampleDistantLight(light, surfacePos, lightSampleRec);
}

#ifdef ENVMAP
#ifndef CONSTANT_BG

float EnvMapPdf(in Ray r)
{
    float theta = acos(clamp(r.direction.y, -1.0, 1.0));
    vec2 uv = vec2((PI + atan(r.direction.z, r.direction.x)) * INV_TWO_PI, theta * INV_PI);
    float pdf = texture(hdrCondDistTex, uv).y * texture(hdrMarginalDistTex, vec2(uv.y, 0.0)).y;
    return (pdf * hdrResolution) / (2.0 * PI * PI * sin(theta));
}

vec4 SampleEnvMap(inout vec3 color)
{
    float r1 = rand();
    float r2 = rand();

    float v = texture(hdrMarginalDistTex, vec2(r1, 0.0)).x;
    float u = texture(hdrCondDistTex, vec2(r2, v)).x;

    color = texture(hdrTex, vec2(u, v)).xyz * hdrMultiplier;
    float pdf = texture(hdrCondDistTex, vec2(u, v)).y * texture(hdrMarginalDistTex, vec2(v, 0.0)).y;

    float phi = u * TWO_PI;
    float theta = v * PI;

    if (sin(theta) == 0.0)
        pdf = 0.0;

    return vec4(-sin(theta) * cos(phi), cos(theta), -sin(theta) * sin(phi), (pdf * hdrResolution) / (2.0 * PI * PI * sin(theta)));
}

#endif
#endif

vec3 EmitterSample(in Ray r, in State state, in LightSampleRec lightSampleRec, in BsdfSampleRec bsdfSampleRec)
{
    vec3 Le;

    if (state.depth == 0)
        Le = lightSampleRec.emission;
    else
        Le = PowerHeuristic(bsdfSampleRec.pdf, lightSampleRec.pdf) * lightSampleRec.emission;

    return Le;
}