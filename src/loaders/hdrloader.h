#pragma once
#include <iostream>
#include <Vec2.h>
#include <Vec3.h>
/***********************************************************************************
    Created:    17:9:2002
    FileName:     hdrloader.h
    Author:        Igor Kravtchenko
    
    Info:        Load HDR image and convert to a set of float32 RGB triplet.
************************************************************************************/

/* 
    This is a modified version of the original code. Addeed code to build marginal & conditional densities for IBL importance sampling
*/

using namespace GLSLPT;

class HDRData {
public:
    HDRData() : width(0), height(0), cols(nullptr), marginalDistData(nullptr), conditionalDistData(nullptr) {}
    ~HDRData() { delete cols; delete marginalDistData; delete conditionalDistData; }
    int width, height;
    // each pixel takes 3 float32, each component can be of any value...
    float *cols;
    Vec2 *marginalDistData;    // y component holds the pdf
    Vec2 *conditionalDistData; // y component holds the pdf
};

class HDRLoader {
private:
    static void buildDistributions(HDRData* res);
public:
    static HDRData* load(const char *fileName);
};