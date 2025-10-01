#include "pch.h"
#include "Render/Spatial/Public/Frustum.h"
#include <immintrin.h>  // SSE/AVX 지원
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2

void FFrustum::ConstructFromViewProjectionMatrix(const FMatrix& ViewProjMatrix)
{
	// Extract planes from combined matrix
	// Common form: (row 3 ± row X)
	// Note: OpenGL uses -1~1 clip space, DirectX uses 0~1.

	// Left plane: W + X
	Planes[Left].Normal.X = ViewProjMatrix.Data[0][3] + ViewProjMatrix.Data[0][0];
	Planes[Left].Normal.Y = ViewProjMatrix.Data[1][3] + ViewProjMatrix.Data[1][0];
	Planes[Left].Normal.Z = ViewProjMatrix.Data[2][3] + ViewProjMatrix.Data[2][0];
	Planes[Left].Distance = ViewProjMatrix.Data[3][3] + ViewProjMatrix.Data[3][0];
	Planes[Left].Normalize();

	// Right plane: W - X
	Planes[Right].Normal.X = ViewProjMatrix.Data[0][3] - ViewProjMatrix.Data[0][0];
	Planes[Right].Normal.Y = ViewProjMatrix.Data[1][3] - ViewProjMatrix.Data[1][0];
	Planes[Right].Normal.Z = ViewProjMatrix.Data[2][3] - ViewProjMatrix.Data[2][0];
	Planes[Right].Distance = ViewProjMatrix.Data[3][3] - ViewProjMatrix.Data[3][0];
	Planes[Right].Normalize();

	// Bottom plane: W + Y
	Planes[Bottom].Normal.X = ViewProjMatrix.Data[0][3] + ViewProjMatrix.Data[0][1];
	Planes[Bottom].Normal.Y = ViewProjMatrix.Data[1][3] + ViewProjMatrix.Data[1][1];
	Planes[Bottom].Normal.Z = ViewProjMatrix.Data[2][3] + ViewProjMatrix.Data[2][1];
	Planes[Bottom].Distance = ViewProjMatrix.Data[3][3] + ViewProjMatrix.Data[3][1];
	Planes[Bottom].Normalize();

	// Top plane: W - Y
	Planes[Top].Normal.X = ViewProjMatrix.Data[0][3] - ViewProjMatrix.Data[0][1];
	Planes[Top].Normal.Y = ViewProjMatrix.Data[1][3] - ViewProjMatrix.Data[1][1];
	Planes[Top].Normal.Z = ViewProjMatrix.Data[2][3] - ViewProjMatrix.Data[2][1];
	Planes[Top].Distance = ViewProjMatrix.Data[3][3] - ViewProjMatrix.Data[3][1];
	Planes[Top].Normalize();

	// Near plane: Z (DirectX 0~1 Range)
	Planes[Near].Normal.X = ViewProjMatrix.Data[0][2];
	Planes[Near].Normal.Y = ViewProjMatrix.Data[1][2];
	Planes[Near].Normal.Z = ViewProjMatrix.Data[2][2];
	Planes[Near].Distance = ViewProjMatrix.Data[3][2];
	Planes[Near].Normalize();

	// Far plane: W - Z
	Planes[Far].Normal.X = ViewProjMatrix.Data[0][3] - ViewProjMatrix.Data[0][2];
	Planes[Far].Normal.Y = ViewProjMatrix.Data[1][3] - ViewProjMatrix.Data[1][2];
	Planes[Far].Normal.Z = ViewProjMatrix.Data[2][3] - ViewProjMatrix.Data[2][2];
	Planes[Far].Distance = ViewProjMatrix.Data[3][3] - ViewProjMatrix.Data[3][2];
	Planes[Far].Normalize();
}

/* AABB test against frustum (SIMD 최적화) */
bool FFrustum::IsBoxInFrustum(const FAABB& Box) const
{
	// SIMD로 한번에 여러 평면 테스트 최적화
	__m128 boxMin = _mm_set_ps(0.0f, Box.Min.Z, Box.Min.Y, Box.Min.X);
	__m128 boxMax = _mm_set_ps(0.0f, Box.Max.Z, Box.Max.Y, Box.Max.X);
	__m128 epsilon = _mm_set1_ps(-FRUSTUM_EPSILON);

	for (int i = 0; i < PlaneCount; ++i)
	{
		const FPlane& Plane = Planes[i];

		// 평면 노멀 벡터
		__m128 normal = _mm_set_ps(0.0f, Plane.Normal.Z, Plane.Normal.Y, Plane.Normal.X);
		__m128 distance = _mm_set1_ps(Plane.Distance);

		// 평면 노멀이 양수인지 확인하여 가장 가까운 점 선택
		__m128 zero = _mm_setzero_ps();
		__m128 normalPositive = _mm_cmpgt_ps(normal, zero);

		// 조건에 따라 boxMin 또는 boxMax 선택
		__m128 closestPoint = _mm_blendv_ps(boxMin, boxMax, normalPositive);

		// 내적 계산
		__m128 dot = _mm_mul_ps(normal, closestPoint);
		__m128 sum = _mm_hadd_ps(dot, dot);
		sum = _mm_hadd_ps(sum, sum);

		// 평면과의 거리 계산
		__m128 planeDistance = _mm_add_ss(sum, distance);

		// 컬링 테스트
		if (_mm_comilt_ss(planeDistance, epsilon))
			return false;	// culled
	}
	return true;
}

/* point test */
bool FFrustum::IsPointInFrustum(const FVector& Point) const
{
	for (int i = 0; i < PlaneCount; ++i)
	{
		if (Planes[i].GetDistanceToPoint(Point) < 0.0f)
			return false;
	}
	return true;
}

/* sphere test */
bool FFrustum::IsSphereInFrustum(const FVector& Center, float Radius) const
{
	for (int i = 0; i < PlaneCount; ++i)
	{
		if (Planes[i].GetDistanceToPoint(Center) < -Radius)
			return false;
	}
	return true;
}
