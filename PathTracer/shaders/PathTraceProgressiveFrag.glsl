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
struct Light { vec3 position; vec3 emission; vec3 u; vec3 v; vec3 radiusAreaType; };
struct State { vec3 normal; vec3 ffnormal; vec3 fhp; bool isEmitter; int depth; float hitDist; vec2 texCoord; vec3 bary; ivec3 triID; int matID; Material mat; bool specularBounce; };
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
bool SceneIntersectShadow(Ray r, float maxDist)
//-----------------------------------------------------------------------
{
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

				vec3 v0 = texelFetch(verticesTex, ivec2(vert_indices.x >> 12, vert_indices.x & 0x00000FFF), 0).xyz;
				vec3 v1 = texelFetch(verticesTex, ivec2(vert_indices.y >> 12, vert_indices.y & 0x00000FFF), 0).xyz;
				vec3 v2 = texelFetch(verticesTex, ivec2(vert_indices.z >> 12, vert_indices.z & 0x00000FFF), 0).xyz;

				vec3 e0 = v1 - v0;
				vec3 e1 = v2 - v0;
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

				if (all(greaterThanEqual(uvt, vec4(0.0))) && uvt.z < maxDist)
					return true;
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

	return false;
}

//-----------------------------------------------------------------------
vec3 CosineSampleHemisphere(float u1, float u2)
//-----------------------------------------------------------------------
{
	vec3 dir;
	float r = sqrt(u1);
	float phi = 2.0 * PI * u2;
	dir.x = r * cos(phi);
	dir.y = r * sin(phi);
	dir.z = sqrt(max(0.0, 1.0 - dir.x*dir.x - dir.y*dir.y));

	return dir;
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

	normal = normalize(vec3(transform * vec4(normal, 0.0)));
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
float SchlickFresnel(float u)
//-----------------------------------------------------------------------
{
	float m = clamp(1.0 - u, 0.0, 1.0);
	float m2 = m * m;
	return m2 * m2*m; // pow(m,5)
}

//-----------------------------------------------------------------------
float GTR2(float NDotH, float a)
//-----------------------------------------------------------------------
{
	float a2 = a * a;
	float t = 1.0 + (a2 - 1.0)*NDotH*NDotH;
	return a2 / (PI * t*t);
}

//-----------------------------------------------------------------------
float SmithG_GGX(float NDotv, float alphaG)
//-----------------------------------------------------------------------
{
	float a = alphaG * alphaG;
	float b = NDotv * NDotv;
	return 1.0 / (NDotv + sqrt(a + b - a * b));
}

//-----------------------------------------------------------------------
float UE4Pdf(in Ray ray, inout State state, in vec3 bsdfDir)
//-----------------------------------------------------------------------
{
	vec3 n = state.normal;
	vec3 V = -ray.direction;
	vec3 L = bsdfDir;

	float specularAlpha = max(0.001, state.mat.param.y);

	float diffuseRatio = 0.5 * (1.0 - state.mat.param.x);
	float specularRatio = 1.0 - diffuseRatio;

	vec3 halfVec = normalize(L + V);

	float cosTheta = abs(dot(halfVec, n));
	float pdfGTR2 = GTR2(cosTheta, specularAlpha) * cosTheta;

	// calculate diffuse and specular pdfs and mix ratio
	float pdfSpec = pdfGTR2 / (4.0 * abs(dot(L, halfVec)));
	float pdfDiff = abs(dot(L, n)) * (1.0 / PI);

	// weight pdfs according to ratios
	return diffuseRatio * pdfDiff + specularRatio * pdfSpec;
}

//-----------------------------------------------------------------------
vec3 UE4Sample(in Ray ray, inout State state)
//-----------------------------------------------------------------------
{
	vec3 N = state.normal;
	vec3 V = -ray.direction;

	vec3 dir;

	float probability = rand();
	float diffuseRatio = 0.5 * (1.0 - state.mat.param.x);

	float r1 = rand();
	float r2 = rand();

	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
	vec3 TangentX = normalize(cross(UpVector, N));
	vec3 TangentY = cross(N, TangentX);

	if (probability < diffuseRatio) // sample diffuse
	{
		dir = CosineSampleHemisphere(r1, r2);
		dir = TangentX * dir.x + TangentY * dir.y + N * dir.z;
	}
	else
	{
		float a = max(0.001, state.mat.param.y);

		float phi = r1 * 2.0 * PI;

		float cosTheta = sqrt((1.0 - r2) / (1.0 + (a*a - 1.0) *r2));
		float sinTheta = clamp(sqrt(1.0 - (cosTheta * cosTheta)), 0.0, 1.0);
		float sinPhi = sin(phi);
		float cosPhi = cos(phi);

		vec3 halfVec = vec3(sinTheta*cosPhi, sinTheta*sinPhi, cosTheta);
		halfVec = TangentX * halfVec.x + TangentY * halfVec.y + N * halfVec.z;

		dir = 2.0*dot(V, halfVec)*halfVec - V;
	}
	return dir;
}

//-----------------------------------------------------------------------
vec3 UE4Eval(in Ray ray, inout State state, in vec3 bsdfDir)
//-----------------------------------------------------------------------
{
	vec3 N = state.normal;
	vec3 V = -ray.direction;
	vec3 L = bsdfDir;

	float NDotL = dot(N, L);
	float NDotV = dot(N, V);

	if (NDotL <= 0.0 || NDotV <= 0.0)
		return vec3(0.0);

	vec3 H = normalize(L + V);
	float NDotH = dot(N, H);
	float LDotH = dot(L, H);

	// specular	
	float specular = 0.5;
	vec3 specularCol = mix(vec3(1.0) * 0.08 * specular, state.mat.albedo.xyz, state.mat.param.x);
	float a = max(0.001, state.mat.param.y);
	float Ds = GTR2(NDotH, a);
	float FH = SchlickFresnel(LDotH);
	vec3 Fs = mix(specularCol, vec3(1.0), FH);
	float roughg = (state.mat.param.y*0.5 + 0.5);
	roughg = roughg * roughg;
	float Gs = SmithG_GGX(NDotL, roughg) * SmithG_GGX(NDotV, roughg);

	return (state.mat.albedo.xyz / PI) * (1.0 - state.mat.param.x) + Gs * Fs*Ds;
}

//-----------------------------------------------------------------------
float GlassPdf(Ray ray, inout State state)
//-----------------------------------------------------------------------
{
	return 1.0;
}

//-----------------------------------------------------------------------
vec3 GlassSample(in Ray ray, inout State state)
//-----------------------------------------------------------------------
{
	float n1 = 1.0;
	float n2 = state.mat.param.z;
	float R0 = (n1 - n2) / (n1 + n2);
	R0 *= R0;
	float theta = dot(-ray.direction, state.ffnormal);
	float prob = R0 + (1. - R0) * SchlickFresnel(theta);
	vec3 dir;

	//vec3 transmittance = vec3(1.0);
	//vec3 extinction = -log(vec3(0.1, 0.1, 0.908));
	//vec3 extinction = -log(vec3(0.905, 0.63, 0.3));

	float eta = dot(state.normal, state.ffnormal) > 0.0 ? (n1 / n2) : (n2 / n1);
	vec3 transDir = normalize(refract(ray.direction, state.ffnormal, eta));
	float cos2t = 1.0 - eta * eta * (1.0 - theta * theta);

	//if(dot(-ray.direction, state.normal) <= 0.0)
	//	transmittance = exp(-extinction * state.hitDist * 100.0);

	if (cos2t < 0.0 || rand() < prob) // Reflection
	{
		dir = normalize(reflect(ray.direction, state.ffnormal));
	}
	else  // Transmission
	{
		dir = transDir;
	}
	//state.mat.albedo.xyz = transmittance;
	return dir;
}

//-----------------------------------------------------------------------
vec3 GlassEval(in Ray ray, inout State state)
//-----------------------------------------------------------------------
{
	return state.mat.albedo.xyz;
}


//-----------------------------------------------------------------------
float powerHeuristic(float a, float b)
//-----------------------------------------------------------------------
{
	float t = a * a;
	return t / (b*b + t);
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
float EnvPdf(in Ray r)
//-----------------------------------------------------------------------
{
	float theta = acos(clamp(r.direction.y, -1.0, 1.0));
	vec2 uv = vec2((PI + atan(r.direction.z, r.direction.x)) * (1.0 / TWO_PI), theta * (1.0 / PI));
	float pdf = texture(hdrCondDistTex, uv).y * texture(hdrMarginalDistTex, vec2(uv.y, 0.)).y;
	return (pdf * hdrResolution) / (2.0 * PI * PI * sin(theta));
}

//-----------------------------------------------------------------------
vec4 EnvSample(inout vec3 color)
//-----------------------------------------------------------------------
{
	float r1 = rand();
	float r2 = rand();

	float v = texture(hdrMarginalDistTex, vec2(r1, 0.)).x;
	float u = texture(hdrCondDistTex, vec2(r2, v)).x;

	color = texture(hdrTex, vec2(u, v)).xyz * hdrMultiplier;
	float pdf = texture(hdrCondDistTex, vec2(u, v)).y * texture(hdrMarginalDistTex, vec2(v, 0.)).y;

	float phi = u * TWO_PI;
	float theta = v * PI;

	if (sin(theta) == 0.0)
		pdf = 0.0;

	return vec4(-sin(theta) * cos(phi), cos(theta), -sin(theta)*sin(phi), (pdf * hdrResolution) / (2.0 * PI * PI * sin(theta)));
}
//-----------------------------------------------------------------------
vec3 DirectLight(in Ray r, in State state)
//-----------------------------------------------------------------------
{
	vec3 L = vec3(0.0);
	BsdfSampleRec bsdfSampleRec;

	vec3 surfacePos = state.fhp + state.normal * EPS;

	/* Environment Light */
	if (useEnvMap)
	{
		vec3 color;
		vec4 dirPdf = EnvSample(color);
		vec3 lightDir = dirPdf.xyz;
		float lightPdf = dirPdf.w;

		Ray shadowRay = Ray(surfacePos, lightDir);
		bool inShadow = SceneIntersectShadow(shadowRay, INFINITY - EPS);

		if (!inShadow)
		{
			float bsdfPdf = UE4Pdf(r, state, lightDir);
			vec3 f = UE4Eval(r, state, lightDir);

			float misWeight = powerHeuristic(lightPdf, bsdfPdf);
			if (misWeight > 0.0)
				L += misWeight * f * abs(dot(lightDir, state.normal)) * color / lightPdf;
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

		if (dot(lightDir, state.normal) <= 0.0 || dot(lightDir, lightSampleRec.normal) >= 0.0)
			return L;

		Ray shadowRay = Ray(surfacePos, lightDir);
		bool inShadow = SceneIntersectShadow(shadowRay, lightDist - EPS);

		if (!inShadow)
		{
			float bsdfPdf = UE4Pdf(r, state, lightDir);
			vec3 f = UE4Eval(r, state, lightDir);
			float lightPdf = lightDistSq / (light.radiusAreaType.y * abs(dot(lightSampleRec.normal, lightDir)));

			L += powerHeuristic(lightPdf, bsdfPdf) * f * abs(dot(state.normal, lightDir)) * lightSampleRec.emission / lightPdf;
		}
	}

	return L;
}

//-----------------------------------------------------------------------
vec3 EmitterSample(in Ray r, in State state, in LightSampleRec lightSampleRec, in BsdfSampleRec bsdfSampleRec)
//-----------------------------------------------------------------------
{
	vec3 Le;

	if (state.depth == 0 || state.specularBounce)
		Le = lightSampleRec.emission;
	else
		Le = powerHeuristic(bsdfSampleRec.pdf, lightSampleRec.pdf) * lightSampleRec.emission;

	return Le;
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
		float t = SceneIntersect(r, state, lightSampleRec);

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
				throughput *= UE4Eval(r, state, bsdfSampleRec.bsdfDir) * abs(dot(state.normal, bsdfSampleRec.bsdfDir)) / bsdfSampleRec.pdf;
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