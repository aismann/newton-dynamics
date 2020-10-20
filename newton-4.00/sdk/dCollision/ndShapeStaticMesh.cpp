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
#include "ndCollisionStdafx.h"
#include "ndShapeStaticMesh.h"

//#include "dgPhysicsStdafx.h"
//#include "dgBody.h"
//#include "dgWorld.h"
//#include "dgContact.h"
//#include "ndShapeStaticMesh.h"
//#include "dgCollisionConvexPolygon.h"

#if 0

dgPolygonMeshDesc::dgPolygonMeshDesc(dgCollisionParamProxy& proxy, void* const userData)
	:dFastAABBInfo()
	,m_boxDistanceTravelInMeshSpace(dFloat32 (0.0f))
	,m_threadNumber(proxy.m_threadIndex)
	,m_faceCount(0)
	,m_vertexStrideInBytes(0)
	,m_skinThickness(proxy.m_skinThickness)
	,m_userData (userData)
	,m_objBody (proxy.m_body0)
	,m_polySoupBody(proxy.m_body1)
	,m_convexInstance(proxy.m_instance0)
	,m_polySoupInstance(proxy.m_instance1)
	,m_vertex(NULL)
	,m_faceIndexCount(NULL)
	,m_faceVertexIndex(NULL)
	,m_faceIndexStart(NULL)
	,m_hitDistance(NULL)
	,m_maxT(dFloat32 (1.0f))
	,m_doContinuesCollisionTest(proxy.m_continueCollision)
{
	dgAssert (m_polySoupInstance->IsType (dgCollision::dgCollisionMesh_RTTI));
	dgAssert (m_convexInstance->IsType (dgCollision::dgCollisionConvexShape_RTTI));

	const dMatrix& hullMatrix = m_convexInstance->GetGlobalMatrix();
	const dMatrix& soupMatrix = m_polySoupInstance->GetGlobalMatrix();

	dMatrix& matrix = *this;
	matrix = hullMatrix * soupMatrix.Inverse();
	dMatrix convexMatrix (dGetIdentityMatrix());

	switch (m_polySoupInstance->GetScaleType())
	{
		case dgCollisionInstance::m_unit:
		{
			break;
		}

		case dgCollisionInstance::m_uniform:
		{
			const dVector& invScale = m_polySoupInstance->GetInvScale();
			convexMatrix[0][0] = invScale.GetScalar();
			convexMatrix[1][1] = invScale.GetScalar();
			convexMatrix[2][2] = invScale.GetScalar();
			matrix.m_posit = matrix.m_posit * (invScale | dVector::m_wOne);
			break;
		}

		case dgCollisionInstance::m_nonUniform:
		{
			const dVector& invScale = m_polySoupInstance->GetInvScale();
			dMatrix tmp (matrix[0] * invScale, matrix[1] * invScale, matrix[2] * invScale, dVector::m_wOne);
			convexMatrix = tmp * matrix.Inverse();
			convexMatrix.m_posit = dVector::m_wOne;
			matrix.m_posit = matrix.m_posit * (invScale | dVector::m_wOne);
			break;
		}

		case dgCollisionInstance::m_global:
		default:
		{
		   dgAssert (0);
		}
	}

	dMatrix fullMatrix (convexMatrix * matrix);
	m_convexInstance->CalcAABB(fullMatrix, m_p0, m_p1);

	dVector p0;
	dVector p1;
	SetTransposeAbsMatrix(matrix);
	m_convexInstance->CalcAABB(convexMatrix, p0, p1);
	m_size = dVector::m_half * (p1 - p0);
	m_posit = matrix.TransformVector(dVector::m_half * (p1 + p0));
	dgAssert (m_posit.m_w == dFloat32 (1.0f));
}

