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
float DisneyPdf(in Ray ray, inout State state, in vec3 bsdfDir)
//-----------------------------------------------------------------------
{
    vec3 N = state.ffnormal;
    vec3 V = -ray.direction;
    vec3 L = bsdfDir;
    vec3 H = normalize(L + V);

    float NDotH = dot(N, H);

    float clearcoatAlpha = mix(0.1, 0.001, state.mat.clearcoatGloss);

    float diffuseRatio = 0.5 * (1.0 - state.mat.metallic);
    float specularRatio = 1.0 - diffuseRatio;

    float cosTheta = abs(dot(H, N));

    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 X = normalize(cross(UpVector, N));
    vec3 Y = cross(N, X);

    float aspect = sqrt(1.0 - state.mat.anisotropic * 0.9);
    float ax = max(0.001, (state.mat.roughness * state.mat.roughness) / aspect);
    float ay = max(0.001, (state.mat.roughness * state.mat.roughness) * aspect);
    float pdfGTR2 = GTR2_aniso(NDotH, dot(H, X), dot(H, Y), ax, ay) * cosTheta;
    float pdfGTR1 = GTR1(cosTheta, clearcoatAlpha) * cosTheta;

    // calculate diffuse and specular pdfs and mix ratio
    float ratio = 1.0 / (1.0 + state.mat.clearcoat);
    float pdfSpec = mix(pdfGTR1, pdfGTR2, ratio) / (4.0 * abs(dot(L, H)));
    float pdfDiff = abs(dot(L, N)) * (1.0 / PI);

    // weigh pdfs according to ratios
    return diffuseRatio * pdfDiff + specularRatio * pdfSpec;
}

//-----------------------------------------------------------------------
vec3 DisneySample(in Ray ray, inout State state)
//-----------------------------------------------------------------------
{
    vec3 N = state.ffnormal;
    vec3 V = -ray.direction;

    vec3 dir;

    float probability = rand();
    float diffuseRatio = 0.5 * (1.0 - state.mat.metallic);

    float r1 = rand();
    float r2 = rand();

    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 TangentX = normalize(cross(UpVector, N));
    vec3 TangentY = cross(N, TangentX);

    if (probability < diffuseRatio) // sample diffuse
    {
        dir = CosineSampleHemisphere(r1, r2);
        dir = TangentX * dir.x + TangentY * dir.y + N * dir.z;
    }
    else
    {
        float a = max(0.001, state.mat.roughness);

        float phi = r1 * 2.0 * PI;

        float cosTheta = sqrt((1.0 - r2) / (1.0 + (a*a - 1.0) *r2));
        float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

        vec3 H = vec3(sinTheta*cosPhi, sinTheta*sinPhi, cosTheta);
        H = TangentX * H.x + TangentY * H.y + N * H.z;

        dir = 2.0*dot(V, H)*H - V;
    }
    return dir;
}

//-----------------------------------------------------------------------
vec3 DisneyEval(in Ray ray, inout State state, in vec3 bsdfDir)
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

    vec3 Cdlin = state.mat.albedo;
    float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

    vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0f); // normalize lum. to isolate hue+sat
    vec3 Cspec0 = mix(state.mat.specular * 0.08 * mix(vec3(1.0), Ctint, state.mat.specularTint), Cdlin, state.mat.metallic);
    vec3 Csheen = mix(vec3(1.0), Ctint, state.mat.sheenTint);

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
    // and mix in diffuse retro-reflection based on roughness
    float FL = SchlickFresnel(NDotL);
    float FV = SchlickFresnel(NDotV);
    float Fd90 = 0.5 + 2.0 * LDotH * LDotH * state.mat.roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
    // 1.25 scale is used to (roughly) preserve albedo
    // Fss90 used to "flatten" retroreflection based on roughness
    float Fss90 = LDotH * LDotH * state.mat.roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (NDotL + NDotV) - 0.5) + 0.5);

    // specular
    /*float a = max(0.001f, state.mat.roughness);
    float Ds = GTR2(NDotH, a);
    float FH = SchlickFresnel(LDotH);
    vec3 Fs = mix(Cspec0, vec3(1.0), FH);
    float roughg = (state.mat.roughness * 0.5 + 0.5);
    roughg *= roughg;
    float Gs = SmithG_GGX(NDotL, roughg) * SmithG_GGX(NDotV, roughg);*/

    // specular
    vec3 UpVector = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 X = normalize(cross(UpVector, N));
    vec3 Y = cross(N, X);

    float aspect = sqrt(1.0 - state.mat.anisotropic * 0.9);
    float ax = max(0.001, (state.mat.roughness * state.mat.roughness) / aspect);
    float ay = max(0.001, (state.mat.roughness * state.mat.roughness) * aspect);
    float Ds = GTR2_aniso(NDotH, dot(H, X), dot(H, Y), ax, ay);
    float FH = SchlickFresnel(LDotH);
    vec3 Fs = mix(Cspec0, vec3(1.0), FH);
    float Gs = SmithG_GGX_aniso(NDotL, dot(L, X), dot(L, Y), ax, ay);
    Gs *= SmithG_GGX_aniso(NDotV, dot(V, X), dot(V, Y), ax, ay);

    // sheen
    vec3 Fsheen = FH * state.mat.sheen * Csheen;

    // clearcoat (ior = 1.5 -> F0 = 0.04)
    float Dr = GTR1(NDotH, mix(0.1, 0.001, state.mat.clearcoatGloss));
    float Fr = mix(0.04, 1.0, FH);
    float Gr = SmithG_GGX(NDotL, 0.25) * SmithG_GGX(NDotV, 0.25);

    return ((1.0 / PI) * mix(Fd, ss, state.mat.subsurface) * Cdlin + Fsheen) * (1.0 - state.mat.metallic) + Gs * Fs * Ds + 0.25 * state.mat.clearcoat * Gr * Fr * Dr;
}