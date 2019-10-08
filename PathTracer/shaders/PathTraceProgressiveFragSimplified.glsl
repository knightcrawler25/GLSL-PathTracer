#version 300 es

precision highp float;
precision highp int;
precision highp sampler2D;
precision highp samplerCube;
precision highp isampler2D;
precision highp sampler2DArray;

out vec3 color;
in vec2 TexCoords;
uniform bool isCameraMoving;
uniform bool useEnvMap;
uniform vec3 randomVector;
uniform vec2 screenResolution;
uniform float hdrTexSize;

uniform sampler2D accumTexture;
uniform isampler2D BVH;
uniform sampler2D BBoxMin;
uniform sampler2D BBoxMax;
uniform isampler2D vertexIndicesTex;
uniform sampler2D verticesTex;
uniform sampler2D normalsTex;
uniform sampler2D materialsTex;
uniform sampler2D transformsTex;
uniform sampler2D lightsTex;
uniform sampler2DArray textureMapsArrayTex;

uniform sampler2D hdrTex;
uniform sampler2D hdrMarginalDistTex;
uniform sampler2D hdrCondDistTex;
uniform float hdrResolution;
uniform float hdrMultiplier;

uniform int numOfLights;
uniform int maxDepth;
uniform int topBVHIndex;
uniform int vertIndicesSize;

#define PI        3.14159265358979323
#define TWO_PI    6.28318530717958648
#define INFINITY  1000000.0
#define EPS 0.001

// Global variables

mat4 transform;

vec2 seed;
vec3 tempTexCoords;
struct Ray { vec3 origin; vec3 direction; };
struct Material { vec4 albedo; vec4 emission; vec4 param; vec4 texIDs; };
struct Camera { vec3 up; vec3 right; vec3 forward; vec3 position; float fov; float focalDist; float aperture; };
struct State { vec3 normal; vec3 ffnormal; vec3 fhp; bool isEmitter; int depth; float hitDist; vec2 texCoord; vec3 bary; ivec3 triID; int matID; Material mat; bool specularBounce; };
struct Light { vec3 position; vec3 emission; vec3 u; vec3 v; vec3 radiusAreaType; };
struct BsdfSampleRec { vec3 bsdfDir; float pdf; };
struct LightSampleRec { vec3 surfacePos; vec3 normal; vec3 emission; float pdf; };

uniform Camera camera;