void dgPolygonMeshDesc::SortFaceArray ()
{
	dInt32 stride = 8;
	if (m_faceCount >= 8) {
		dInt32 stack[DG_MAX_COLLIDING_FACES][2];

		stack[0][0] = 0;
		stack[0][1] = m_faceCount - 1;
		dInt32 stackIndex = 1;
		while (stackIndex) {
			stackIndex --;
			dInt32 lo = stack[stackIndex][0];
			dInt32 hi = stack[stackIndex][1];
			if ((hi - lo) > stride) {
				dInt32 i = lo;
				dInt32 j = hi;
				dFloat32 dist = m_hitDistance[(lo + hi) >> 1];
				do {    
					while (m_hitDistance[i] < dist) i ++;
					while (m_hitDistance[j] > dist) j --;

					if (i <= j)	{
						dgSwap (m_hitDistance[i], m_hitDistance[j]);
						dgSwap (m_faceIndexStart[i], m_faceIndexStart[j]);
						dgSwap (m_faceIndexCount[i], m_faceIndexCount[j]);
						i++; 
						j--;
					}
				} while (i <= j);

				if (i < hi) {
					stack[stackIndex][0] = i;
					stack[stackIndex][1] = hi;
					stackIndex ++;
				}
				if (lo < j) {
					stack[stackIndex][0] = lo;
					stack[stackIndex][1] = j;
					stackIndex ++;
				}
				dgAssert (stackIndex < dInt32 (sizeof (stack) / (2 * sizeof (stack[0][0]))));
			}
		}
	}

	stride = stride * 2;
	if (m_faceCount < stride) {
		stride = m_faceCount;
	}
	for (dInt32 i = 1; i < stride; i ++) {
		if (m_hitDistance[i] < m_hitDistance[0]) {
			dgSwap (m_hitDistance[i], m_hitDistance[0]);
			dgSwap (m_faceIndexStart[i], m_faceIndexStart[0]);
			dgSwap (m_faceIndexCount[i], m_faceIndexCount[0]);
		}
	}

	for (dInt32 i = 1; i < m_faceCount; i ++) {
		dInt32 j = i;
		dInt32 ptr = m_faceIndexStart[i];
		dInt32 count = m_faceIndexCount[i];
		dFloat32 dist = m_hitDistance[i];
		for ( ; dist < m_hitDistance[j - 1]; j --) {
			dgAssert (j > 0);
			m_hitDistance[j] = m_hitDistance [j-1];
			m_faceIndexStart[j] = m_faceIndexStart[j-1];
			m_faceIndexCount[j] = m_faceIndexCount[j-1];
		}
		m_hitDistance[j] = dist;
		m_faceIndexStart[j] = ptr;
		m_faceIndexCount[j] = count;
	}

#ifdef _DEBUG
	for (dInt32 i = 0; i < m_faceCount - 1; i ++) {
		dgAssert (m_hitDistance[i] <= m_hitDistance[i+1]);
	}
#endif
}


ndShapeStaticMesh::ndShapeStaticMesh(dgWorld* const world, dgCollisionID type)
	:dgCollision(world->GetAllocator(), 0, type)
{
	m_rtti |= dgCollisionMesh_RTTI;
	m_debugCallback = NULL;
	SetCollisionBBox (dVector (dFloat32 (0.0f)), dVector (dFloat32 (0.0f)));
}

ndShapeStaticMesh::ndShapeStaticMesh (dgWorld* const world, dgDeserialize deserialization, void* const userData, dInt32 revisionNumber)
	:dgCollision(world, deserialization, userData, revisionNumber)
{
	dgAssert (m_rtti | dgCollisionMesh_RTTI);

	m_debugCallback = NULL;
	SetCollisionBBox (dVector (dFloat32 (0.0f)), dVector (dFloat32 (0.0f)));
}

ndShapeStaticMesh::~ndShapeStaticMesh()
{
}

void ndShapeStaticMesh::SetCollisionBBox (const dVector& p0, const dVector& p1)
{
	dgAssert (p0.m_x <= p1.m_x);
	dgAssert (p0.m_y <= p1.m_y);
	dgAssert (p0.m_z <= p1.m_z);

	m_boxSize = (p1 - p0).Scale (dFloat32 (0.5f)) & dVector::m_triplexMask;
	m_boxOrigin = (p1 + p0).Scale (dFloat32 (0.5f)) & dVector::m_triplexMask; 
}

dInt32 ndShapeStaticMesh::CalculateSignature () const
{
	dgAssert (0);
	return 0;
}


dInt32 ndShapeStaticMesh::CalculatePlaneIntersection (const dVector& normal, const dVector& point, dVector* const contactsOut) const
{
	return 0;
}

void ndShapeStaticMesh::SetDebugCollisionCallback (dgCollisionMeshCollisionCallback debugCallback)
{
	m_debugCallback = debugCallback;
}




