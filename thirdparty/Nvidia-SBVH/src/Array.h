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
#include <cmath>
#include "linear_math.h"
#include <string.h>
#include "Util.h"


template <class T> class Array
{
private:
	enum
	{
		MinBytes = 32,
	};

public:
	Array(void)                          { init(); }
	explicit        Array(const T& item)                 { init(); add(item); }
	Array(const T* ptr, int size)        { init(); set(ptr, size); }
	Array(const Array<T>& other)         { init(); set(other); }
	~Array(void)                          { delete[] m_ptr; }

	int             getSize(void) const                    { return m_size; }
	const T&        get(int idx) const                 { FW_ASSERT(idx >= 0 && idx < m_size); return m_ptr[idx]; }
	T&              get(int idx)                       { FW_ASSERT(idx >= 0 && idx < m_size); return m_ptr[idx]; }
	T               set(int idx, const T& item)        { T& slot = get(idx); T old = slot; slot = item; return old; }
	const T&        getFirst(void) const                    { return get(0); }
	T&              getFirst(void)                          { return get(0); }
	const T&        getLast(void) const                    { return get(getSize() - 1); }
	T&              getLast(void)                          { return get(getSize() - 1); }
	const T*        getPtr(int idx = 0) const             { FW_ASSERT(idx >= 0 && idx <= m_size); return m_ptr + idx; }
	T*              getPtr(int idx = 0)                   { FW_ASSERT(idx >= 0 && idx <= m_size); return m_ptr + idx; }

	int             getStride(void) const                    { return sizeof(T); }
	int             getNumBytes(void) const                    { return getSize() * getStride(); }

	void            reset(int size = 0)                  { clear(); setCapacity(size); m_size = size; }
	void            clear(void)                          { m_size = 0; }
	void            resize(int size);
	void            setCapacity(int capacity)                  { int c = max1i(capacity, m_size); if (m_alloc != c) realloc(c); }
	void            compact(void)                          { setCapacity(0); }

	void            set(const T* ptr, int size)        { reset(size); if (ptr) copy(getPtr(), ptr, size); }
	void            set(const Array<T>& other)         { if (&other != this) set(other.getPtr(), other.getSize()); }
	void            setRange(int start, int end, const T* ptr) { FW_ASSERT(end <= m_size); copy(getPtr(start), ptr, end - start); }
	void            setRange(int start, const Array<T>& other) { setRange(start, start + other.getSize(), other.getPtr()); }
	Array<T>        getRange(int start, int end) const      { FW_ASSERT(end <= m_size); return Array<T>(getPtr(start), end - start); }

	T&              add(void)                          { return *add(NULL, 1); }
	T&              add(const T& item)                 { T* slot = add(NULL, 1); *slot = item; return *slot; }
	T*              add(const T* ptr, int size)        { int oldSize = getSize(); resize(oldSize + size); T* slot = getPtr(oldSize); if (ptr) copy(slot, ptr, size); return slot; }
	T*              add(const Array<T>& other)         { return replace(getSize(), getSize(), other); }

	T&              insert(int idx)                       { return *replace(idx, idx, 1); }
	T&              insert(int idx, const T& item)        { T* slot = replace(idx, idx, 1); *slot = item; return *slot; }
	T*              insert(int idx, const T* ptr, int size) { return replace(idx, idx, ptr, size); }
	T*              insert(int idx, const Array<T>& other) { return replace(idx, idx, other); }

	T               remove(int idx)                       { T old = get(idx); replace(idx, idx + 1, 0); return old; }
	void            remove(int start, int end)            { replace(start, end, 0); }
	T&              removeLast(void)                          { FW_ASSERT(m_size > 0); m_size--; return m_ptr[m_size]; }
	T               removeSwap(int idx);
	void            removeSwap(int start, int end);

	T*              replace(int start, int end, int size);
	T*              replace(int start, int end, const T* ptr, int size) { T* slot = replace(start, end, size); if (ptr) copy(slot, ptr, size); return slot; }
	T*              replace(int start, int end, const Array<T>& other);

	int             indexOf(const T& item) const           { return indexOf(item, 0); }
	int             indexOf(const T& item, int fromIdx) const;
	int             lastIndexOf(const T& item) const           { return lastIndexOf(item, getSize() - 1); }
	int             lastIndexOf(const T& item, int fromIdx) const;
	bool            contains(const T& item) const           { return (indexOf(item) != -1); }
	bool            removeItem(const T& item)                 { int idx = indexOf(item); if (idx == -1) return false; remove(idx); return true; }

	const T&        operator[]  (int idx) const                 { return get(idx); }
	T&              operator[]  (int idx)                       { return get(idx); }
	Array<T>&       operator=   (const Array<T>& other)         { set(other); return *this; }
	bool            operator==  (const Array<T>& other) const;
	bool            operator!=  (const Array<T>& other) const   { return (!operator==(other)); }

	static void     copy(T* dst, const T* src, int size);
	static void     copyOverlap(T* dst, const T* src, int size);

private:
	void            init(void)                          { m_ptr = NULL; m_size = 0; m_alloc = 0; }
	void            realloc(int size);

private:
	T*              m_ptr;
	S32             m_size;
	S32             m_alloc;
};

