//-----------------------------------------------------------------------
void GetNormalsAndTexCoord(inout State state, inout Ray r)
//-----------------------------------------------------------------------
{
	vec4 n1 = texelFetch(normalsTex, ivec2(state.triID.x >> 12, state.triID.x & 0x00000FFF), 0).xyzw;
	vec4 n2 = texelFetch(normalsTex, ivec2(state.triID.y >> 12, state.triID.y & 0x00000FFF), 0).xyzw;
	vec4 n3 = texelFetch(normalsTex, ivec2(state.triID.z >> 12, state.triID.z & 0x00000FFF), 0).xyzw;

	vec2 t1 = vec2(tempTexCoords.x, n1.w);
	vec2 t2 = vec2(tempTexCoords.y, n2.w);
	vec2 t3 = vec2(tempTexCoords.z, n3.w);

	state.texCoord = t1 * state.bary.x + t2 * state.bary.y + t3 * state.bary.z;

	vec3 normal = normalize(n1.xyz * state.bary.x + n2.xyz * state.bary.y + n3.xyz * state.bary.z);

	mat3 normalMatrix = transpose(inverse(mat3(transform)));
	normal = normalize(normalMatrix * normal);
	state.normal = normal;
	state.ffnormal = dot(normal, r.direction) <= 0.0 ? normal : normal * -1.0;
}

