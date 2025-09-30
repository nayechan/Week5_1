#include "pch.h"
#include "Physics/Public/AABB.h"
#include <immintrin.h>  // SSE/AVX 지원
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2

/**
 * @brief Checks if this AABB completely contains another AABB (SIMD 최적화).
 */
bool FAABB::Contains(const FAABB& Other) const
{
	// SIMD를 사용한 최적화
	__m128 min1 = _mm_set_ps(0.0f, Min.Z, Min.Y, Min.X);
	__m128 max1 = _mm_set_ps(0.0f, Max.Z, Max.Y, Max.X);
	__m128 min2 = _mm_set_ps(0.0f, Other.Min.Z, Other.Min.Y, Other.Min.X);
	__m128 max2 = _mm_set_ps(0.0f, Other.Max.Z, Other.Max.Y, Other.Max.X);
	
	// min1 <= min2 && max1 >= max2 테스트 (완전히 포함)
	__m128 test1 = _mm_cmple_ps(min1, min2);  // min1 <= min2
	__m128 test2 = _mm_cmpge_ps(max1, max2);  // max1 >= max2
	__m128 result = _mm_and_ps(test1, test2);
	
	// 3개 축 모두 통과해야 완전히 포함
	int mask = _mm_movemask_ps(result);
	return (mask & 0x7) == 0x7;
}

/**
 * @brief Checks if this AABB intersects with another AABB (SIMD 최적화).
 */
bool FAABB::Intersects(const FAABB& Other) const
{
	// SIMD를 사용한 최적화 - 3개 축을 한 번에 처리
	__m128 min1 = _mm_set_ps(0.0f, Min.Z, Min.Y, Min.X);
	__m128 max1 = _mm_set_ps(0.0f, Max.Z, Max.Y, Max.X);
	__m128 min2 = _mm_set_ps(0.0f, Other.Min.Z, Other.Min.Y, Other.Min.X);
	__m128 max2 = _mm_set_ps(0.0f, Other.Max.Z, Other.Max.Y, Other.Max.X);
	
	// min1 <= max2 && max1 >= min2 테스트
	__m128 test1 = _mm_cmple_ps(min1, max2);  // min1 <= max2
	__m128 test2 = _mm_cmpge_ps(max1, min2);  // max1 >= min2
	__m128 result = _mm_and_ps(test1, test2);
	
	// 3개 축 모두 통과해야 충돌
	int mask = _mm_movemask_ps(result);
	return (mask & 0x7) == 0x7;  // 하위 3비트가 모두 1이면 충돌
}