#ifdef DG_DEBUG_AABB
dVector ndShapeStaticMesh::BoxSupportMapping  (const dVector& dir) const
{
	return dVector (dir.m_x < dFloat32 (0.0f) ? m_p0.m_x : m_p1.m_x, 
					 dir.m_y < dFloat32 (0.0f) ? m_p0.m_y : m_p1.m_y, 
					 dir.m_z < dFloat32 (0.0f) ? m_p0.m_z : m_p1.m_z, dFloat32 (0.0f));
}
#endif




dVector ndShapeStaticMesh::CalculateVolumeIntegral (const dMatrix& globalMatrix, const dVector& plane, const dgCollisionInstance& parentScale) const
{
	return dVector (dFloat32 (0.0f));
}


void ndShapeStaticMesh::DebugCollision (const dMatrix& matrixPtr, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	dgAssert (0);
}


dFloat32 ndShapeStaticMesh::GetVolume () const
{
//	dgAssert (0);
	return dFloat32 (0.0f); 
}

dFloat32 ndShapeStaticMesh::GetBoxMinRadius () const
{
	return dFloat32 (0.0f);  
}

dFloat32 ndShapeStaticMesh::GetBoxMaxRadius () const
{
	return dFloat32 (0.0f);  
}



void ndShapeStaticMesh::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgAssert (0);
	dgCollision::GetCollisionInfo(info);
//	info->m_offsetMatrix = GetLocalMatrix();
}

void ndShapeStaticMesh::Serialize(dgSerialize callback, void* const userData) const
{
	dgAssert (0);
}

dVector ndShapeStaticMesh::SupportVertex (const dVector& dir, dInt32* const vertexIndex) const
{
	dgAssert (0);
	return dVector (0, 0, 0, 0);
}


void ndShapeStaticMesh::CalcAABB(const dMatrix& matrix, dVector &p0, dVector &p1) const
{
	dVector origin (matrix.TransformVector(m_boxOrigin));
	dVector size (matrix.m_front.Abs().Scale(m_boxSize.m_x) + matrix.m_up.Abs().Scale(m_boxSize.m_y) + matrix.m_right.Abs().Scale(m_boxSize.m_z));

	p0 = (origin - size) & dVector::m_triplexMask;
	p1 = (origin + size) & dVector::m_triplexMask;
}


dInt32 ndShapeStaticMesh::CalculatePlaneIntersection (const dFloat32* const vertex, const dInt32* const index, dInt32 indexCount, dInt32 stride, const dgPlane& localPlane, dVector* const contactsOut) const
{
	dInt32 count = 0;
	dInt32 j = index[indexCount - 1] * stride;
	dVector p0 (&vertex[j]);
	p0 = p0 & dVector::m_triplexMask;
	dFloat32 side0 = localPlane.Evalue (p0);
	for (dInt32 i = 0; i < indexCount; i ++) {
		j = index[i] * stride;
		dVector p1 (&vertex[j]);
		p1 = p1 & dVector::m_triplexMask;
		dFloat32 side1 = localPlane.Evalue (p1);

		if (side0 < dFloat32 (0.0f)) {
			if (side1 >= dFloat32 (0.0f)) {
				dVector dp (p1 - p0);
				dgAssert (dp.m_w == dFloat32 (0.0f));
				dFloat32 t = localPlane.DotProduct(dp).GetScalar();
				dgAssert (dgAbs (t) >= dFloat32 (0.0f));
				if (dgAbs (t) < dFloat32 (1.0e-8f)) {
					t = dgSign(t) * dFloat32 (1.0e-8f);	
				}
				dgAssert (0);
				contactsOut[count] = p0 - dp.Scale (side0 / t);
				count ++;

			} 
		} else if (side1 <= dFloat32 (0.0f)) {
			dVector dp (p1 - p0);
			dgAssert (dp.m_w == dFloat32 (0.0f));
			dFloat32 t = localPlane.DotProduct(dp).GetScalar();
			dgAssert (dgAbs (t) >= dFloat32 (0.0f));
			if (dgAbs (t) < dFloat32 (1.0e-8f)) {
				t = dgSign(t) * dFloat32 (1.0e-8f);	
			}
			dgAssert (0);
			contactsOut[count] = p0 - dp.Scale (side0 / t);
			count ++;
		}

		side0 = side1;
		p0 = p1;
	}

	return count;
}

#endif