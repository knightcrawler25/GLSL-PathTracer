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

#include <iostream>
#include "Texture.h"
#include "stb_image.h"

namespace GLSLPT
{
    Texture::Texture(std::string texName, unsigned char* data, int w, int h, int c) : name(texName)
        , width(w)
        , height(h)
        , components(c)
    {
        texData.resize(width * height * components);
        std::copy(data, data + width * height * components, texData.begin());
    }

    bool Texture::LoadTexture(const std::string& filename)
    {
        name = filename;
        components = 4;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, NULL, components);
        if (data == nullptr)
            return false;
        texData.resize(width * height * components);
        std::copy(data, data + width * height * components, texData.begin());
        stbi_image_free(data);
        return true;
    }
}