//-----------------------------------------------------------------------
float rand()
//-----------------------------------------------------------------------
{
	seed -= vec2(randomVector.x * randomVector.y);
	return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

//-----------------------------------------------------------------------
float SphereIntersect(float rad, vec3 pos, Ray r)
//-----------------------------------------------------------------------
{
	vec3 op = pos - r.origin;
	float eps = 0.001;
	float b = dot(op, r.direction);
	float det = b * b - dot(op, op) + rad * rad;
	if (det < 0.0)
		return INFINITY;

	det = sqrt(det);
	float t1 = b - det;
	if (t1 > eps)
		return t1;

	float t2 = b + det;
	if (t2 > eps)
		return t2;

	return INFINITY;
}

//-----------------------------------------------------------------------
float RectIntersect(in vec3 pos, in vec3 u, in vec3 v, in vec3 normal, in vec4 plane, in Ray r)
//-----------------------------------------------------------------------
{
	vec3 n = vec3(plane);
	float dt = dot(r.direction, n);
	float t = (plane.w - dot(n, r.origin)) / dt;
	if (t > EPS)
	{
		vec3 p = r.origin + r.direction * t;
		vec3 vi = p - pos;
		float a1 = dot(u, vi);
		if (a1 >= 0. && a1 <= 1.)
		{
			float a2 = dot(v, vi);
			if (a2 >= 0. && a2 <= 1.)
				return t;
		}
	}

	return INFINITY;
}


//----------------------------------------------------------------
float IntersectRayAABB(vec3 minCorner, vec3 maxCorner, Ray r)
//----------------------------------------------------------------
{
	vec3 invdir = 1.0 / r.direction;

	vec3 f = (maxCorner - r.origin) * invdir;
	vec3 n = (minCorner - r.origin) * invdir;

	vec3 tmax = max(f, n);
	vec3 tmin = min(f, n);

	float t1 = min(tmax.x, min(tmax.y, tmax.z));
	float t0 = max(tmin.x, max(tmin.y, tmin.z));

	return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.0;
}

//-------------------------------------------------------------------------------
vec3 BarycentricCoord(vec3 point, vec3 v0, vec3 v1, vec3 v2)
//-------------------------------------------------------------------------------
{
	vec3 ab = v1 - v0;
	vec3 ac = v2 - v0;
	vec3 ah = point - v0;

	float ab_ab = dot(ab, ab);
	float ab_ac = dot(ab, ac);
	float ac_ac = dot(ac, ac);
	float ab_ah = dot(ab, ah);
	float ac_ah = dot(ac, ah);

	float inv_denom = 1.0 / (ab_ab * ac_ac - ab_ac * ab_ac);

	float v = (ac_ac * ab_ah - ab_ac * ac_ah) * inv_denom;
	float w = (ab_ab * ac_ah - ab_ac * ab_ah) * inv_denom;
	float u = 1.0 - v - w;

	return vec3(u, v, w);
}

//-----------------------------------------------------------------------
float SceneIntersect(Ray r, inout State state, inout LightSampleRec lightSampleRec)
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
			if (dot(normal, r.direction) > 0.) // Hide backfacing quad light
				continue;
			vec4 plane = vec4(normal, dot(normal, position));
			u *= 1.0f / dot(u, u);
			v *= 1.0f / dot(v, v);

			d = RectIntersect(position, u, v, normal, plane, r);
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
					state.bary = BarycentricCoord(state.fhp, v0.xyz, v1.xyz, v2.xyz);
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

			leftHit = IntersectRayAABB(texelFetch(BBoxMin, lc, 0).xyz, texelFetch(BBoxMax, lc, 0).xyz, r_trans);
			rightHit = IntersectRayAABB(texelFetch(BBoxMin, rc, 0).xyz, texelFetch(BBoxMax, rc, 0).xyz, r_trans);

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

	state.mat = mat;
}

//-----------------------------------------------------------------------
vec3 UniformSampleSphere(float u1, float u2)
//-----------------------------------------------------------------------
{
	float z = 1.0 - 2.0 * u1;
	float r = sqrt(max(0.f, 1.0 - z * z));
	float phi = 2.0 * PI * u2;
	float x = r * cos(phi);
	float y = r * sin(phi);

	return vec3(x, y, z);
}

//-----------------------------------------------------------------------
void sampleSphereLight(in Light light, inout LightSampleRec lightSampleRec)
//-----------------------------------------------------------------------
{
	float r1 = rand();
	float r2 = rand();

	lightSampleRec.surfacePos = light.position + UniformSampleSphere(r1, r2) * light.radiusAreaType.x;
	lightSampleRec.normal = normalize(lightSampleRec.surfacePos - light.position);
	lightSampleRec.emission = light.emission * float(numOfLights);
}

//-----------------------------------------------------------------------
void sampleQuadLight(in Light light, inout LightSampleRec lightSampleRec)
//-----------------------------------------------------------------------
{
	float r1 = rand();
	float r2 = rand();

	lightSampleRec.surfacePos = light.position + light.u * r1 + light.v * r2;
	lightSampleRec.normal = normalize(cross(light.u, light.v));
	lightSampleRec.emission = light.emission * float(numOfLights);
}

//-----------------------------------------------------------------------
void sampleLight(in Light light, inout LightSampleRec lightSampleRec)
//-----------------------------------------------------------------------
{
	if (int(light.radiusAreaType.z) == 0) // Quad Light
		sampleQuadLight(light, lightSampleRec);
	else
		sampleSphereLight(light, lightSampleRec);
}

