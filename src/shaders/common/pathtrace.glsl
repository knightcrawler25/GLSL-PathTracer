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
void GetMaterial(inout State state, in Ray r)
//-----------------------------------------------------------------------
{
    int index = state.matID * 6;
    Material mat;

    vec4 param1 = texelFetch(materialsTex, ivec2(index + 0, 0), 0);
    vec4 param2 = texelFetch(materialsTex, ivec2(index + 1, 0), 0);
    vec4 param3 = texelFetch(materialsTex, ivec2(index + 2, 0), 0);
    vec4 param4 = texelFetch(materialsTex, ivec2(index + 3, 0), 0);
    vec4 param5 = texelFetch(materialsTex, ivec2(index + 4, 0), 0);
    vec4 param6 = texelFetch(materialsTex, ivec2(index + 5, 0), 0);

    mat.albedo         = param1.xyz;
    mat.specular       = param1.w;

    mat.emission       = param2.xyz;
    mat.anisotropic    = param2.w;

    mat.metallic       = param3.x;
    mat.roughness      = max(param3.y, 0.001);

    mat.subsurface     = param3.z;
    mat.specularTint   = param3.w;

    mat.sheen          = param4.x;
    mat.sheenTint      = param4.y;
    mat.clearcoat      = param4.z;
    mat.clearcoatGloss = param4.w;

    mat.specTrans      = param5.x;
    mat.ior            = param5.y;
    mat.intMediumID    = param5.z;
    mat.extMediumID    = param5.w;

    vec3 texIDs        = param6.xyz;

    vec2 texUV = state.texCoord;
    texUV.y = 1.0 - texUV.y;

    // Albedo Map
    if (int(texIDs.x) >= 0)
        mat.albedo *= pow(texture(textureMapsArrayTex, vec3(texUV, int(texIDs.x))).xyz, vec3(2.2));

    // Metallic Roughness Map
    if (int(texIDs.y) >= 0)
    {
        vec2 matRgh;
        // TODO: Change metallic roughness maps in repo to linear space and remove gamma correction
        matRgh = pow(texture(textureMapsArrayTex, vec3(texUV, int(texIDs.y))).xy, vec2(2.2));
        mat.metallic = matRgh.x;
        mat.roughness = max(matRgh.y, 0.001);
    }

    // Normal Map
    // FIXME: Output when using a normal map doesn't match up with Cycles (Blender) output
    if (int(texIDs.z) >= 0)
    {
        vec3 nrm = texture(textureMapsArrayTex, vec3(texUV, int(texIDs.z))).xyz;
        nrm = normalize(nrm * 2.0 - 1.0);

        vec3 T, B;
        Onb(state.normal, T, B);

        nrm = T * nrm.x + B * nrm.y + state.normal * nrm.z;
        state.normal = normalize(nrm);
        state.ffnormal = dot(state.normal, r.direction) <= 0.0 ? state.normal : state.normal * -1.0;

        Onb(state.normal, state.tangent, state.bitangent);
    }

    // Commented out the following as anisotropic param is temporarily unused.
    // Calculate anisotropic roughness along the tangent and bitangent directions
    // float aspect = sqrt(1.0 - mat.anisotropic * 0.9);
    // mat.ax = max(0.001, mat.roughness / aspect);
    // mat.ay = max(0.001, mat.roughness * aspect);

    state.mat = mat;
    state.eta = dot(state.normal, state.ffnormal) > 0.0 ? (1.0 / mat.ior) : mat.ior;
}

//-----------------------------------------------------------------------
void UpdateMedium(inout State state)
//-----------------------------------------------------------------------
{
    state.medium.sigmaA = texelFetch(mediumsTex, ivec2(state.mediumID + 0, 0), 0).xyz;
    state.medium.sigmaS = texelFetch(mediumsTex, ivec2(state.mediumID + 1, 0), 0).xyz;
}

