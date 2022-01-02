
/***********************************************************************************
    Created:    17:9:2002
    FileName:     hdrloader.cpp
    Author:        Igor Kravtchenko
    
    Info:        Load HDR image and convert to a set of float32 RGB triplet.
************************************************************************************/

/* 
    This is a modified version of the original code. Added code to build marginal & conditional cdf for importance sampling
*/

#include <math.h>
#include <memory.h>
#include <stdio.h>
#include "hdrloader.h"
#include "Distribution.h"

typedef unsigned char RGBE[4];
#define R            0
#define G            1
#define B            2
#define E            3

#define  MINELEN    8                // minimum scanline length for encoding
#define  MAXELEN    0x7fff            // maximum scanline length for encoding

static void workOnRGBE(RGBE *scan, int len, float *cols);
static bool decrunch(RGBE *scanline, int len, FILE *file);
static bool oldDecrunch(RGBE *scanline, int len, FILE *file);

float luminance(const Vec3 &c)
{
    return 0.212671f * c.x + 0.715160f * c.y + 0.072169f * c.z;
}

void HDRLoader::buildDistributions(HDRData* res)
{
    int width = res->width;
    int height = res->height;

    // Gather weights for building a 2D distribution
    float* weights = new float[width * height];
    for (int v = 0; v < height; v++)
    {
        float sinTheta = sin(PI * (v + 1) / height);
        for (int u = 0; u < width; u++)
        {
            int imgIdx = v * width * 3 + u * 3;
            weights[u + v * width] = luminance(Vec3(res->cols[imgIdx + 0], res->cols[imgIdx + 1], res->cols[imgIdx + 2]));
            weights[u + v * width] *= sinTheta;
        }
    }

    Distribution2D *dist = new Distribution2D(&weights[0], width, height);
    
    // Generate a lookup table with row and col to avoid binary search in the shader
    res->lookupData = new Vec3[width * height];
    for (int v = 0; v < height; v++)
    {
        for (int u = 0; u < width; u++)
        {
            float r1 = (float)(v + 1) / height;
            float r2 = (float)(u + 1) / width;
            float pdf;
            Vec2 rowCol = dist->SampleContinuous(r1, r2, pdf);
            res->lookupData[u + v * width] = Vec3(rowCol.x, rowCol.y, pdf);
        }
    }

    delete dist;
    delete[] weights;
}

HDRData* HDRLoader::load(const char *fileName)
{
    int i;
    char str[200];
    FILE *file;

    file = fopen(fileName, "rb");
    if (!file)
        return nullptr;

    HDRData *res = new HDRData;

    fread(str, 10, 1, file);
    if (memcmp(str, "#?RADIANCE", 10)) {
        fclose(file);
        return nullptr;
    }

    fseek(file, 1, SEEK_CUR);

    char cmd[200];
    i = 0;
    char c = 0, oldc;
    while(true) {
        oldc = c;
        c = fgetc(file);
        if (c == 0xa && oldc == 0xa)
            break;
        cmd[i++] = c;
    }

    char reso[200];
    i = 0;
    while(true) {
        c = fgetc(file);
        reso[i++] = c;
        if (c == 0xa)
            break;
    }

    int w, h;
    if (!sscanf(reso, "-Y %d +X %d", &h, &w)) {
        fclose(file);
        return nullptr;
    }

    res->width = w;
    res->height = h;

    float *cols = new float[w * h * 3];
    res->cols = cols;

    RGBE *scanline = new RGBE[w];
    if (!scanline) {
        fclose(file);
        return nullptr;
    }

    // convert image 
    for (int y = h - 1; y >= 0; y--) {
        if (decrunch(scanline, w, file) == false)
            break;
        workOnRGBE(scanline, w, cols);
        cols += w * 3;
    }

    delete [] scanline;
    fclose(file);

    buildDistributions(res);
    return res;
}

float convertComponent(int expo, int val)
{
    float v = val / 256.0f;
    float d = (float) pow(2, expo);
    return v * d;
}

void workOnRGBE(RGBE *scan, int len, float *cols)
{
    while (len-- > 0) {
        int expo = scan[0][E] - 128;
        cols[0] = convertComponent(expo, scan[0][R]);
        cols[1] = convertComponent(expo, scan[0][G]);
        cols[2] = convertComponent(expo, scan[0][B]);
        cols += 3;
        scan++;
    }
}

bool decrunch(RGBE *scanline, int len, FILE *file)
{
    int  i, j;
                    
    if (len < MINELEN || len > MAXELEN)
        return oldDecrunch(scanline, len, file);

    i = fgetc(file);
    if (i != 2) {
        fseek(file, -1, SEEK_CUR);
        return oldDecrunch(scanline, len, file);
    }

    scanline[0][G] = fgetc(file);
    scanline[0][B] = fgetc(file);
    i = fgetc(file);

    if (scanline[0][G] != 2 || scanline[0][B] & 128) {
        scanline[0][R] = 2;
        scanline[0][E] = i;
        return oldDecrunch(scanline + 1, len - 1, file);
    }

    // read each component
    for (i = 0; i < 4; i++) {
        for (j = 0; j < len; ) {
            unsigned char code = fgetc(file);
            if (code > 128) { // run
                code &= 127;
                unsigned char val = fgetc(file);
                while (code--)
                    scanline[j++][i] = val;
            }
            else  {    // non-run
                while(code--)
                    scanline[j++][i] = fgetc(file);
            }
        }
    }

    return feof(file) ? false : true;
}

bool oldDecrunch(RGBE *scanline, int len, FILE *file)
{
    int i;
    int rshift = 0;
    
    while (len > 0) {
        scanline[0][R] = fgetc(file);
        scanline[0][G] = fgetc(file);
        scanline[0][B] = fgetc(file);
        scanline[0][E] = fgetc(file);
        if (feof(file))
            return false;

        if (scanline[0][R] == 1 &&
            scanline[0][G] == 1 &&
            scanline[0][B] == 1) {
            for (i = scanline[0][E] << rshift; i > 0; i--) {
                memcpy(&scanline[0][0], &scanline[-1][0], 4);
                scanline++;
                len--;
            }
            rshift += 8;
        }
        else {
            scanline++;
            len--;
            rshift = 0;
        }
    }
    return true;
}
