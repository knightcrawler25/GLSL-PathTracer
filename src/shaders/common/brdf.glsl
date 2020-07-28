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
float UE4Pdf(in Ray ray, inout State state, in vec3 bsdfDir)
//-----------------------------------------------------------------------
{
    vec3 n = state.ffnormal;
    vec3 V = -ray.direction;
    vec3 L = bsdfDir;

    float specularAlpha = max(0.001, state.mat.param.y);

    float diffuseRatio = 0.5 * (1.0 - state.mat.param.x);
    float specularRatio = 1.0 - diffuseRatio;

    vec3 halfVec = normalize(L + V);

    float cosTheta = abs(dot(halfVec, n));
    float pdfGTR2 = GTR2(cosTheta, specularAlpha) * cosTheta;

    // calculate diffuse and specular pdfs and mix ratio
    float pdfSpec = pdfGTR2 / (4.0 * abs(dot(L, halfVec)));
    float pdfDiff = abs(dot(L, n)) * (1.0 / PI);

    // weight pdfs according to ratios
    return diffuseRatio * pdfDiff + specularRatio * pdfSpec;
}

//-----------------------------------------------------------------------
vec3 UE4Sample(in Ray ray, inout State state)
//-----------------------------------------------------------------------
{
    vec3 N = state.ffnormal;
    vec3 V = -ray.direction;

    vec3 dir;

    float probability = rand();
    float diffuseRatio = 0.5 * (1.0 - state.mat.param.x);

    float r1 = rand();
    float r2 = rand();

    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 TangentX = normalize(cross(UpVector, N));
    vec3 TangentY = cross(N, TangentX);

    if (probability < diffuseRatio) // sample diffuse
    {
        dir = CosineSampleHemisphere(r1, r2);
        dir = TangentX * dir.x + TangentY * dir.y + N * dir.z;
    }
    else
    {
        float a = max(0.001, state.mat.param.y);

        float phi = r1 * 2.0 * PI;

        float cosTheta = sqrt((1.0 - r2) / (1.0 + (a*a - 1.0) *r2));
        float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

        vec3 halfVec = vec3(sinTheta*cosPhi, sinTheta*sinPhi, cosTheta);
        halfVec = TangentX * halfVec.x + TangentY * halfVec.y + N * halfVec.z;

        dir = 2.0*dot(V, halfVec)*halfVec - V;
    }
    return dir;
}

//-----------------------------------------------------------------------
vec3 UE4Eval(in Ray ray, inout State state, in vec3 bsdfDir)
//-----------------------------------------------------------------------
{
    vec3 N = state.ffnormal;
    vec3 V = -ray.direction;
    vec3 L = bsdfDir;

    float NDotL = dot(N, L);
    float NDotV = dot(N, V);

    if (NDotL <= 0.0 || NDotV <= 0.0)
        return vec3(0.0);

    vec3 H = normalize(L + V);
    float NDotH = dot(N, H);
    float LDotH = dot(L, H);

    // specular    
    float specular = 0.5;
    vec3 specularCol = mix(vec3(1.0) * 0.08 * specular, state.mat.albedo.xyz, state.mat.param.x);
    float a = max(0.001, state.mat.param.y);
    float Ds = GTR2(NDotH, a);
    float FH = SchlickFresnel(LDotH);
    vec3 Fs = mix(specularCol, vec3(1.0), FH);
    float roughg = (state.mat.param.y*0.5 + 0.5);
    roughg = roughg * roughg;
    float Gs = SmithG_GGX(NDotL, roughg) * SmithG_GGX(NDotV, roughg);

    return (state.mat.albedo.xyz / PI) * (1.0 - state.mat.param.x) + Gs * Fs*Ds;
}