//-----------------------------------------------------------------------
vec3 EvalTransmittance(Ray r, State state)
//-----------------------------------------------------------------------
{
    LightSampleRec lightSampleRec;
    vec3 Tr = vec3(1.0);

    for (int depth = 0; depth < 10; depth++)
    {
        bool hit = ClosestHit(r, state, lightSampleRec);

        if (state.isEmitter)
            break;
        else
            GetMaterial(state, r);

        if (hit && !(state.mat.specTrans > 0.))
            return vec3(0.0);

        if (state.mediumID != -1)
        {
            vec3 sigmaT = state.medium.sigmaA + state.medium.sigmaS;
            Tr *= exp(-sigmaT * state.hitDist);
        }

        // Get the interior or the exterior medium based on ray direction and surface normal
        if (dot(r.direction, state.normal) < 0.0)
            state.mediumID = int(state.mat.intMediumID);
        else
            state.mediumID = int(state.mat.extMediumID);

        if (state.mediumID != -1)
            UpdateMedium(state);

        r.origin = state.hitPoint + r.direction * EPS;
    }

    return Tr;
}

//-----------------------------------------------------------------------
vec3 DirectLight(Ray r, State state, bool isSurface)
//-----------------------------------------------------------------------
{
    vec3 Ld = vec3(0.0);
    vec3 Li;
    vec3 scatterPos = state.hitPoint + state.normal * EPS;
    ScatteringRec scatteringRec;
    LightSampleRec lightSampleRec;

    // Environment Light
#ifdef ENVMAP
#ifndef CONSTANT_BG
    {
        vec4 dirPdf = EnvSample(Li);
        lightSampleRec.direction = dirPdf.xyz;
        lightSampleRec.pdf = dirPdf.w;

        Ray shadowRay = Ray(scatterPos, lightSampleRec.direction);

        //if (handleMedia)    
            Li *= EvalTransmittance(shadowRay, state);
        //else
        //  Li *= AnyHit(shadowRay, lightSampleRec.dist - EPS) ? vec3(0.0) : vec3(1.0);
        if (isSurface)
            scatteringRec.f = DisneyEval(state, -r.direction, state.ffnormal, lightSampleRec.direction, scatteringRec.pdf);
        else
        {
            scatteringRec.f = vec3(INV_4_PI);
            scatteringRec.pdf = INV_4_PI;
        }

        float weight = powerHeuristic(lightSampleRec.pdf, scatteringRec.pdf);

        if (scatteringRec.pdf > 0.0)
            Ld += weight * Li * scatteringRec.f / lightSampleRec.pdf;
    }
#endif
#endif

    // Analytic Lights 
#ifdef LIGHTS
    {
        Light light;

        //Pick a light to sample
        int index = int(rand() * float(numOfLights)) * 5;

        // Fetch light Data
        vec3 position = texelFetch(lightsTex, ivec2(index + 0, 0), 0).xyz;
        vec3 emission = texelFetch(lightsTex, ivec2(index + 1, 0), 0).xyz;
        vec3 u        = texelFetch(lightsTex, ivec2(index + 2, 0), 0).xyz; // u vector for rect
        vec3 v        = texelFetch(lightsTex, ivec2(index + 3, 0), 0).xyz; // v vector for rect
        vec3 params   = texelFetch(lightsTex, ivec2(index + 4, 0), 0).xyz;
        float radius  = params.x;
        float area    = params.y;
        float type    = params.z; // 0->Rect, 1->Sphere, 2->Distant

        light = Light(position, emission, u, v, radius, area, type);
        sampleOneLight(light, scatterPos, lightSampleRec);
        Li = lightSampleRec.emission;

        if (dot(lightSampleRec.direction, lightSampleRec.normal) < 0.0) // Required for quad lights with single sided emission
        {
            Ray shadowRay = Ray(scatterPos, lightSampleRec.direction);
            
            //if (handleMedia)    
                Li *= EvalTransmittance(shadowRay, state);
            //else
            //Li *= AnyHit(shadowRay, lightSampleRec.dist - EPS) ? vec3(0.0) : vec3(1.0);
            if (isSurface)
                scatteringRec.f = DisneyEval(state, -r.direction, state.ffnormal, lightSampleRec.direction, scatteringRec.pdf);
            else
            {
                scatteringRec.f = vec3(INV_4_PI);
                scatteringRec.pdf = INV_4_PI;
            }

            float weight = 1.0;
            if(light.area > 0.0) // No MIS for distant light
                weight = powerHeuristic(lightSampleRec.pdf, scatteringRec.pdf);

            if (scatteringRec.pdf > 0.0)
                Ld += weight * Li * scatteringRec.f / lightSampleRec.pdf;
        }
    }
#endif

    return Ld;
}


