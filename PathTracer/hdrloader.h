#pragma once
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
/***********************************************************************************
	Created:	17:9:2002
	FileName: 	hdrloader.h
	Author:		Igor Kravtchenko
	
	Info:		Load HDR image and convert to a set of float32 RGB triplet.
************************************************************************************/

/* 
	This is modified version of the original code. Addeed code to build marginal & conditional densities for IBL importance sampling
*/

class HDRLoaderResult {
public:
	HDRLoaderResult()
	{
		width = height = 0;
		cols = NULL;
	}
	int width, height;
	// each pixel takes 3 float32, each component can be of any value...
	float *cols;
	glm::vec2 *marginalDistData;    // y component holds the pdf
	glm::vec2 *conditionalDistData; // y component holds the pdf
};

class HDRLoader {
private:
	static void buildDistributions(HDRLoaderResult &res);
public:
	static bool load(const char *fileName, HDRLoaderResult &res);
};

