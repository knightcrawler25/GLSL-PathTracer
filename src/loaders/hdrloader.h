
/***********************************************************************************
    Created:    17:9:2002
    FileName:     hdrloader.h
    Author:        Igor Kravtchenko

    Info:        Load HDR image and convert to a set of float32 RGB triplet.
************************************************************************************/

/*
    This is a modified version of the original code. Added code for importance sampling
*/

#pragma once

#include <iostream>
#include <Vec3.h>
#include <Vec2.h>

using namespace GLSLPT;

class HDRData {
public:
    HDRData() : width(0), height(0), cols(nullptr), marginalDistData(nullptr), conditionalDistData(nullptr) {}
    ~HDRData() { delete[] cols; delete marginalDistData; delete conditionalDistData; }
    int width, height;
    // each pixel takes 3 float32, each component can be of any value...
    float *cols;
    Vec2* marginalDistData;
    Vec2* conditionalDistData;
};

class HDRLoader {
private:
    static void buildDistributions(HDRData* res);
public:
    static HDRData* load(const char *fileName);
};