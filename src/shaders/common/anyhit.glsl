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
bool AnyHit(Ray r, float maxDist)
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

        int index = n;
        ivec3 LRLeaf = ivec3(texelFetch(BVH, index * 3 + 2).xyz);

        int leftIndex = int(LRLeaf.x);
        int rightIndex = int(LRLeaf.y);
        int leaf = int(LRLeaf.z);

        if (leaf > 0) // Leaf node of BLAS
        {
            for (int i = 0; i < rightIndex; i++) // Loop through tris
            {
                int index = leftIndex + i;
                ivec3 vert_indices = ivec3(texelFetch(vertexIndicesTex, index).xyz);

                vec4 v0 = texelFetch(verticesTex, vert_indices.x);
                vec4 v1 = texelFetch(verticesTex, vert_indices.y);
                vec4 v2 = texelFetch(verticesTex, vert_indices.z);

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

                if (all(greaterThanEqual(uvt, vec4(0.0))) && uvt.z < maxDist)
                    return true;
            }
        }
        else if (leaf < 0) // Leaf node of TLAS
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
            leftHit =  AABBIntersect(texelFetch(BVH, leftIndex  * 3 + 0).xyz, texelFetch(BVH, leftIndex  * 3 + 1).xyz, r_trans);
            rightHit = AABBIntersect(texelFetch(BVH, rightIndex * 3 + 0).xyz, texelFetch(BVH, rightIndex * 3 + 1).xyz, r_trans);

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