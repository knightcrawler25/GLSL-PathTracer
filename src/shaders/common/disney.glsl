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

 /* References:
 * [1] [Physically Based Shading at Disney] https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf
 * [2] [Extending the Disney BRDF to a BSDF with Integrated Subsurface Scattering] https://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf
 * [3] [The Disney BRDF Explorer] https://github.com/wdas/brdf/blob/main/src/brdfs/disney.brdf
 * [4] [Miles Macklin's implementation] https://github.com/mmacklin/tinsel/blob/master/src/disney.h
 * [5] [Simon Kallweit's project report] http://simon-kallweit.me/rendercompo2015/report/
 * [6] [Microfacet Models for Refraction through Rough Surfaces] https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
 * [7] [Sampling the GGX Distribution of Visible Normals] https://jcgt.org/published/0007/04/01/paper.pdf
 * [8] [Pixar's Foundation for Materials] https://graphics.pixar.com/library/PxrMaterialsCourse2017/paper.pdf
 */

vec3 ToWorld(vec3 X, vec3 Y, vec3 Z, vec3 V)
{
    return V.x * X + V.y * Y + V.z * Z;
}

vec3 ToLocal(vec3 X, vec3 Y, vec3 Z, vec3 V)
{
    return vec3(dot(V, X), dot(V, Y), dot(V, Z));
}

void SpecAndSheenColor(Material mat, float eta, out vec3 specCol, out vec3 sheenCol)
{
    float lum = Luminance(mat.baseColor);
    vec3 ctint = lum > 0.0 ? mat.baseColor / lum : vec3(1.0f);
    float F0 = (1.0 - eta) / (1.0 + eta);
    specCol = mix(F0 * F0 * mix(vec3(1.0), ctint, mat.specularTint), mat.baseColor, mat.metallic);
    sheenCol = mix(vec3(1.0), ctint, mat.sheenTint);
}

vec3 EvalDisneyDiffuse(Material mat, vec3 Csheen, vec3 V, vec3 L, vec3 H, out float pdf)
{
    pdf = 0.0;
    if (L.z <= 0.0)
        return vec3(0.0);

    float LDotH = dot(L, H);

    // Diffuse
    float FL = SchlickWeight(L.z);
    float FV = SchlickWeight(V.z);
    float FH = SchlickWeight(LDotH);
    float Fd90 = 0.5 + 2.0 * LDotH * LDotH * mat.roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // Fake subsurface
    float Fss90 = LDotH * LDotH * mat.roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (L.z + V.z) - 0.5) + 0.5);

    // Sheen
    vec3 Fsheen = FH * mat.sheen * Csheen;

    pdf = L.z * INV_PI;
    return INV_PI * mix(Fd, ss, mat.subsurface) * mat.baseColor + Fsheen;
}

vec3 EvalMicrofacetReflection(Material mat, vec3 V, vec3 L, vec3 H, vec3 F, out float pdf)
{
    pdf = 0.0;
    if (L.z <= 0.0)
        return vec3(0.0);

    float D = GTR2Aniso(H.z, H.x, H.y, mat.ax, mat.ay);
    float G1 = SmithGAniso(abs(V.z), V.x, V.y, mat.ax, mat.ay);
    float G2 = G1 * SmithGAniso(abs(L.z), L.x, L.y, mat.ax, mat.ay);

    pdf = G1 * D / (4.0 * V.z);
    return F * D * G2 / (4.0 * L.z * V.z);
}

vec3 EvalMicrofacetRefraction(Material mat, float eta, vec3 V, vec3 L, vec3 H, float F, out float pdf)
{
    pdf = 0.0;
    if (L.z >= 0.0)
        return vec3(0.0);

    float LDotH = dot(L, H);
    float VDotH = dot(V, H);

    float D = GTR2Aniso(H.z, H.x, H.y, mat.ax, mat.ay);
    float G1 = SmithGAniso(abs(V.z), V.x, V.y, mat.ax, mat.ay);
    float G2 = G1 * SmithGAniso(abs(L.z), L.x, L.y, mat.ax, mat.ay);
    float denom = LDotH + VDotH * eta;
    denom *= denom;
    float eta2 = eta * eta;
    float jacobian = abs(LDotH) / denom;

    pdf = G1 * max(0.0, VDotH) * D * jacobian / V.z;
    return pow(mat.baseColor, vec3(0.5)) * (1.0 - F) * D * G2 * abs(VDotH) * jacobian * eta2 / abs(L.z * V.z);
}

