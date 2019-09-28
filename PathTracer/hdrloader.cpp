
/***********************************************************************************
	Created:	17:9:2002
	FileName: 	hdrloader.cpp
	Author:		Igor Kravtchenko
	
	Info:		Load HDR image and convert to a set of float32 RGB triplet.
************************************************************************************/

/* 
	This is modified version of the original code. Addeed code to build marginal & conditional densities for IBL importance sampling
*/

#include "hdrloader.h"

#include <math.h>
#include <memory.h>
#include <stdio.h>

typedef unsigned char RGBE[4];
#define R			0
#define G			1
#define B			2
#define E			3

#define  MINELEN	8				// minimum scanline length for encoding
#define  MAXELEN	0x7fff			// maximum scanline length for encoding

static void workOnRGBE(RGBE *scan, int len, float *cols);
static bool decrunch(RGBE *scanline, int len, FILE *file);
static bool oldDecrunch(RGBE *scanline, int len, FILE *file);

float Luminance(const glm::vec3 &c)
{
	return c.x*0.3f + c.y*0.6f + c.z*0.1f;
}

int LowerBound(const float* array, int lower, int upper, const float value)
{
	while (lower < upper)
	{
		int mid = lower + (upper - lower) / 2;

		if (array[mid] < value)
		{
			lower = mid + 1;
		}
		else
		{
			upper = mid;
		}
	}

	return lower;
}

void HDRLoader::buildDistributions(HDRData* res)
{
	int width  = res->width;
	int height = res->height;

	float *pdf2D = new float[width*height];
	float *cdf2D = new float[width*height];

	float *pdf1D = new float[height];
	float *cdf1D = new float[height];

	res->marginalDistData    = new glm::vec2[height];
	res->conditionalDistData = new glm::vec2[width*height];

	float colWeightSum = 0.0f;

	for (int j = 0; j < height; j++)
	{
		float rowWeightSum = 0.0f;

		for (int i = 0; i < width; ++i)
		{
			float weight = Luminance(glm::vec3(res->cols[j*width * 3 + i * 3 + 0], res->cols[j*width * 3 + i * 3 + 1], res->cols[j*width * 3 + i * 3 + 2]));

			rowWeightSum += weight;

			pdf2D[j*width + i] = weight;
			cdf2D[j*width + i] = rowWeightSum;
		}

		/* Convert to range 0,1 */
		for (int i = 0; i < width; i++)
		{
			pdf2D[j*width + i] /= rowWeightSum;
			cdf2D[j*width + i] /= rowWeightSum;
		}

		colWeightSum += rowWeightSum;

		pdf1D[j] = rowWeightSum;
		cdf1D[j] = colWeightSum;
	}
	
	/* Convert to range 0,1 */
	for (int j = 0; j < height; j++)
	{
		cdf1D[j] /= colWeightSum;
		pdf1D[j] /= colWeightSum;
	}

	/* Precalculate row and col to avoid binary search during lookup in the shader */
	for (int i = 0; i < height; i++)
	{
		float invHeight = (float)(i+1) / height;
		int row = LowerBound(cdf1D, 0, height, invHeight);
		res->marginalDistData[i].x = row / (float)height;
		res->marginalDistData[i].y = pdf1D[i];
	}

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			float invWidth = (float)(i+1) / width;
			int col = LowerBound(cdf2D, j*width, (j + 1)*width, invWidth) - j * width;
			res->conditionalDistData[j*width + i].x = col / (float)width;
			res->conditionalDistData[j*width + i].y = pdf2D[j*width + i];
		}
	}

	delete[] pdf2D;
	delete[] pdf1D;
	delete[] cdf2D;
	delete[] cdf1D;
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
		return false;
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
	if (!sscanf(reso, "-Y %ld +X %ld", &h, &w)) {
		fclose(file);
		return false;
	}

	res->width = w;
	res->height = h;

	float *cols = new float[w * h * 3];
	res->cols = cols;

	RGBE *scanline = new RGBE[w];
	if (!scanline) {
		fclose(file);
		return false;
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
			else  {	// non-run
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
