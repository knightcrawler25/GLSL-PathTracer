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

 /* References:
 * [1] https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf
 * [2] https://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
 * [3] https://github.com/wdas/brdf/blob/main/src/brdfs/disney.brdf
 * [4] https://github.com/mmacklin/tinsel/blob/master/src/disney.h
 * [5] http://simon-kallweit.me/rendercompo2015/report/
 * [6] http://shihchinw.github.io/2015/07/implementing-disney-principled-brdf-in-arnold.html
 * [7] https://github.com/mmp/pbrt-v4/blob/0ec29d1ec8754bddd9d667f0e80c4ff025c900ce/src/pbrt/bxdfs.cpp#L76-L286
 * [8] https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
 */

vec3 ToWorld(vec3 X, vec3 Y, vec3 Z, vec3 V)
{
    return V.x * X + V.y * Y + V.z * Z;
}

vec3 ToLocal(vec3 X, vec3 Y, vec3 Z, vec3 V)
{
    return vec3(dot(V, X), dot(V, Y), dot(V, Z));
}

float FresnelMix(Material mat, float eta, vec3 V, vec3 L, vec3 H)
{
    float metallicFresnel = SchlickFresnel(abs(dot(L, H)));
    float dielectricFresnel = DielectricFresnel(abs(dot(V, H)), eta);
    return mix(dielectricFresnel, metallicFresnel, mat.metallic);
}

vec3 EvalDiffuse(Material mat, vec3 Csheen, vec3 V, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (L.z <= 0.0)
        return vec3(0.0);

    // Diffuse
    float FL = SchlickFresnel(L.z);
    float FV = SchlickFresnel(V.z);
    float FH = SchlickFresnel(dot(L, H));
    float Fd90 = 0.5 + 2.0 * dot(L, H) * dot(L, H) * mat.roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // Fake Subsurface TODO: Replace with volumetric scattering
    float Fss90 = dot(L, H) * dot(L, H) * mat.roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (L.z + V.z) - 0.5) + 0.5);

    // Sheen
    vec3 Fsheen = FH * mat.sheen * Csheen;

    pdf = L.z * INV_PI;
    return (INV_PI * mix(Fd, ss, mat.subsurface) * mat.baseColor + Fsheen) * (1.0 - mat.metallic) * (1.0 - mat.specTrans);
}

vec3 EvalSpecReflection(Material mat, float eta, vec3 specCol, vec3 V, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (L.z <= 0.0)
        return vec3(0.0);

    float FM = FresnelMix(mat, eta, V, L, H);
    vec3 F = mix(specCol, vec3(1.0), FM);
    float D = GTR2(H.z, mat.roughness);
    float G = SmithG(abs(L.z), mat.roughness)
            * SmithG(abs(V.z), mat.roughness);

    pdf = D * H.z / (4.0 * abs(dot(V, H)));
    return F * D * G;
}

vec3 EvalSpecRefraction(Material mat, float eta, vec3 V, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (L.z >= 0.0)
        return vec3(0.0);

    float F = DielectricFresnel(abs(dot(V, H)), eta);
    float D = GTR2(H.z, mat.roughness);
    float denomSqrt = dot(L, H) + dot(V, H) * eta;
    float G = SmithG(abs(L.z), mat.roughness) 
            * SmithG(abs(V.z), mat.roughness);

    pdf = D * H.z * abs(dot(L, H)) / (denomSqrt * denomSqrt);
    vec3 specColor = pow(mat.baseColor, vec3(0.5));
    return specColor * (1.0 - mat.metallic) * mat.specTrans * (1.0 - F) * D * G * abs(dot(V, H)) * abs(dot(L, H)) * eta * eta * 4.0 / (denomSqrt * denomSqrt);
}

vec3 EvalClearcoat(Material mat, vec3 V, vec3 L, vec3 H, inout float pdf)
{
    pdf = 0.0;
    if (L.z <= 0.0)
        return vec3(0.0);

    float FH = DielectricFresnel(dot(V, H), 1.0 / 1.5);
    float F = mix(0.04, 1.0, FH);
    float D = GTR1(H.z, mat.clearcoatRoughness);
    float G = SmithG(L.z, 0.25) 
            * SmithG(V.z, 0.25);
    pdf = D * H.z / (4.0 * dot(V, H));
    return vec3(0.25 * mat.clearcoat * F * D * G);
}

void GetSpecColor(Material mat, float eta, inout vec3 specCol, inout vec3 sheenCol)
{
    float lum = Luminance(mat.baseColor);
    vec3 ctint = lum > 0.0 ? mat.baseColor / lum : vec3(1.0f);
    float F0 = (1.0 - eta) / (1.0 + eta);
    specCol = mix(F0 * F0 * mix(vec3(1.0), ctint, mat.specularTint), mat.baseColor, mat.metallic);
    sheenCol = mix(vec3(1.0), ctint, mat.sheenTint);
}

void GetLobeProbabilities(Material mat, float eta, vec3 specCol, float approxFresnel, inout float diffuseWt, inout float specReflectWt, inout float specRefractWt, inout float clearcoatWt)
{
    diffuseWt = Luminance(mat.baseColor) * (1.0 - mat.metallic) * (1.0 - mat.specTrans);
    specReflectWt = Luminance(mix(specCol, vec3(1.0), approxFresnel));
    specRefractWt = (1.0 - approxFresnel) * (1.0 - mat.metallic) * mat.specTrans * Luminance(mat.baseColor);
    clearcoatWt = mat.clearcoat * (1.0 - mat.metallic);
    float totalWt = diffuseWt + specReflectWt + specRefractWt + clearcoatWt;

    diffuseWt /= totalWt;
    specReflectWt /= totalWt;
    specRefractWt /= totalWt;
    clearcoatWt /= totalWt;
}

