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

using namespace GLSLPT;

class HDRData {
public:
    HDRData() : width(0), height(0), cols(nullptr), lookupData(nullptr) {}
    ~HDRData() { delete[] cols; delete[] lookupData; }
    int width, height;
    // each pixel takes 3 float32, each component can be of any value...
    float *cols;
    Vec3 *lookupData;
};

class HDRLoader {
private:
    static void buildDistributions(HDRData* res);
public:
    static HDRData* load(const char *fileName);
};