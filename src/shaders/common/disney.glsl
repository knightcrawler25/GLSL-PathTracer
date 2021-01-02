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

//-----------------------------------------------------------------------
float DisneyPdf(in Ray ray, inout State state, in vec3 bsdfDir)
//-----------------------------------------------------------------------
{
    vec3 N = state.ffnormal;
    vec3 V = -ray.direction;
    vec3 L = bsdfDir;
    vec3 H = normalize(L + V);

    float brdfPdf = 0.0;
    float bsdfPdf = 0.0;

    float NDotH = abs(dot(N, H));

    // TODO: Fix importance sampling for microfacet transmission
    if (dot(N, L) <= 0.0)
        return 1.0;

    float specularAlpha = max(0.001, state.mat.roughness);
    float clearcoatAlpha = mix(0.1, 0.001, state.mat.clearcoatGloss);

    float diffuseRatio = 0.5 * (1.0 - state.mat.metallic);
    float specularRatio = 1.0 - diffuseRatio;

    float aspect = sqrt(1.0 - state.mat.anisotropic * 0.9);
    float ax = max(0.001, state.mat.roughness / aspect);
    float ay = max(0.001, state.mat.roughness * aspect);

    // PDFs for brdf
    float pdfGTR2_aniso = GTR2_aniso(NDotH, dot(H, state.tangent), dot(H, state.bitangent), ax, ay) * NDotH;
    float pdfGTR1 = GTR1(NDotH, clearcoatAlpha) * NDotH;
    float ratio = 1.0 / (1.0 + state.mat.clearcoat);
    float pdfSpec = mix(pdfGTR1, pdfGTR2_aniso, ratio) / (4.0 * abs(dot(L, H)));
    float pdfDiff = abs(dot(L, N)) * (1.0 / PI);
    brdfPdf = diffuseRatio * pdfDiff + specularRatio * pdfSpec;

    // PDFs for bsdf
    float pdfGTR2 = GTR2(NDotH, specularAlpha) * NDotH;
    float F = Fresnel(abs(dot(L, H)), 1.0, state.mat.ior);
    bsdfPdf = pdfGTR2 * F / (4.0 * abs(dot(L, H)));

    return mix(brdfPdf, bsdfPdf, state.mat.transmission);
}

//-----------------------------------------------------------------------
vec3 DisneySample(in Ray ray, inout State state)
//-----------------------------------------------------------------------
{
    vec3 N = state.ffnormal;
    vec3 V = -ray.direction;
    state.specularBounce = false;

    vec3 dir;

    float r1 = rand();
    float r2 = rand();

    // BSDF
    if (rand() < state.mat.transmission)
    {
        float n1 = 1.0;
        float n2 = state.mat.ior;

        vec3 H = ImportanceSampleGGX(state.mat.roughness, r1, r2);
        H = state.tangent * H.x + state.bitangent * H.y + N * H.z;

        float theta = abs(dot(N, V));
        float eta = dot(state.normal, state.ffnormal) > 0.0 ? (n1 / n2) : (n2 / n1);
        float cos2t = 1.0 - eta * eta * (1.0 - theta * theta);

        float F = Fresnel(theta, n1, n2);

        if (cos2t < 0.0 || rand() < F)
            dir = normalize(reflect(-V, H));
        else
        {
            dir = normalize(refract(-V, H, eta));
            state.specularBounce = true;
        }
    }
    // BRDF
    else
    {
        float diffuseRatio = 0.5 * (1.0 - state.mat.metallic);

        if (rand() < diffuseRatio)
        {
            vec3 H = CosineSampleHemisphere(r1, r2);
            H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
            dir = H;
        }
        else
        {
            vec3 H = ImportanceSampleGGX(state.mat.roughness, r1, r2);
            H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
            dir = reflect(-V, H);
        }
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

    vec3 H = normalize(L + V);

    float NDotL = dot(N, L);
    float NDotV = dot(N, V);

    float NDotH = dot(N, H);
    float LDotH = dot(L, H);

    vec3 brdf = vec3(0.0);
    vec3 bsdf = vec3(0.0);

    // TODO: Fix bsdf for microfacet transmission
    if (state.mat.transmission > 0.0)
    {
        vec3 transmittance = vec3(1.0);
        vec3 extinction = log(state.mat.extinction);

        if (dot(state.normal, state.ffnormal) < 0.0)
            transmittance = exp(extinction * state.hitDist);

        if (dot(N, L) <= 0.0)
            bsdf = state.mat.albedo * transmittance / abs(NDotL);
        else
        {
            float a = max(0.001, state.mat.roughness);
            float F = Fresnel(LDotH, 1.0, state.mat.ior);
            float D = GTR2(NDotH, a);
            float G = SmithG_GGX(NDotL, a) * SmithG_GGX(NDotV, a);
            bsdf = state.mat.albedo * F * G * D;
        }
    }

    if (state.mat.transmission < 1.0 && NDotL > 0.0 && NDotV > 0.0)
    {
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
        float aspect = sqrt(1.0 - state.mat.anisotropic * 0.9);
        float ax = max(0.001, state.mat.roughness / aspect);
        float ay = max(0.001, state.mat.roughness * aspect);
        float Ds = GTR2_aniso(NDotH, dot(H, state.tangent), dot(H, state.bitangent), ax, ay);
        float FH = SchlickFresnel(LDotH);
        vec3 Fs = mix(Cspec0, vec3(1.0), FH);
        float Gs = SmithG_GGX_aniso(NDotL, dot(L, state.tangent), dot(L, state.bitangent), ax, ay);
        Gs *= SmithG_GGX_aniso(NDotV, dot(V, state.tangent), dot(V, state.bitangent), ax, ay);

        // sheen
        vec3 Fsheen = FH * state.mat.sheen * Csheen;

        // clearcoat (ior = 1.5 -> F0 = 0.04)
        float Dr = GTR1(NDotH, mix(0.1, 0.001, state.mat.clearcoatGloss));
        float Fr = mix(0.04, 1.0, FH);
        float Gr = SmithG_GGX(NDotL, 0.25) * SmithG_GGX(NDotV, 0.25);

        brdf = ((1.0 / PI) * mix(Fd, ss, state.mat.subsurface) * Cdlin + Fsheen) * (1.0 - state.mat.metallic) + Gs * Fs * Ds + 0.25 * state.mat.clearcoat * Gr * Fr * Dr;
    }

    return mix(brdf, bsdf, state.mat.transmission);
}