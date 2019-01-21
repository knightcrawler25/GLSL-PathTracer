/*
*  Copyright (c) 2009-2011, NVIDIA Corporation
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*      * Redistributions of source code must retain the above copyright
*        notice, this list of conditions and the following disclaimer.
*      * Redistributions in binary form must reproduce the above copyright
*        notice, this list of conditions and the following disclaimer in the
*        documentation and/or other materials provided with the distribution.
*      * Neither the name of NVIDIA Corporation nor the
*        names of its contributors may be used to endorse or promote products
*        derived from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
*  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
*  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
*  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "linear_math.h"
#include <string>
#include <ctime>


#define FW_F32_MIN          (1.175494351e-38f)
#define FW_F32_MAX          (3.402823466e+38f)
#define NULL 0
#define FW_ASSERT(X) ((void)0)  

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long U64;
typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
typedef signed long S64;
typedef float F32;
typedef double F64;

inline F32          bitsToFloat(U32 a)         { return *(F32*)&a; }
inline U32          floatToBits(F32 a)         { return *(U32*)&a; }

inline int max1i(const int& a, const int& b){ return (a < b) ? b : a; }
inline int min1i(const int& a, const int& b){ return (a > b) ? b : a; }
inline float clamp(float x){ return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }
inline int toInt(float x){ return int(pow(clamp(x), 1 / 2.2) * 255 + .5); }

	//------------------------------------------------------------------------

struct Clock {
	unsigned firstValue;
	Clock() { reset(); }
	void reset() { firstValue = clock(); }
	unsigned readMS() { return (clock() - firstValue) / (CLOCKS_PER_SEC / 1000); }
};
  
unsigned int WangHash(unsigned int a);

class Platform
{
public:
	Platform() { m_name = std::string("Default"); m_SAHNodeCost = 1.f; m_SAHTriangleCost = 1.f; m_nodeBatchSize = 1; m_triBatchSize = 1; m_minLeafSize = 1; m_maxLeafSize = 0x7FFFFFF; } /// leafsize = aantal tris
	Platform(const std::string& name, float nodeCost = 1.f, float triCost = 1.f, S32 nodeBatchSize = 1, S32 triBatchSize = 1) { m_name = name; m_SAHNodeCost = nodeCost; m_SAHTriangleCost = triCost; m_nodeBatchSize = nodeBatchSize; m_triBatchSize = triBatchSize; m_minLeafSize = 1; m_maxLeafSize = 0x7FFFFFF; }

	const std::string&   getName() const                { return m_name; }

	// SAH weights
	float getSAHTriangleCost() const                    { return m_SAHTriangleCost; }
	float getSAHNodeCost() const                        { return m_SAHNodeCost; }

	// SAH costs, raw and batched
	float getCost(int numChildNodes, int numTris) const { return getNodeCost(numChildNodes) + getTriangleCost(numTris); }
	float getTriangleCost(S32 n) const                  { return roundToTriangleBatchSize(n) * m_SAHTriangleCost; }
	float getNodeCost(S32 n) const                      { return roundToNodeBatchSize(n) * m_SAHNodeCost; }

	// batch processing (how many ops at the price of one)
	S32   getTriangleBatchSize() const                  { return m_triBatchSize; }
	S32   getNodeBatchSize() const                      { return m_nodeBatchSize; }
	void  setTriangleBatchSize(S32 triBatchSize)        { m_triBatchSize = triBatchSize; }
	void  setNodeBatchSize(S32 nodeBatchSize)           { m_nodeBatchSize = nodeBatchSize; }
	S32   roundToTriangleBatchSize(S32 n) const         { return ((n + m_triBatchSize - 1) / m_triBatchSize)*m_triBatchSize; }
	S32   roundToNodeBatchSize(S32 n) const             { return ((n + m_nodeBatchSize - 1) / m_nodeBatchSize)*m_nodeBatchSize; }

	// leaf preferences
	void  setLeafPreferences(S32 minSize, S32 maxSize)   { m_minLeafSize = minSize; m_maxLeafSize = maxSize; }
	S32   getMinLeafSize() const                        { return m_minLeafSize; }
	S32   getMaxLeafSize() const                        { return m_maxLeafSize; }

private:
	std::string  m_name;
	float   m_SAHNodeCost;
	float   m_SAHTriangleCost;
	S32     m_triBatchSize;
	S32     m_nodeBatchSize;
	S32     m_minLeafSize;
	S32     m_maxLeafSize;
};