//------------------------------------------------------------------------



template <class T> void Array<T>::resize(int size)
{
	FW_ASSERT(size >= 0);

	if (size > m_alloc)
	{
		int newAlloc = max1i((int)(MinBytes / sizeof(T)), 1);
		while (size > newAlloc)
			newAlloc <<= 1;
		realloc(newAlloc);
	}

	m_size = size;
}

//------------------------------------------------------------------------

template <class T> T Array<T>::removeSwap(int idx)
{
	FW_ASSERT(idx >= 0 && idx < m_size);

	T old = get(idx);
	m_size--;
	if (idx < m_size)
		m_ptr[idx] = m_ptr[m_size];
	return old;
}

//------------------------------------------------------------------------

template <class T> void Array<T>::removeSwap(int start, int end)
{
	FW_ASSERT(start >= 0);
	FW_ASSERT(start <= end);
	FW_ASSERT(end <= m_size);

	int oldSize = m_size;
	m_size += start - end;

	int copyStart = std::max(m_size, end);
	copy(m_ptr + start, m_ptr + copyStart, oldSize - copyStart);
}

//------------------------------------------------------------------------

template <class T> T* Array<T>::replace(int start, int end, int size)
{
	FW_ASSERT(start >= 0);
	FW_ASSERT(start <= end);
	FW_ASSERT(end <= m_size);
	FW_ASSERT(size >= 0);

	int tailSize = m_size - end;
	int newEnd = start + size;
	resize(m_size + newEnd - end);

	copyOverlap(m_ptr + newEnd, m_ptr + end, tailSize);
	return m_ptr + start;
}

//------------------------------------------------------------------------

template <class T> T* Array<T>::replace(int start, int end, const Array<T>& other)
{
	Array<T> tmp;
	const T* ptr = other.getPtr();
	if (&other == this)
	{
		tmp = other;
		ptr = tmp.getPtr();
	}
	return replace(start, end, ptr, other.getSize());
}

//------------------------------------------------------------------------

template <class T> int Array<T>::indexOf(const T& item, int fromIdx) const
{
	for (int i = max1i(fromIdx, 0); i < getSize(); i++)
		if (get(i) == item)
			return i;
	return -1;
}

//------------------------------------------------------------------------

template <class T> int Array<T>::lastIndexOf(const T& item, int fromIdx) const
{
	for (int i = min1i(fromIdx, getSize() - 1); i >= 0; i--)
		if (get(i) == item)
			return i;
	return -1;
}

//------------------------------------------------------------------------

template <class T> bool Array<T>::operator==(const Array<T>& other) const
{
	if (getSize() != other.getSize())
		return false;

	for (int i = 0; i < getSize(); i++)
		if (get(i) != other[i])
			return false;
	return true;
}

//------------------------------------------------------------------------

template <class T> void Array<T>::copy(T* dst, const T* src, int size)
{
	FW_ASSERT(size >= 0);
	if (!size)
		return;

	FW_ASSERT(dst && src);
	for (int i = 0; i < size; i++)
		dst[i] = src[i];
}

//------------------------------------------------------------------------

template <class T> void Array<T>::copyOverlap(T* dst, const T* src, int size)
{
	FW_ASSERT(size >= 0);
	if (!size)
		return;

	FW_ASSERT(dst && src);
	if (dst < src || dst >= src + size)
		for (int i = 0; i < size; i++)
			dst[i] = src[i];
	else
		for (int i = size - 1; i >= 0; i--)
			dst[i] = src[i];
}

//------------------------------------------------------------------------

template <class T> void Array<T>::realloc(int size)
{
	FW_ASSERT(size >= 0);

	T* newPtr = NULL;
	if (size)
	{
		newPtr = new T[size];
		copy(newPtr, m_ptr, min1i(size, m_size));
	}

	delete[] m_ptr;
	m_ptr = newPtr;
	m_alloc = size;
}

//------------------------------------------------------------------------

template <> inline void Array<S32>::copy(S32* dst, const S32* src, int size)        { memcpy(dst, src, size * sizeof(S32)); }
template <> inline void Array<U32>::copy(U32* dst, const U32* src, int size)        { memcpy(dst, src, size * sizeof(U32)); }
template <> inline void Array<F32>::copy(F32* dst, const F32* src, int size)        { memcpy(dst, src, size * sizeof(F32)); }

template <> inline void Array<Vec2f>::copy(Vec2f* dst, const Vec2f* src, int size)  { memcpy(dst, src, size * sizeof(Vec2f)); }
template <> inline void Array<Vec3f>::copy(Vec3f* dst, const Vec3f* src, int size)  { memcpy(dst, src, size * sizeof(Vec3f)); }
template <> inline void Array<Vec4f>::copy(Vec4f* dst, const Vec4f* src, int size)  { memcpy(dst, src, size * sizeof(Vec4f)); }

//------------------------------------------------------------------------
