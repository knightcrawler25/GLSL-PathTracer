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

void GetMaterials(inout State state, in Ray r)
{
    int index = state.matID * 8;
    Material mat;

    vec4 param1 = texelFetch(materialsTex, ivec2(index + 0, 0), 0);
    vec4 param2 = texelFetch(materialsTex, ivec2(index + 1, 0), 0);
    vec4 param3 = texelFetch(materialsTex, ivec2(index + 2, 0), 0);
    vec4 param4 = texelFetch(materialsTex, ivec2(index + 3, 0), 0);
    vec4 param5 = texelFetch(materialsTex, ivec2(index + 4, 0), 0);
    vec4 param6 = texelFetch(materialsTex, ivec2(index + 5, 0), 0);
    vec4 param7 = texelFetch(materialsTex, ivec2(index + 6, 0), 0);
    vec4 param8 = texelFetch(materialsTex, ivec2(index + 7, 0), 0);

    mat.baseColor          = param1.rgb;
    mat.anisotropic        = param1.w;
                           
    mat.emission           = param2.rgb;
                           
    mat.metallic           = param3.x;
    mat.roughness          = max(param3.y, 0.001);      
    mat.subsurface         = param3.z;
    mat.specularTint       = param3.w;
                           
    mat.sheen              = param4.x;
    mat.sheenTint          = param4.y;
    mat.clearcoat          = param4.z;
    mat.clearcoatRoughness = mix(0.1, 0.001, param4.w); // Remapping from gloss to roughness

    mat.specTrans          = param5.x;
    mat.ior                = param5.y;
    mat.atDistance         = param5.z;
                           
    mat.extinction         = param6.rgb;
                           
    ivec4 texIDs           = ivec4(param7);

    mat.opacity            = param8.x;
    mat.alphaMode          = int(param8.y);
    mat.alphaCutoff        = param8.z;

    // Base Color Map
    if (texIDs.x >= 0)
    {
        vec4 col = texture(textureMapsArrayTex, vec3(state.texCoord, texIDs.x));
        mat.baseColor.rgb *= pow(col.rgb, vec3(2.2));
        mat.opacity *= col.a;
    }

    // Metallic Roughness Map
    if (texIDs.y >= 0)
    {
        vec2 matRgh = texture(textureMapsArrayTex, vec3(state.texCoord, texIDs.y)).bg;
        mat.metallic = matRgh.x;
        mat.roughness = max(matRgh.y * matRgh.y, 0.001);
    }

    // Normal Map
    if (texIDs.z >= 0)
    {
        vec3 texNormal = texture(textureMapsArrayTex, vec3(state.texCoord, texIDs.z)).rgb;

#ifdef OPT_OPENGL_NORMALMAP
        texNormal.y = 1.0 - texNormal.y;
#endif
        texNormal = normalize(texNormal * 2.0 - 1.0);

        vec3 origNormal = state.normal;
        state.normal = normalize(state.tangent * texNormal.x + state.bitangent * texNormal.y + state.normal * texNormal.z);
        state.ffnormal = dot(origNormal, r.direction) <= 0.0 ? state.normal : -state.normal;
    }

    // Emission Map
    if (texIDs.w >= 0)
        mat.emission = pow(texture(textureMapsArrayTex, vec3(state.texCoord, texIDs.w)).rgb, vec3(2.2));

    // Commented out the following as anisotropic param is temporarily unused.
    // float aspect = sqrt(1.0 - mat.anisotropic * 0.9);
    // mat.ax = max(0.001, mat.roughness / aspect);
    // mat.ay = max(0.001, mat.roughness * aspect);

    state.mat = mat;
    state.eta = dot(r.direction, state.normal) < 0.0 ? (1.0 / mat.ior) : mat.ior;
}