//-----------------------------------------------------------------------
vec3 DirectLight(in Ray r, in State state)
//-----------------------------------------------------------------------
{
	vec3 L = vec3(0.0);
	BsdfSampleRec bsdfSampleRec;

	vec3 surfacePos = state.fhp + state.normal * EPS;

	/* Since we are not randomly sampling for progressive mode, sample all analytic Lights */
	for(int i = 0; i < numOfLights; i++)
	{
		LightSampleRec lightSampleRec;
		Light light;

		//Pick a light to sample
		int index = i;

		// Fetch light Data
		vec3 p = texelFetch(lightsTex, ivec2(index * 5 + 0, 0), 0).xyz;
		vec3 e = texelFetch(lightsTex, ivec2(index * 5 + 1, 0), 0).xyz;
		vec3 u = texelFetch(lightsTex, ivec2(index * 5 + 2, 0), 0).xyz;
		vec3 v = texelFetch(lightsTex, ivec2(index * 5 + 3, 0), 0).xyz;
		vec3 rad = texelFetch(lightsTex, ivec2(index * 5 + 4, 0), 0).xyz;

		light = Light(p, e, u, v, rad);
		sampleLight(light, lightSampleRec);

		vec3 lightDir = p - surfacePos;
		float lightDist = length(lightDir);
		float lightDistSq = lightDist * lightDist;
		lightDir /= sqrt(lightDistSq);

		if (dot(lightDir, state.normal) <= 0.0 || dot(lightDir, lightSampleRec.normal) >= 0.0)
			continue;

		float lightPdf = lightDistSq / (light.radiusAreaType.y * abs(dot(lightSampleRec.normal, lightDir)));

		L += (state.mat.albedo.xyz / PI) * abs(dot(state.normal, lightDir)) * e / lightPdf;
		
	}
	return L;
}

//-----------------------------------------------------------------------
vec3 PathTrace(Ray r)
//-----------------------------------------------------------------------
{
	vec3 radiance = vec3(0.0);
	State state;
	LightSampleRec lightSampleRec;
	BsdfSampleRec bsdfSampleRec;

	float t = SceneIntersect(r, state, lightSampleRec);

	if (t == INFINITY)
	{
		if (useEnvMap)
		{
			vec2 uv = vec2((PI + atan(r.direction.z, r.direction.x)) * (1.0 / TWO_PI), acos(r.direction.y) * (1.0 / PI));

			radiance += texture(hdrTex, uv).xyz * hdrMultiplier;
		}
	}
	else
	{
		GetNormalsAndTexCoord(state, r);
		GetMaterialsAndTextures(state, r);
		radiance += state.mat.emission.xyz;
		
		if (state.isEmitter)
		{
			radiance += lightSampleRec.emission;
			return radiance;
		}
		
		// If we have albedo textures then choose this over HDR reflection
		if (int(state.mat.texIDs.x) >= 0 && numOfLights == 0) 
		{
			radiance += state.mat.albedo.xyz;
		}
		else if (numOfLights > 0)
		{
			if(dot(state.normal, -r.direction) > 0.0)
				radiance += DirectLight(r, state);
		}
		else if (useEnvMap)
		{
			vec3 R = reflect(r.direction, normalize(state.normal));
			vec2 uv = vec2((PI + atan(R.z, R.x)) * (1.0 / TWO_PI), acos(R.y) * (1.0 / PI));
			radiance += texture(hdrTex, uv).xyz * hdrMultiplier * state.mat.albedo.xyz;
		}
	}

	return radiance;
}

void main(void)
{
	seed = gl_FragCoord.xy;

	float r1 = 2.0 * rand();
	float r2 = 2.0 * rand();

	vec2 jitter;

	jitter.x = r1 < 1.0 ? sqrt(r1) - 1.0 : 1.0 - sqrt(2.0 - r1);
	jitter.y = r2 < 1.0 ? sqrt(r2) - 1.0 : 1.0 - sqrt(2.0 - r2);
	jitter /= (screenResolution * 0.5);

	vec2 d = (2.0 * TexCoords - 1.0) + jitter;
	d.x *= screenResolution.x / screenResolution.y * tan(camera.fov / 2.0);
	d.y *= tan(camera.fov / 2.0);
	vec3 rayDir = normalize(d.x * camera.right + d.y * camera.up + camera.forward);

	Ray ray = Ray(camera.position, rayDir);

	vec3 pixelColor = PathTrace(ray);

	color = pixelColor;
}