//-----------------------------------------------------------------------
void GetMaterialsAndTextures(inout State state, in Ray r)
//-----------------------------------------------------------------------
{
	int index = state.matID;
	Material mat;

	mat.albedo = texelFetch(materialsTex, ivec2(index * 4 + 0, 0), 0);
	mat.emission = texelFetch(materialsTex, ivec2(index * 4 + 1, 0), 0);
	mat.param = texelFetch(materialsTex, ivec2(index * 4 + 2, 0), 0);
	mat.texIDs = texelFetch(materialsTex, ivec2(index * 4 + 3, 0), 0);

	vec2 texUV = state.texCoord;
	texUV.y = 1.0 - texUV.y;

	if (int(mat.texIDs.x) >= 0)
		mat.albedo.xyz *= pow(texture(textureMapsArrayTex, vec3(texUV, int(mat.texIDs.x))).xyz, vec3(2.2));

	if (int(mat.texIDs.y) >= 0)
		mat.param.xy = pow(texture(textureMapsArrayTex, vec3(texUV, int(mat.texIDs.y))).zy, vec2(2.2));

	if (int(mat.texIDs.z) >= 0)
	{
		vec3 nrm = texture(textureMapsArrayTex, vec3(texUV, int(mat.texIDs.z))).xyz;
		nrm = normalize(nrm * 2.0 - 1.0);

		// Orthonormal Basis
		vec3 UpVector = abs(state.ffnormal.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
		vec3 TangentX = normalize(cross(UpVector, state.ffnormal));
		vec3 TangentY = cross(state.ffnormal, TangentX);

		nrm = TangentX * nrm.x + TangentY * nrm.y + state.ffnormal * nrm.z;
		state.normal = normalize(nrm);
		state.ffnormal = dot(state.normal, r.direction) <= 0.0 ? state.normal : state.normal * -1.0;
	}

	state.mat = mat;
}

//-----------------------------------------------------------------------
vec3 DirectLight(in Ray r, in State state)
//-----------------------------------------------------------------------
{
	vec3 L = vec3(0.0);
	BsdfSampleRec bsdfSampleRec;

	vec3 surfacePos = state.fhp + state.ffnormal * EPS;

	/* Environment Light */
	if (useEnvMap)
	{
		vec3 color;
		vec4 dirPdf = EnvSample(color);
		vec3 lightDir = dirPdf.xyz;
		float lightPdf = dirPdf.w;

		Ray shadowRay = Ray(surfacePos, lightDir);
		bool inShadow = AnyHit(shadowRay, INFINITY - EPS);

		if (!inShadow)
		{
			float bsdfPdf = UE4Pdf(r, state, lightDir);
			vec3 f = UE4Eval(r, state, lightDir);

			float misWeight = powerHeuristic(lightPdf, bsdfPdf);
			if (misWeight > 0.0)
				L += misWeight * f * abs(dot(lightDir, state.ffnormal)) * color / lightPdf;
		}
	}

	/* Sample Analytic Lights */
	if (numOfLights > 0)
	{
		LightSampleRec lightSampleRec;
		Light light;

		//Pick a light to sample
		int index = int(rand() * float(numOfLights));

		// Fetch light Data
		vec3 p = texelFetch(lightsTex, ivec2(index * 5 + 0, 0), 0).xyz;
		vec3 e = texelFetch(lightsTex, ivec2(index * 5 + 1, 0), 0).xyz;
		vec3 u = texelFetch(lightsTex, ivec2(index * 5 + 2, 0), 0).xyz;
		vec3 v = texelFetch(lightsTex, ivec2(index * 5 + 3, 0), 0).xyz;
		vec3 rad = texelFetch(lightsTex, ivec2(index * 5 + 4, 0), 0).xyz;

		light = Light(p, e, u, v, rad);
		sampleLight(light, lightSampleRec);

		vec3 lightDir = lightSampleRec.surfacePos - surfacePos;
		float lightDist = length(lightDir);
		float lightDistSq = lightDist * lightDist;
		lightDir /= sqrt(lightDistSq);

		if (dot(lightDir, state.ffnormal) <= 0.0 || dot(lightDir, lightSampleRec.normal) >= 0.0)
			return L;

		Ray shadowRay = Ray(surfacePos, lightDir);
		bool inShadow = AnyHit(shadowRay, lightDist - EPS);

		if (!inShadow)
		{
			float bsdfPdf = UE4Pdf(r, state, lightDir);
			vec3 f = UE4Eval(r, state, lightDir);
			float lightPdf = lightDistSq / (light.radiusAreaType.y * abs(dot(lightSampleRec.normal, lightDir)));

			L += powerHeuristic(lightPdf, bsdfPdf) * f * abs(dot(state.ffnormal, lightDir)) * lightSampleRec.emission / lightPdf;
		}
	}

	return L;
}

//-----------------------------------------------------------------------
vec3 PathTrace(Ray r)
//-----------------------------------------------------------------------
{
	vec3 radiance = vec3(0.0);
	vec3 throughput = vec3(1.0);
	State state;
	LightSampleRec lightSampleRec;
	BsdfSampleRec bsdfSampleRec;

	for (int depth = 0; depth < maxDepth; depth++)
	{
		float lightPdf = 1.0f;
		state.depth = depth;
		float t = ClosestHit(r, state, lightSampleRec);

		if (t == INFINITY)
		{
			if (useEnvMap)
			{
				float misWeight = 1.0f;
				vec2 uv = vec2((PI + atan(r.direction.z, r.direction.x)) * (1.0 / TWO_PI), acos(r.direction.y) * (1.0 / PI));

				if (depth > 0 && !state.specularBounce)
				{
					lightPdf = EnvPdf(r);
					misWeight = powerHeuristic(bsdfSampleRec.pdf, lightPdf);
				}
				radiance += misWeight * texture(hdrTex, uv).xyz * throughput * hdrMultiplier;
			}
			break;
		}

		GetNormalsAndTexCoord(state, r);
		GetMaterialsAndTextures(state, r);

		radiance += state.mat.emission.xyz * throughput;

		if (state.isEmitter)
		{
			radiance += EmitterSample(r, state, lightSampleRec, bsdfSampleRec) * throughput;
			break;
		}

		if (state.mat.albedo.w == 0.0) // UE4 Brdf
		{
			state.specularBounce = false;
			radiance += DirectLight(r, state) * throughput;

			bsdfSampleRec.bsdfDir = UE4Sample(r, state);
			bsdfSampleRec.pdf = UE4Pdf(r, state, bsdfSampleRec.bsdfDir);

			if (bsdfSampleRec.pdf > 0.0)
				throughput *= UE4Eval(r, state, bsdfSampleRec.bsdfDir) * abs(dot(state.ffnormal, bsdfSampleRec.bsdfDir)) / bsdfSampleRec.pdf;
			else
				break;
		}
		else // Glass
		{
			state.specularBounce = true;

			bsdfSampleRec.bsdfDir = GlassSample(r, state);
			bsdfSampleRec.pdf = GlassPdf(r, state);

			throughput *= GlassEval(r, state); // Pdf will always be 1.0
		}

		r.direction = bsdfSampleRec.bsdfDir;
		r.origin = state.fhp + r.direction * EPS;
	}

	return radiance;
}