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

#ifdef OPT_ENVMAP
#ifndef OPT_UNIFORM_LIGHT

vec2 BinarySearch(float value)
{
    vec2 uv;
    float lower = 0.0;
    float upper = envMapRes.y - 1.0;
    while ((lower + 0.5) < upper)
    {
        float mid = floor((lower + upper) * 0.5);
        if (value < texture(envMapCDFTex, vec2(envMapRes.x - 0.5, mid + 0.5) / envMapRes).r)
            upper = mid;
        else
            lower = mid + 1.0;
    }
    uv.y =  clamp(lower, 0.0, envMapRes.y - 1.0);

    lower = 0.0;
    upper = envMapRes.x - 1.0;
    while ((lower + 0.5) < upper)
    {
        float mid = floor((lower + upper) * 0.5);
        if (value < texture(envMapCDFTex, vec2(mid + 0.5, uv.y + 0.5) / envMapRes).r)
            upper = mid;
        else
            lower = mid + 1.0;
    }
    uv.x = clamp(lower, 0.0, envMapRes.x - 1.0);
    return uv / envMapRes;
}

float EnvMapPdf(vec3 color)
{
    return ((Luminance(color) / envMapTotalSum) * envMapRes.x * envMapRes.y) / (TWO_PI * PI);
}

vec4 SampleEnvMap(inout vec3 color)
{
    vec2 uv = BinarySearch(rand() * envMapTotalSum);

    color = texture(envMapTex, uv).xyz;
    float pdf = Luminance(color) / envMapTotalSum;

    uv.x -= envMapRot;
    float phi = uv.x * TWO_PI;
    float theta = uv.y * PI;

    if (sin(theta) == 0.0)
        pdf = 0.0;

    return vec4(-sin(theta) * cos(phi), cos(theta), -sin(theta) * sin(phi), (pdf * envMapRes.x * envMapRes.y) / (TWO_PI * PI * sin(theta)));
}

#endif
#endif