vec3 EvalClearcoat(Material mat, vec3 V, vec3 L, vec3 H, out float pdf)
{
    pdf = 0.0;
    if (L.z <= 0.0)
        return vec3(0.0);

    float VDotH = dot(V, H);

    float FH = DielectricFresnel(VDotH, 1.0 / 1.5);
    float F = mix(0.04, 1.0, FH);
    float D = GTR1(H.z, mat.clearcoatRoughness);
    float G = SmithG(L.z, 0.25) * SmithG(V.z, 0.25);
    float jacobian = 1.0 / (4.0 * VDotH);

    pdf = D * H.z * jacobian;
    return vec3(1.0) * F * D * G / (4.0 * L.z * V.z);
}

vec3 DisneySample(State state, vec3 V, vec3 N, out vec3 L, out float pdf)
{
    pdf = 0.0;
    vec3 f = vec3(0.0);

    float r1 = rand();
    float r2 = rand();

    // TODO: Tangent and bitangent should be calculated from mesh (provided, the mesh has proper uvs)
    vec3 T, B;
    Onb(N, T, B);

    // Transform to shading space to simplify operations (NDotL = L.z; NDotV = V.z; NDotH = H.z)
    V = ToLocal(T, B, N, V);

    // Sample microfacet normal
    vec3 H = SampleGGXVNDF(V, state.mat.ax, state.mat.ay, r1, r2);

    if (H.z < 0.0)
        H = -H;

    // Specular and sheen color
    vec3 specCol, sheenCol;
    SpecAndSheenColor(state.mat, state.eta, specCol, sheenCol);

    // Dielectric fresnel
    float dielectricFr = DielectricFresnel(abs(dot(V, H)), state.eta);
    // Disney's 2015 paper doesn't specify how fresnel is handled between the BRDF and BSDF
    vec3 disneyFr = mix(specCol, vec3(1.0), mix(dielectricFr, SchlickWeight(dot(V, H)), state.mat.metallic));

    // Lobe weights
    float diffWt = (1.0 - state.mat.metallic) * (1.0 - state.mat.specTrans);
    float transWt = (1.0 - state.mat.metallic) * state.mat.specTrans;

    // Lobe sampling probabilities
    float lumBaseCol = Luminance(state.mat.baseColor);
    float diffPr = diffWt * lumBaseCol;
    float reflectPr = Luminance(disneyFr);
    float refractPr = transWt * (1.0 - dielectricFr) * lumBaseCol;
    float clearCtPr = 0.25 * state.mat.clearcoat;

    // Normalize probabilities
    float invTotalWt = 1.0 / (diffPr + reflectPr + refractPr + clearCtPr);
    diffPr *= invTotalWt;
    reflectPr *= invTotalWt;
    refractPr *= invTotalWt;
    clearCtPr *= invTotalWt;

    // CDF of the sampling probabilities
    float cdf[4];
    cdf[0] = diffPr;
    cdf[1] = cdf[0] + reflectPr;
    cdf[2] = cdf[1] + refractPr;
    cdf[3] = cdf[2] + clearCtPr;

    // Sample and evaluate a single lobe based on its importance 
    // To keep the implementation simple, one-sample MIS is not used (See 3.1 of [8])

    float r3 = rand();

    if (r3 < cdf[0]) // Diffuse
    {
        L = CosineSampleHemisphere(r1, r2);

        H = normalize(L + V);

        f = EvalDisneyDiffuse(state.mat, sheenCol, V, L, H, pdf) * diffWt;
        pdf *= diffPr;
    }
    else if (r3 < cdf[1]) // Reflection
    {
        L = normalize(reflect(-V, H));

        f = EvalMicrofacetReflection(state.mat, V, L, H, disneyFr, pdf);
        pdf *= reflectPr;
    }
    else if (r3 < cdf[2]) // Refraction
    {
        L = normalize(refract(-V, H, state.eta));

        f = EvalMicrofacetRefraction(state.mat, state.eta, V, L, H, dielectricFr, pdf) * transWt;
        pdf *= refractPr;
    }
    else // Clearcoat
    {
        H = SampleGTR1(state.mat.clearcoatRoughness, r1, r2);

        if (H.z < 0.0)
            H = -H;

        L = normalize(reflect(-V, H));

        f = EvalClearcoat(state.mat, V, L, H, pdf) * clearCtPr;
        pdf *= clearCtPr;
    }

    L = ToWorld(T, B, N, L);
    return f * abs(dot(N, L));
}

