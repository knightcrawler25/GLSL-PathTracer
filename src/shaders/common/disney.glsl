/*
 * MIT License
 *
 * Copyright(c) 2019-2021 Asif Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
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

//-----------------------------------------------------------------------
vec3 EvalDielectricReflection(State state, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
//-----------------------------------------------------------------------
{
    if (dot(N, L) < 0.0) return vec3(0.0);

    float F = DielectricFresnel(dot(V, H), state.eta);
    float D = GTR2(dot(N, H), state.mat.roughness);
    
    pdf = D * dot(N, H) * F / (4.0 * dot(V, H));

    float G = SmithG_GGX(abs(dot(N, L)), state.mat.roughness) * SmithG_GGX(dot(N, V), state.mat.roughness);
    return state.mat.albedo * F * D * G;
}

//-----------------------------------------------------------------------
vec3 EvalDielectricRefraction(State state, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
//-----------------------------------------------------------------------
{
    float F = DielectricFresnel(abs(dot(V, H)), state.eta);
    float D = GTR2(dot(N, H), state.mat.roughness);

    float denomSqrt = dot(L, H) * state.eta + dot(V, H);
    pdf = D * dot(N, H) * (1.0 - F) * abs(dot(L, H)) / (denomSqrt * denomSqrt);

    float G = SmithG_GGX(abs(dot(N, L)), state.mat.roughness) * SmithG_GGX(dot(N, V), state.mat.roughness);
    return state.mat.albedo * (1.0 - F) * D * G * abs(dot(V, H)) * abs(dot(L, H)) * 4.0 * state.eta * state.eta / (denomSqrt * denomSqrt);
}

//-----------------------------------------------------------------------
vec3 EvalSpecular(State state, vec3 Cspec0, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
//-----------------------------------------------------------------------
{
    if (dot(N, L) < 0.0) return vec3(0.0);

    float D = GTR2_aniso(dot(N, H), dot(H, state.tangent), dot(H, state.bitangent), state.mat.ax, state.mat.ay);
    pdf = D * dot(N, H) / (4.0 * dot(V, H));

    float FH = SchlickFresnel(dot(L, H));
    vec3 F = mix(Cspec0, vec3(1.0), FH);
    float G = SmithG_GGX_aniso(dot(N, L), dot(L, state.tangent), dot(L, state.bitangent), state.mat.ax, state.mat.ay);
    G *= SmithG_GGX_aniso(dot(N, V), dot(V, state.tangent), dot(V, state.bitangent), state.mat.ax, state.mat.ay);
    return F * D * G;
}

//-----------------------------------------------------------------------
vec3 EvalClearcoat(State state, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
//-----------------------------------------------------------------------
{
    if (dot(N, L) < 0.0) return vec3(0.0);

    float D = GTR1(dot(N, H), state.mat.clearcoatRoughness);
    pdf = D * dot(N, H) / (4.0 * dot(V, H));

    float FH = SchlickFresnel(dot(L, H));
    float F = mix(0.04, 1.0, FH);
    float G = SmithG_GGX(dot(N, L), 0.25) * SmithG_GGX(dot(N, V), 0.25);
    return vec3(0.25 * state.mat.clearcoat * F * D * G);
}

//-----------------------------------------------------------------------
vec3 EvalDiffuse(State state, vec3 Csheen, vec3 V, vec3 N, vec3 L, vec3 H, inout float pdf)
//-----------------------------------------------------------------------
{
    if (dot(N, L) < 0.0) return vec3(0.0);

    pdf = dot(N, L) * (1.0 / PI);

    float FL = SchlickFresnel(dot(N, L));
    float FV = SchlickFresnel(dot(N, V));
    float FH = SchlickFresnel(dot(L, H));
    float Fd90 = 0.5 + 2.0 * dot(L, H) * dot(L, H) * state.mat.roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);
    vec3 Fsheen = FH * state.mat.sheen * Csheen;
    return ((1.0 / PI) * Fd * (1.0 - state.mat.subsurface) * state.mat.albedo + Fsheen) * (1.0 - state.mat.metallic);
}

//-----------------------------------------------------------------------
vec3 EvalSubsurface(State state, vec3 V, vec3 N, vec3 L, inout float pdf)
//-----------------------------------------------------------------------
{
    pdf = (1.0 / TWO_PI);

    float FL = SchlickFresnel(abs(dot(N, L)));
    float FV = SchlickFresnel(dot(N, V));
    float Fd = (1.0f - 0.5f * FL) * (1.0f - 0.5f * FV);
    return sqrt(state.mat.albedo) * state.mat.subsurface * (1.0 / PI) * Fd * (1.0 - state.mat.metallic) * (1.0 - state.mat.specTrans);
}

//-----------------------------------------------------------------------
vec3 DisneySample(inout State state, vec3 V, vec3 N, inout vec3 L, inout float pdf)
//-----------------------------------------------------------------------
{
    state.isSubsurface = false;
    pdf = 0.0;
    vec3 f = vec3(0.0);

    float r1 = rand();
    float r2 = rand();

    float diffuseRatio = 0.5 * (1.0 - state.mat.metallic);
    float transWeight = (1.0 - state.mat.metallic) * state.mat.specTrans;

    vec3 Cdlin = state.mat.albedo;
    float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

    vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0f); // normalize lum. to isolate hue+sat
    vec3 Cspec0 = mix(state.mat.specular * 0.08 * mix(vec3(1.0), Ctint, state.mat.specularTint), Cdlin, state.mat.metallic);
    vec3 Csheen = mix(vec3(1.0), Ctint, state.mat.sheenTint);

    // BSDF
    if (rand() < transWeight)
    {
        vec3 H = ImportanceSampleGTR2(state.mat.roughness, r1, r2);
        H = state.tangent * H.x + state.bitangent * H.y + N * H.z;

        vec3 R = reflect(-V, H);
        float F = DielectricFresnel(abs(dot(R, H)), state.eta);

        // Reflection/Total internal reflection
        if (rand() < F)
        {
            L = normalize(R);
            f = EvalDielectricReflection(state, V, N, L, H, pdf);
        }
        else // Transmission
        {
            L = normalize(refract(-V, H, state.eta));
            f = EvalDielectricRefraction(state, V, N, L, H, pdf);
        }

        f *= transWeight;
        pdf *= transWeight;
    }
    else // BRDF
    {
        if (rand() < diffuseRatio)
        {
            // Diffuse transmission. A way to approximate subsurface scattering
            if (rand() < state.mat.subsurface)
            {
                L = UniformSampleHemisphere(r1, r2);
                L = state.tangent * L.x + state.bitangent * L.y - N * L.z;

                f = EvalSubsurface(state, V, N, L, pdf);
                pdf *= state.mat.subsurface * diffuseRatio;

                state.isSubsurface = true; // Required when sampling lights from inside surface
            }
            else // Diffuse
            {
                L = CosineSampleHemisphere(r1, r2);
                L = state.tangent * L.x + state.bitangent * L.y + N * L.z;

                vec3 H = normalize(L + V);

                f = EvalDiffuse(state, Csheen, V, N, L, H, pdf);
                pdf *= (1.0 - state.mat.subsurface) * diffuseRatio;
            }
        }
        else // Specular
        {
            float primarySpecRatio = 1.0 / (1.0 + state.mat.clearcoat);
            
            // Sample primary specular lobe
            if (rand() < primarySpecRatio) 
            {
                // TODO: Implement http://jcgt.org/published/0007/04/01/
                vec3 H = ImportanceSampleGTR2_aniso(state.mat.ax, state.mat.ay, r1, r2);
                H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
                L = normalize(reflect(-V, H));

                f = EvalSpecular(state, Cspec0, V, N, L, H, pdf);
                pdf *= primarySpecRatio * (1.0 - diffuseRatio);
            }
            else // Sample clearcoat lobe
            {
                vec3 H = ImportanceSampleGTR1(state.mat.clearcoatRoughness, r1, r2);
                H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
                L = normalize(reflect(-V, H));

                f = EvalClearcoat(state, V, N, L, H, pdf);
                pdf *= (1.0 - primarySpecRatio) * (1.0 - diffuseRatio);
            }
        }

        f *= (1.0 - transWeight);
        pdf *= (1.0 - transWeight);
    }
    return f;
}

//-----------------------------------------------------------------------
vec3 DisneyEval(State state, vec3 V, vec3 N, vec3 L, inout float pdf)
//-----------------------------------------------------------------------
{
    vec3 H;

    if (dot(N, L) < 0.0)
        H = normalize(L * (1.0 / state.eta) + V);
    else
        H = normalize(L + V);

    if (dot(N, H) < 0.0)
        H = -H;

    float diffuseRatio = 0.5 * (1.0 - state.mat.metallic);
    float primarySpecRatio = 1.0 / (1.0 + state.mat.clearcoat);
    float transWeight = (1.0 - state.mat.metallic) * state.mat.specTrans;

    vec3 brdf = vec3(0.0);
    vec3 bsdf = vec3(0.0);
    float brdfPdf = 0.0;
    float bsdfPdf = 0.0;

    // BSDF
    if (transWeight > 0.0)
    {
        // Transmission
        if (dot(N, L) < 0.0) 
        {
            bsdf = EvalDielectricRefraction(state, V, N, L, H, bsdfPdf);
        }
        else // Reflection
        {
            bsdf = EvalDielectricReflection(state, V, N, L, H, bsdfPdf);
        }
    }

    float m_pdf;

    if (transWeight < 1.0)
    {
        // Subsurface
        if (dot(N, L) < 0.0)
        {
            // TODO: Double check this. Fails furnace test when used with rough transmission
            if (state.mat.subsurface > 0.0)
            {
                brdf = EvalSubsurface(state, V, N, L, m_pdf);
                brdfPdf = m_pdf * state.mat.subsurface * diffuseRatio;
            }
        }
        // BRDF
        else
        {
            vec3 Cdlin = state.mat.albedo;
            float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

            vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0f); // normalize lum. to isolate hue+sat
            vec3 Cspec0 = mix(state.mat.specular * 0.08 * mix(vec3(1.0), Ctint, state.mat.specularTint), Cdlin, state.mat.metallic);
            vec3 Csheen = mix(vec3(1.0), Ctint, state.mat.sheenTint);

            // Diffuse
            brdf += EvalDiffuse(state, Csheen, V, N, L, H, m_pdf);
            brdfPdf += m_pdf * (1.0 - state.mat.subsurface) * diffuseRatio;
            
            // Specular
            brdf += EvalSpecular(state, Cspec0, V, N, L, H, m_pdf);
            brdfPdf += m_pdf * primarySpecRatio * (1.0 - diffuseRatio);
            
            // Clearcoat
            brdf += EvalClearcoat(state, V, N, L, H, m_pdf);
            brdfPdf += m_pdf * (1.0 - primarySpecRatio) * (1.0 - diffuseRatio);
        }
    }

    pdf = mix(brdfPdf, bsdfPdf, transWeight);
    return mix(brdf, bsdf, transWeight);
}
