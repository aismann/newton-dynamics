/* Copyright (c) <2003-2019> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _D_CLASS_ALLOC_H_
#define _D_CLASS_ALLOC_H_

#include "dCoreStdafx.h"

class dClassAlloc  
{
	public:
	dClassAlloc()
	{
	}

	~dClassAlloc() 
	{
	}

	void *operator new (size_t size)
	{
		return Malloc(size);
	}

	void operator delete (void* ptr)
	{
		Free(ptr);
	}

	D_CORE_API static void* Malloc(size_t size);
	D_CORE_API static void Free(void* const ptr);
};

template<class T, dInt32 size>
class ndFixSizeBuffer: public dClassAlloc
{
	public:
	ndFixSizeBuffer()
		:dClassAlloc()
	{
	}

	T& operator[] (dInt32 i)
	{
		dAssert(i >= 0);
		dAssert(i < size);
		return m_array[i];
	}

	const T& operator[] (dInt32 i) const
	{
		dAssert(i >= 0);
		dAssert(i < size);
		return m_array[i];
	}

	T m_array[size];
};

#endif