vec3 DisneyEval(State state, vec3 V, vec3 N, vec3 L, out float pdf)
{
    pdf = 0.0;
    vec3 f = vec3(0.0);

    // TODO: Tangent and bitangent should be calculated from mesh (provided, the mesh has proper uvs)
    vec3 T, B;
    Onb(N, T, B);

    // Transform to shading space to simplify operations (NDotL = L.z; NDotV = V.z; NDotH = H.z)
    V = ToLocal(T, B, N, V); 
    L = ToLocal(T, B, N, L);

    vec3 H;
    if (L.z > 0.0)
        H = normalize(L + V);
    else
        H = normalize(L + V * state.eta);

    if (H.z < 0.0)
        H = -H;

    // Specular and sheen color
    vec3 specCol, sheenCol;
    SpecAndSheenColor(state.mat, state.eta, specCol, sheenCol);

    // Dielectric fresnel
    float dielectricFr = DielectricFresnel(abs(dot(V, H)), state.eta);
    // Disney's 2015 paper doesn't specify how fresnel is handled between the BRDF and BSDF
    vec3 disneyFr = mix(specCol, vec3(1.0), mix(dielectricFr, SchlickWeight(dot(V, H)), state.mat.metallic));

    // Lobe weights
    float diffWt = (1.0 - state.mat.metallic) * (1.0 - state.mat.specTrans);
    float transWt = (1.0 - state.mat.metallic) * state.mat.specTrans;

    // Lobe sampling probabilities
    float lumBaseCol = Luminance(state.mat.baseColor);
    float diffPr = diffWt * lumBaseCol;
    float reflectPr = Luminance(disneyFr);
    float refractPr = transWt * (1.0 - dielectricFr) * lumBaseCol;
    float clearCtPr = 0.25 * state.mat.clearcoat;

    // Normalize probabilities
    float invTotalWt = 1.0 / (diffPr + reflectPr + refractPr + clearCtPr);
    diffPr *= invTotalWt;
    reflectPr *= invTotalWt;
    refractPr *= invTotalWt;
    clearCtPr *= invTotalWt;

    bool reflect = L.z * V.z > 0;
    bool refract = L.z * V.z < 0;

    float tmpPdf = 0.0;

    // Diffuse
    if (diffPr > 0.0 && reflect)
    {
        f += EvalDisneyDiffuse(state.mat, sheenCol, V, L, H, tmpPdf) * diffWt;
        pdf += tmpPdf * diffPr;
    }

    // Reflection
    if (reflectPr > 0.0 && reflect)
    {
        f += EvalMicrofacetReflection(state.mat, V, L, H, disneyFr, tmpPdf);
        pdf += tmpPdf * reflectPr;
    }

    // Refraction
    if (refractPr > 0 && refract)
    {
        f += EvalMicrofacetRefraction(state.mat, state.eta, V, L, H, dielectricFr, tmpPdf) * transWt;
        pdf += tmpPdf * refractPr;
    }

    // Clearcoat
    if (clearCtPr > 0.0 && reflect)
    {
        f += EvalClearcoat(state.mat, V, L, H, tmpPdf) * clearCtPr;
        pdf += tmpPdf * clearCtPr;
    }

    return f * abs(L.z);
}