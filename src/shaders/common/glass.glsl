/*
 * MIT License
 *
 * Copyright(c) 2019-2020 Asif Ali
 *
 * Authors/Contributors:
 *
 * Asif Ali
 * Cedric Guillemet
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
    //    transmittance = exp(-extinction * state.hitDist * 100.0);

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