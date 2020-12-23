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

//-----------------------------------------------------------------------
float GlassPdf(Ray ray, inout State state, in vec3 bsdfDir)
//-----------------------------------------------------------------------
{
    vec3 N = state.ffnormal;
    vec3 V = -ray.direction;
    vec3 L = bsdfDir;

    if (dot(N,L) <= 0.0)
    {
        L = -L;
    }

    vec3 H = normalize(L + V);

    float NDotH = abs(dot(N, H));
    float VDotH = abs(dot(V, H));

    float a = max(0.001, state.mat.roughness);
    float pdfGTR2 = GTR2(NDotH, a) * NDotH;

    float F = Fresnel(abs(dot(L, H)), 1.0, 1.45);

    float n1 = 1.0;
    float n2 = 1.45;
    float eta = dot(state.normal, N) > 0.0 ? (n1 / n2) : (n2 / n1);

    // Transmission Vector
    if (dot(N, L) < 0.0)
    {
        float denom = n1 * dot(V, H) + n2 * dot(L, H);
        return pdfGTR2 * (1.0 - F) * abs(dot(L, H)) / (denom * denom);
        //return 1.0;
    }
    else
        return pdfGTR2 * F / (4.0 * abs(dot(L, H)));

}

//-----------------------------------------------------------------------
vec3 GlassSample(in Ray ray, inout State state)
//-----------------------------------------------------------------------
{
    vec3 N = state.ffnormal;
    vec3 V = -ray.direction;

    float r1 = rand();
    float r2 = rand();

    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 TangentX = normalize(cross(UpVector, N));
    vec3 TangentY = cross(N, TangentX);

    float a = max(0.001, state.mat.roughness);

    float phi = r1 * 2.0 * PI;

    float cosTheta = sqrt((1.0 - r2) / (1.0 + (a * a - 1.0) * r2));
    float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);

    vec3 H = vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
    H = TangentX * H.x + TangentY * H.y + N * H.z;

    float n1 = 1.0;
    float n2 = 1.45;

    float theta = abs(dot(-V, N));
    float eta = dot(state.normal, N) > 0.0 ? (n1 / n2) : (n2 / n1);
    float cos2t = 1.0 - eta * eta * (1.0 - theta * theta);

    float F = Fresnel(theta, n1, n2);

    vec3 dir;
    if (cos2t < 0.0 || rand() < F)
       dir = normalize(reflect(-V, H));
    else
        dir = normalize(refract(-V, H, eta));

    return dir;
}

//-----------------------------------------------------------------------
vec3 GlassEval(in Ray ray, inout State state, in vec3 bsdfDir)
//-----------------------------------------------------------------------
{
    vec3 N = state.ffnormal;
    vec3 V = -ray.direction;
    vec3 L = bsdfDir;

    float NDotL = dot(N, L);
    float NDotV = dot(N, V);

    if (NDotL <= 0.0)
    {
        L = -L;
    }
    NDotL = dot(N, L);

    vec3 H = normalize(L + V);

    //if (NDotL <= 0.0 || NDotV <= 0.0)
    //    return vec3(0.0);
   
    float NDotH = abs(dot(N, H));
    float LDotH = abs(dot(L, H));
    float VDotH = abs(dot(V, H));

    float n1 = 1.0;
    float n2 = 1.45;
    float a = max(0.001, state.mat.roughness);
    float D = GTR2(NDotH, a);
    float F = Fresnel(LDotH, n1, n2);
    float G = SmithG_GGX(NDotL, a) * SmithG_GGX(NDotV, a);

    if (dot(N, L) < 0.0)
    {
        float denom = n1 * dot(V, H) + n2 * dot(L, H);
        return 4.0 * state.mat.albedo * (LDotH * VDotH) * (1.0 - F) * D * G / (denom * denom);
        //return vec3(1.0);
    }
    else
        return state.mat.albedo * F * D * G;
}