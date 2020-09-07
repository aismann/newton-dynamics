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

#include "dCoreStdafx.h"
#include "dTypes.h"
#include "dMemory.h"

static dMemAllocCallback m_allocMemory = malloc;
static dMemFreeCallback m_freeMemory = free;

class dMemoryHeader
{
	public:
	union
	{
		char m_padd[32];
		struct
		{
			void* m_ptr;
			dInt32 m_size;
		};
	};
};

void* dMalloc(size_t size)
{
	size += 2 * sizeof(dMemoryHeader) - 1;
	void* const ptr = m_allocMemory(size);
	dInt64 val = dUnsigned64(ptr) + sizeof(dMemoryHeader) - 1;
	dInt64 mask = -dInt64(sizeof(dMemoryHeader));
	val = val & mask;
	dMemoryHeader* const ret = (dMemoryHeader*)val;
	ret->m_size = dInt32 (size);
	ret->m_ptr = ptr;
	return &ret[1];
}

void dFree(void* const ptr)
{
	dMemoryHeader* const ret = ((dMemoryHeader*)ptr) - 1;
	m_freeMemory(ret->m_ptr);
}

void dSetMemoryAllocators(dMemAllocCallback alloc, dMemFreeCallback free)
{
	m_allocMemory = alloc;
	m_freeMemory = free;
}