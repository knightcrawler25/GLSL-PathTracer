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

    float cosTheta = sqrt((1.0 - pow(a2, 1.0 - r2)) / (1.0 - a2));
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

vec3 SampleGGXVNDF(vec3 V, float ax, float ay, float r1, float r2)
{
    vec3 Vh = normalize(vec3(ax * V.x, ay * V.y, V.z));

    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = lensq > 0 ? vec3(-Vh.y, Vh.x, 0) * inversesqrt(lensq) : vec3(1, 0, 0);
    vec3 T2 = cross(Vh, T1);

    float r = sqrt(r1);
    float phi = 2.0 * PI * r2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;

    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;

    return normalize(vec3(ax * Nh.x, ay * Nh.y, max(0.0, Nh.z)));
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
    return (2.0 * NDotV) / (NDotV + sqrt(a + b - a * b));
}

float SmithGAniso(float NDotV, float VDotX, float VDotY, float ax, float ay)
{
    float a = VDotX * ax;
    float b = VDotY * ay;
    float c = NDotV;
    return (2.0 * NDotV) / (NDotV + sqrt(a * a + b * b + c * c));
}

float SchlickWeight(float u)
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
    vec3 up = abs(N.z) < 0.9999999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

void SampleSphereLight(in Light light, in vec3 scatterPos, inout LightSampleRec lightSample)
{
    float r1 = rand();
    float r2 = rand();

    vec3 sphereCentertoSurface = scatterPos - light.position;
    float distToSphereCenter = length(sphereCentertoSurface);
    vec3 sampledDir;

    // TODO: Fix this. Currently assumes the light will be hit only from the outside
    sphereCentertoSurface /= distToSphereCenter;
    sampledDir = UniformSampleHemisphere(r1, r2);
    vec3 T, B;
    Onb(sphereCentertoSurface, T, B);
    sampledDir = T * sampledDir.x + B * sampledDir.y + sphereCentertoSurface * sampledDir.z;

    vec3 lightSurfacePos = light.position + sampledDir * light.radius;

    lightSample.direction = lightSurfacePos - scatterPos;
    lightSample.dist = length(lightSample.direction);
    float distSq = lightSample.dist * lightSample.dist;

    lightSample.direction /= lightSample.dist;
    lightSample.normal = normalize(lightSurfacePos - light.position);
    lightSample.emission = light.emission * float(numOfLights);
    lightSample.pdf = distSq / (light.area * 0.5 * abs(dot(lightSample.normal, lightSample.direction)));
}

void SampleRectLight(in Light light, in vec3 scatterPos, inout LightSampleRec lightSample)
{
    float r1 = rand();
    float r2 = rand();

    vec3 lightSurfacePos = light.position + light.u * r1 + light.v * r2;
    lightSample.direction = lightSurfacePos - scatterPos;
    lightSample.dist = length(lightSample.direction);
    float distSq = lightSample.dist * lightSample.dist;
    lightSample.direction /= lightSample.dist;
    lightSample.normal = normalize(cross(light.u, light.v));
    lightSample.emission = light.emission * float(numOfLights);
    lightSample.pdf = distSq / (light.area * abs(dot(lightSample.normal, lightSample.direction)));
}

void SampleDistantLight(in Light light, in vec3 scatterPos, inout LightSampleRec lightSample)
{
    lightSample.direction = normalize(light.position - vec3(0.0));
    lightSample.normal = normalize(scatterPos - light.position);
    lightSample.emission = light.emission * float(numOfLights);
    lightSample.dist = INF;
    lightSample.pdf = 1.0;
}

void SampleOneLight(in Light light, in vec3 scatterPos, inout LightSampleRec lightSample)
{
    int type = int(light.type);

    if (type == QUAD_LIGHT)
        SampleRectLight(light, scatterPos, lightSample);
    else if (type == SPHERE_LIGHT)
        SampleSphereLight(light, scatterPos, lightSample);
    else
        SampleDistantLight(light, scatterPos, lightSample);
}

vec3 SampleHG(vec3 V, float g, float r1, float r2)
{
    float cosTheta;

    if (abs(g) < 0.001)
        cosTheta = 1 - 2 * r2;
    else 
    {
        float sqrTerm = (1 - g * g) / (1 + g - 2 * g * r2);
        cosTheta = -(1 + g * g - sqrTerm * sqrTerm) / (2 * g);
    }

    float phi = r1 * TWO_PI;
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);

    vec3 v1, v2;
    Onb(V, v1, v2);

    return sinTheta * cosPhi * v1 + sinTheta * sinPhi * v2 + cosTheta * V;
}

float PhaseHG(float cosTheta, float g)
{
    float denom = 1 + g * g + 2 * g * cosTheta;
    return INV_4_PI * (1 - g * g) / (denom * sqrt(denom));
}