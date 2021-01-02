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

//----------------------------------------------------------------------
vec3 ImportanceSampleGGX(float rgh, float r1, float r2)
//----------------------------------------------------------------------
{
    float a = max(0.001, rgh);

    float phi = r1 * 2.0 * PI;

    float cosTheta = sqrt((1.0 - r2) / (1.0 + (a * a - 1.0) * r2));
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);

    return vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

//-----------------------------------------------------------------------
float SchlickFresnel(float u)
//-----------------------------------------------------------------------
{
    float m = clamp(1.0 - u, 0.0, 1.0);
    float m2 = m * m;
    return m2 * m2*m; // pow(m,5)
}

//-----------------------------------------------------------------------
float Fresnel(float theta, float n1, float n2)
//-----------------------------------------------------------------------
{
    float R0 = (n1 - n2) / (n1 + n2);
    R0 *= R0;
    return R0 + (1.0 - R0) * SchlickFresnel(theta);
}

//-----------------------------------------------------------------------
float GTR1(float NDotH, float a)
//-----------------------------------------------------------------------
{
    if (a >= 1.0) 
        return (1.0 / PI);
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NDotH * NDotH;
    return (a2 - 1.0) / (PI * log(a2) * t);
}

//-----------------------------------------------------------------------
float GTR2(float NDotH, float a)
//-----------------------------------------------------------------------
{
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0)*NDotH*NDotH;
    return a2 / (PI * t*t);
}

//-----------------------------------------------------------------------
float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
//-----------------------------------------------------------------------
{
    float a = HdotX / ax;
    float b = HdotY / ay;
    float c = a * a + b * b + NdotH * NdotH;
    return 1.0 / (PI * ax * ay * c * c);
}

//-----------------------------------------------------------------------
float SmithG_GGX(float NDotv, float alphaG)
//-----------------------------------------------------------------------
{
    float a = alphaG * alphaG;
    float b = NDotv * NDotv;
    return 1.0 / (NDotv + sqrt(a + b - a * b));
}

//-----------------------------------------------------------------------
float SmithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax, float ay)
//-----------------------------------------------------------------------
{
    float a = VdotX * ax;
    float b = VdotY * ay;
    float c = NdotV;
    return 1.0 / (NdotV + sqrt(a*a + b*b + c*c));
}

//-----------------------------------------------------------------------
vec3 CosineSampleHemisphere(float u1, float u2)
//-----------------------------------------------------------------------
{
    vec3 dir;
    float r = sqrt(u1);
    float phi = 2.0 * PI * u2;
    dir.x = r * cos(phi);
    dir.y = r * sin(phi);
    dir.z = sqrt(max(0.0, 1.0 - dir.x*dir.x - dir.y*dir.y));

    return dir;
}

//-----------------------------------------------------------------------
vec3 UniformSampleSphere(float u1, float u2)
//-----------------------------------------------------------------------
{
    float z = 1.0 - 2.0 * u1;
    float r = sqrt(max(0.f, 1.0 - z * z));
    float phi = 2.0 * PI * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);

    return vec3(x, y, z);
}

//-----------------------------------------------------------------------
float powerHeuristic(float a, float b)
//-----------------------------------------------------------------------
{
    float t = a * a;
    return t / (b*b + t);
}

//-----------------------------------------------------------------------
void sampleSphereLight(in Light light, inout LightSampleRec lightSampleRec)
//-----------------------------------------------------------------------
{
    float r1 = rand();
    float r2 = rand();

    lightSampleRec.surfacePos = light.position + UniformSampleSphere(r1, r2) * light.radiusAreaType.x;
    lightSampleRec.normal = normalize(lightSampleRec.surfacePos - light.position);
    lightSampleRec.emission = light.emission * float(numOfLights);
}

//-----------------------------------------------------------------------
void sampleQuadLight(in Light light, inout LightSampleRec lightSampleRec)
//-----------------------------------------------------------------------
{
    float r1 = rand();
    float r2 = rand();

    lightSampleRec.surfacePos = light.position + light.u * r1 + light.v * r2;
    lightSampleRec.normal = normalize(cross(light.u, light.v));
    lightSampleRec.emission = light.emission * float(numOfLights);
}

//-----------------------------------------------------------------------
void sampleLight(in Light light, inout LightSampleRec lightSampleRec)
//-----------------------------------------------------------------------
{
    if (int(light.radiusAreaType.z) == 0) // Quad Light
        sampleQuadLight(light, lightSampleRec);
    else
        sampleSphereLight(light, lightSampleRec);
}

#ifdef ENVMAP
#ifndef CONSTANT_BG

//-----------------------------------------------------------------------
float EnvPdf(in Ray r)
//-----------------------------------------------------------------------
{
    float theta = acos(clamp(r.direction.y, -1.0, 1.0));
    vec2 uv = vec2((PI + atan(r.direction.z, r.direction.x)) * (1.0 / TWO_PI), theta * (1.0 / PI));
    float pdf = texture(hdrCondDistTex, uv).y * texture(hdrMarginalDistTex, vec2(uv.y, 0.)).y;
    return (pdf * hdrResolution) / (2.0 * PI * PI * sin(theta));
}

//-----------------------------------------------------------------------
vec4 EnvSample(inout vec3 color)
//-----------------------------------------------------------------------
{
    float r1 = rand();
    float r2 = rand();

    float v = texture(hdrMarginalDistTex, vec2(r1, 0.)).x;
    float u = texture(hdrCondDistTex, vec2(r2, v)).x;

    color = texture(hdrTex, vec2(u, v)).xyz * hdrMultiplier;
    float pdf = texture(hdrCondDistTex, vec2(u, v)).y * texture(hdrMarginalDistTex, vec2(v, 0.)).y;

    float phi = u * TWO_PI;
    float theta = v * PI;

    if (sin(theta) == 0.0)
        pdf = 0.0;

    return vec4(-sin(theta) * cos(phi), cos(theta), -sin(theta)*sin(phi), (pdf * hdrResolution) / (2.0 * PI * PI * sin(theta)));
}

#endif
#endif

//-----------------------------------------------------------------------
vec3 EmitterSample(in Ray r, in State state, in LightSampleRec lightSampleRec, in BsdfSampleRec bsdfSampleRec)
//-----------------------------------------------------------------------
{
    vec3 Le;

    if (state.depth == 0 || state.specularBounce)
        Le = lightSampleRec.emission;
    else
        Le = powerHeuristic(bsdfSampleRec.pdf, lightSampleRec.pdf) * lightSampleRec.emission;

    return Le;
}