vec3 DirectLight(in Ray r, in State state)
{
    vec3 Li = vec3(0.0);
    vec3 surfacePos = state.fhp + state.normal * EPS;

    BsdfSampleRec bsdfSampleRec;

    // Environment Light
#ifdef OPT_ENVMAP
#ifndef OPT_UNIFORM_LIGHT
    {
        vec3 color;
        vec4 dirPdf = SampleEnvMap(color);
        vec3 lightDir = dirPdf.xyz;
        float lightPdf = dirPdf.w;

        Ray shadowRay = Ray(surfacePos, lightDir);
        bool inShadow = AnyHit(shadowRay, INF - EPS);

        if (!inShadow)
        {
            bsdfSampleRec.f = DisneyEval(state, -r.direction, state.ffnormal, lightDir, bsdfSampleRec.pdf);

            if (bsdfSampleRec.pdf > 0.0)
            {
                float misWeight = PowerHeuristic(lightPdf, bsdfSampleRec.pdf);
                if (misWeight > 0.0)
                    Li += misWeight * bsdfSampleRec.f * color / lightPdf;
            }
        }
    }
#endif
#endif

    // Analytic Lights 
#ifdef OPT_LIGHTS
    {
        LightSampleRec lightSampleRec;
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
        SampleOneLight(light, surfacePos, lightSampleRec);

        if (dot(lightSampleRec.direction, lightSampleRec.normal) < 0.0) // Required for quad lights with single sided emission
        {
            Ray shadowRay = Ray(surfacePos, lightSampleRec.direction);
            bool inShadow = AnyHit(shadowRay, lightSampleRec.dist - EPS);

            if (!inShadow)
            {
                bsdfSampleRec.f = DisneyEval(state, -r.direction, state.ffnormal, lightSampleRec.direction, bsdfSampleRec.pdf);

                float weight = 1.0;
                if(light.area > 0.0) // No MIS for distant light
                    weight = PowerHeuristic(lightSampleRec.pdf, bsdfSampleRec.pdf);

                if (bsdfSampleRec.pdf > 0.0)
                    Li += weight * bsdfSampleRec.f * lightSampleRec.emission / lightSampleRec.pdf;
            }
        }
    }
#endif

    return Li;
}

vec4 PathTrace(Ray r)
{
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);
    State state;
    LightSampleRec lightSampleRec;
    BsdfSampleRec bsdfSampleRec;
    vec3 absorption = vec3(0.0);
    // TODO: alpha from material opacity
    float alpha = 1.0;
    
    for (int depth = 0; depth < maxDepth; depth++)
    {
        state.depth = depth;
        bool hit = ClosestHit(r, state, lightSampleRec);

        if (!hit)
        {
#if defined(OPT_BACKGROUND) || defined(OPT_TRANSPARENT_BACKGROUND) 
            if (state.depth == 0)
                alpha = 0.0;
#endif

#ifdef OPT_UNIFORM_LIGHT
#ifdef OPT_HIDE_EMITTERS
            if(state.depth > 0)
#endif
                radiance += uniformLightCol * throughput;
#else
#ifdef OPT_ENVMAP
            {
                float misWeight = 1.0f;
                vec2 uv = vec2((PI + atan(r.direction.z, r.direction.x)) * INV_TWO_PI, acos(r.direction.y) * INV_PI);

                if (depth > 0)
                {
                    float lightPdf = EnvMapPdf(r);
                    misWeight = PowerHeuristic(bsdfSampleRec.pdf, lightPdf);
                }
#ifdef OPT_HIDE_EMITTERS
                if (state.depth > 0)
#endif
                    radiance += misWeight * texture(hdrTex, uv).xyz * throughput * hdrMultiplier;
            }
#endif
#endif
            return vec4(radiance, alpha);
        }

        GetMaterials(state, r);

        // Reset absorption when ray is going out of surface
        if (dot(state.normal, state.ffnormal) > 0.0)
            absorption = vec3(0.0);

        radiance += state.mat.emission * throughput;

#ifdef OPT_LIGHTS
        if (state.isEmitter)
        {
            radiance += EmitterSample(r, state, lightSampleRec, bsdfSampleRec) * throughput;
            break;
        }
#endif

#ifdef OPT_ALPHA_TEST
        // Ignore intersection based on alpha test
        // TODO: Alphatest for anyhit()
        bool ignoreHit = false;

        if (state.mat.alphaMode == ALPHA_MODE_MASK && state.mat.opacity < state.mat.alphaCutoff)
            ignoreHit = true;
        else if(state.mat.alphaMode == ALPHA_MODE_BLEND && rand() > state.mat.opacity)
            ignoreHit = true;

        if (ignoreHit)
        {
            depth--;
            r.origin = state.fhp + r.direction * EPS;
            continue;
        }
#endif

        // Add absoption
        throughput *= exp(-absorption * state.hitDist);

        radiance += DirectLight(r, state) * throughput;

        bsdfSampleRec.f = DisneySample(state, -r.direction, state.ffnormal, bsdfSampleRec.L, bsdfSampleRec.pdf);

        // Set absorption only if the ray is currently inside the object.
        if (dot(state.ffnormal, bsdfSampleRec.L) < 0.0)
            absorption = -log(state.mat.extinction) / state.mat.atDistance;

        if (bsdfSampleRec.pdf > 0.0)
            throughput *= bsdfSampleRec.f / bsdfSampleRec.pdf;
        else
            break;

#ifdef OPT_RR
        // Russian roulette
        if (depth >= OPT_RR_DEPTH)
        {
            float q = min(max(throughput.x, max(throughput.y, throughput.z)) + 0.001, 0.95);
            if (rand() > q)
                break;
            throughput /= q;
        }
#endif

        r.direction = bsdfSampleRec.L;
        r.origin = state.fhp + r.direction * EPS;
    }

    return vec4(radiance, alpha);
}