//-----------------------------------------------------------------------
vec3 PathTrace(Ray r)
//-----------------------------------------------------------------------
{
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);
    State state;
    state.mediumID = -1;
    LightSampleRec lightSampleRec;
    ScatteringRec scatteringRec;
    
    for (int depth = 0; depth < maxDepth; depth++)
    {
        state.depth = depth;
        bool hit = ClosestHit(r, state, lightSampleRec);

        if (!hit)
        {
#ifdef CONSTANT_BG
            radiance += bgColor * throughput;
#else
#ifdef ENVMAP
            {
                vec2 uv = vec2((PI + atan(r.direction.z, r.direction.x)) * (1.0 / TWO_PI), acos(r.direction.y) * (1.0 / PI));
                
                // TODO: Fix NaNs when using certain HDRs
                float misWeight = 1.0f;
                if (depth > 0)
                    misWeight = powerHeuristic(scatteringRec.pdf, EnvPdf(r));

                radiance += misWeight * texture(hdrTex, uv).xyz * throughput * hdrMultiplier;
            }
#endif
#endif
            return radiance;
        }

        GetMaterial(state, r);

        radiance += state.mat.emission * throughput;

#ifdef LIGHTS
        if (state.isEmitter)
        {
            radiance += EmitterSample(r, state, lightSampleRec, scatteringRec) * throughput;
            break;
        }
#endif

        bool mediumSampled = false;
        if(state.mediumID != -1)
        {
            // Sample distance in medium
            vec3 sigmaT = state.medium.sigmaA + state.medium.sigmaS;
            int channel = int(rand() * 3.0);
            float sampledDist = min(-log(1.0 - rand()) / sigmaT[channel], state.hitDist);

            mediumSampled = sampledDist < state.hitDist;
           
            vec3 tr = exp(-sigmaT * sampledDist);
            vec3 density = mediumSampled ? (sigmaT * tr) : tr;
            scatteringRec.pdf = (density.x + density.y + density.z) / 3.0;
            throughput *= mediumSampled ? (tr * state.medium.sigmaS) / scatteringRec.pdf : tr / scatteringRec.pdf;

            if (mediumSampled)
            {
                r.origin += r.direction * sampledDist;
                r.direction = UniformSampleSphere(rand(), rand());
                state.hitPoint = r.origin;
                radiance += DirectLight(r, state, false) * throughput;
            }
        }

        if (!mediumSampled)
        {
            if (state.mat.specTrans > 0.)
            {
                scatteringRec.L = r.direction;
                --depth;
            }
            else
            {
                // Sample light sources from surface
                radiance += DirectLight(r, state, true) * throughput;

                scatteringRec.f = DisneySample(state, -r.direction, state.ffnormal, scatteringRec.L, scatteringRec.pdf);

                if (scatteringRec.pdf > 0.0)
                    throughput *= scatteringRec.f / scatteringRec.pdf;
                else
                    break;
            }

            // Get the interior or the exterior medium based on scattering direction and surface normal
            if (dot(scatteringRec.L, state.normal) < 0.0)
                state.mediumID = int(state.mat.intMediumID);
            else
                state.mediumID = int(state.mat.extMediumID);

            if(state.mediumID != -1)
                UpdateMedium(state);

            r.direction = scatteringRec.L;
            r.origin = state.hitPoint + r.direction * EPS;
        }
        
#ifdef RR
        // Russian roulette
        if (depth >= RR_DEPTH)
        {
            float q = min(max(throughput.x, max(throughput.y, throughput.z)) + 0.001, 0.95);
            if (rand() > q)
                break;
            throughput /= q;
        }
#endif
    }
    return radiance;
}