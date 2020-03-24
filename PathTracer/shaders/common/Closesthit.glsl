//-----------------------------------------------------------------------
float ClosestHit(Ray r, inout State state, inout LightSampleRec lightSampleRec)
//-----------------------------------------------------------------------
{
	float t = INFINITY;
	float d;

	// Intersect Emitters
	for (int i = 0; i < numOfLights; i++)
	{
		// Fetch light Data
		vec3 position = texelFetch(lightsTex, ivec2(i * 5 + 0, 0), 0).xyz;
		vec3 emission = texelFetch(lightsTex, ivec2(i * 5 + 1, 0), 0).xyz;
		vec3 u = texelFetch(lightsTex, ivec2(i * 5 + 2, 0), 0).xyz;
		vec3 v = texelFetch(lightsTex, ivec2(i * 5 + 3, 0), 0).xyz;
		vec3 radiusAreaType = texelFetch(lightsTex, ivec2(i * 5 + 4, 0), 0).xyz;

		if (radiusAreaType.z == 0.) // Rectangular Area Light
		{
			vec3 normal = normalize(cross(u, v));
			//if (dot(normal, r.direction) > 0.) // Hide backfacing quad light
			//	continue;
			vec4 plane = vec4(normal, dot(normal, position));
			u *= 1.0f / dot(u, u);
			v *= 1.0f / dot(v, v);

			d = RectIntersect(position, u, v, plane, r);
			if (d < 0.)
				d = INFINITY;
			if (d < t)
			{
				t = d;
				float cosTheta = dot(-r.direction, normal);
				float pdf = (t * t) / (radiusAreaType.y * cosTheta);
				lightSampleRec.emission = emission;
				lightSampleRec.pdf = pdf;
				state.isEmitter = true;
			}
		}
		if (radiusAreaType.z == 1.) // Spherical Area Light
		{
			d = SphereIntersect(radiusAreaType.x, position, r);
			if (d < 0.)
				d = INFINITY;
			if (d < t)
			{
				t = d;
				float pdf = (t * t) / radiusAreaType.y;
				lightSampleRec.emission = emission;
				lightSampleRec.pdf = pdf;
				state.isEmitter = true;
			}
		}
	}

	int stack[64];
	int ptr = 0;
	stack[ptr++] = -1;

	int idx = topBVHIndex;
	float leftHit = 0.0;
	float rightHit = 0.0;

	int currMatID = 0;
	bool meshBVH = false;

	Ray r_trans;
	mat4 temp_transform;
	r_trans.origin = r.origin;
	r_trans.direction = r.direction;

	while (idx > -1 || meshBVH)
	{
		int n = idx;

		if (meshBVH && idx < 0)
		{
			meshBVH = false;

			idx = stack[--ptr];

			r_trans.origin = r.origin;
			r_trans.direction = r.direction;
			continue;
		}

		ivec2 index = ivec2(n >> 12, n & 0x00000FFF);
		ivec3 LRLeaf = texelFetch(BVH, index, 0).xyz;

		int leftIndex = int(LRLeaf.x);
		int rightIndex = int(LRLeaf.y);
		int leaf = int(LRLeaf.z);

		if (leaf > 0)
		{
			for (int i = 0; i < rightIndex; i++) // Loop through indices
			{
				ivec2 index = ivec2((leftIndex + i) % vertIndicesSize, (leftIndex + i) / vertIndicesSize);
				ivec3 vert_indices = texelFetch(vertexIndicesTex, index, 0).xyz;

				vec4 v0 = texelFetch(verticesTex, ivec2(vert_indices.x >> 12, vert_indices.x & 0x00000FFF), 0).xyzw;
				vec4 v1 = texelFetch(verticesTex, ivec2(vert_indices.y >> 12, vert_indices.y & 0x00000FFF), 0).xyzw;
				vec4 v2 = texelFetch(verticesTex, ivec2(vert_indices.z >> 12, vert_indices.z & 0x00000FFF), 0).xyzw;

				vec3 e0 = v1.xyz - v0.xyz;
				vec3 e1 = v2.xyz - v0.xyz;
				vec3 pv = cross(r_trans.direction, e1);
				float det = dot(e0, pv);

				vec3 tv = r_trans.origin - v0.xyz;
				vec3 qv = cross(tv, e0);

				vec4 uvt;
				uvt.x = dot(tv, pv);
				uvt.y = dot(r_trans.direction, qv);
				uvt.z = dot(e1, qv);
				uvt.xyz = uvt.xyz / det;
				uvt.w = 1.0 - uvt.x - uvt.y;

				if (all(greaterThanEqual(uvt, vec4(0.0))) && uvt.z < t)
				{
					t = uvt.z;
					state.isEmitter = false;
					state.triID = vert_indices;
					state.matID = currMatID;
					state.fhp = r_trans.origin + r_trans.direction * t;
					state.bary = uvt.wxy;
					tempTexCoords = vec3(v0.w, v1.w, v2.w);
					state.fhp = vec3(temp_transform * vec4(state.fhp, 1.0));
					transform = temp_transform;
				}
			}
		}
		else if (leaf < 0)
		{
			idx = leftIndex;

			vec4 r1 = texelFetch(transformsTex, ivec2((-leaf - 1) * 4 + 0, 0), 0).xyzw;
			vec4 r2 = texelFetch(transformsTex, ivec2((-leaf - 1) * 4 + 1, 0), 0).xyzw;
			vec4 r3 = texelFetch(transformsTex, ivec2((-leaf - 1) * 4 + 2, 0), 0).xyzw;
			vec4 r4 = texelFetch(transformsTex, ivec2((-leaf - 1) * 4 + 3, 0), 0).xyzw;

			temp_transform = mat4(r1, r2, r3, r4);

			r_trans.origin = vec3(inverse(temp_transform) * vec4(r.origin, 1.0));
			r_trans.direction = vec3(inverse(temp_transform) * vec4(r.direction, 0.0));

			stack[ptr++] = -1;
			meshBVH = true;
			currMatID = rightIndex;
			continue;
		}
		else
		{
			ivec2 lc = ivec2(leftIndex >> 12, leftIndex & 0x00000FFF);
			ivec2 rc = ivec2(rightIndex >> 12, rightIndex & 0x00000FFF);

			leftHit = AABBIntersect(texelFetch(BBoxMin, lc, 0).xyz, texelFetch(BBoxMax, lc, 0).xyz, r_trans);
			rightHit = AABBIntersect(texelFetch(BBoxMin, rc, 0).xyz, texelFetch(BBoxMax, rc, 0).xyz, r_trans);

			if (leftHit > 0.0 && rightHit > 0.0)
			{
				int deferred = -1;
				if (leftHit > rightHit)
				{
					idx = rightIndex;
					deferred = leftIndex;
				}
				else
				{
					idx = leftIndex;
					deferred = rightIndex;
				}

				stack[ptr++] = deferred;
				continue;
			}
			else if (leftHit > 0.)
			{
				idx = leftIndex;
				continue;
			}
			else if (rightHit > 0.)
			{
				idx = rightIndex;
				continue;
			}
		}
		idx = stack[--ptr];
	}

	state.hitDist = t;
	return t;
}