vec3 DisneySample(State state, vec3 V, vec3 N, inout vec3 L, inout float pdf)
{
    pdf = 0.0;
    vec3 f = vec3(0.0);

    float r1 = rand();
    float r2 = rand();

    // Specular and sheen color
    vec3 specCol, sheenCol;
    GetSpecColor(state.mat, state.eta, specCol, sheenCol);

    // Lobe weights
    float diffuseWt, specReflectWt, specRefractWt, clearcoatWt;
    float approxFresnel = FresnelMix(state.mat, state.eta, V, reflect(V, N), N);
    GetLobeProbabilities(state.mat, state.eta, specCol, approxFresnel, diffuseWt, specReflectWt, specRefractWt, clearcoatWt);

    // CDF for picking a lobe
    float cdf[4];
    cdf[0] = diffuseWt;
    cdf[1] = cdf[0] + specReflectWt;
    cdf[2] = cdf[1] + specRefractWt;
    cdf[3] = cdf[2] + clearcoatWt;

    vec3 T, B;
    Onb(N, T, B);
    // NDotL = L.z; NDotV = V.z; NDotH = H.z
    V = ToLocal(T, B, N, V);

    if(r1 < cdf[0]) // Diffuse Reflection Lobe
    {
        r1 /= cdf[0];
        L = CosineSampleHemisphere(r1, r2);

        vec3 H = normalize(L + V);

        f = EvalDiffuse(state.mat, sheenCol, V, L, H, pdf);
        pdf *= diffuseWt;
    }
    else if (r1 < cdf[1]) // Specular Reflection Lobe
    {
        r1 = (r1 - cdf[0]) / (cdf[1] - cdf[0]);
        vec3 H = SampleGTR2(state.mat.roughness, r1, r2);

        if (dot(V, H) < 0.0)
            H = -H;

        L = normalize(reflect(-V, H));

        f = EvalSpecReflection(state.mat, state.eta, specCol, V, L, H, pdf);
        pdf *= specReflectWt;
    }
    else if (r1 < cdf[2]) // Specular Refraction Lobe
    {
        r1 = (r1 - cdf[1]) / (cdf[2] - cdf[1]);
        vec3 H = SampleGTR2(state.mat.roughness, r1, r2);

        if (dot(V, H) < 0.0)
            H = -H;

        L = normalize(refract(-V, H, state.eta));

        f = EvalSpecRefraction(state.mat, state.eta, V, L, H, pdf);
        pdf *= specRefractWt;
    }
    else // Clearcoat Lobe
    {
        r1 = (r1 - cdf[2]) / (1.0 - cdf[2]);
        vec3 H = SampleGTR1(state.mat.clearcoatRoughness, r1, r2);

        if (dot(V, H) < 0.0)
            H = -H;

        L = normalize(reflect(-V, H));

        f = EvalClearcoat(state.mat, V, L, H, pdf);
        pdf *= clearcoatWt;
    }

    L = ToWorld(T, B, N, L);
    return f * abs(dot(N, L));
}

vec3 DisneyEval(State state, vec3 V, vec3 N, vec3 L, inout float bsdfPdf)
{
    bsdfPdf = 0.0;
    vec3 f = vec3(0.0);

    vec3 T, B;
    Onb(N, T, B);
    // NDotL = L.z; NDotV = V.z; NDotH = H.z
    V = ToLocal(T, B, N, V);
    L = ToLocal(T, B, N, L);

    vec3 H;
    if (L.z > 0.0)
        H = normalize(L + V);
    else
        H = normalize(L + V * state.eta);

    if (dot(V, H) < 0.0)
        H = -H;

    // Specular and sheen color
    vec3 specCol, sheenCol;
    GetSpecColor(state.mat, state.eta, specCol, sheenCol);

    // Lobe weights
    float diffuseWt, specReflectWt, specRefractWt, clearcoatWt;
    float approxFresnel = FresnelMix(state.mat, state.eta, V, L, H);
    GetLobeProbabilities(state.mat, state.eta, specCol, approxFresnel, diffuseWt, specReflectWt, specRefractWt, clearcoatWt);

    float pdf;

    // Diffuse
    if (diffuseWt > 0.0 && L.z > 0.0)
    {
        f += EvalDiffuse(state.mat, sheenCol, V, L, H, pdf);
        bsdfPdf += pdf * diffuseWt;
    }

    // Specular Reflection
    if (specReflectWt > 0.0 && L.z > 0.0 && V.z > 0.0)
    {
        f += EvalSpecReflection(state.mat, state.eta, specCol, V, L, H, pdf);
        bsdfPdf += pdf * specReflectWt;
    }

    // Specular Refraction
    if (specRefractWt > 0.0 && L.z < 0.0)
    {
        f += EvalSpecRefraction(state.mat, state.eta, V, L, H, pdf);
        bsdfPdf += pdf * specRefractWt;
    }

    // Clearcoat
    if (clearcoatWt > 0.0 && L.z > 0.0 && V.z > 0.0)
    {
        f += EvalClearcoat(state.mat, V, L, H, pdf);
        bsdfPdf += pdf * clearcoatWt;
    }

    return f * abs(L.z);
}
