#include "pch.h"
#include "Global/Quaternion.h"
#include <immintrin.h>  // SSE/AVX 지원
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2

FQuaternion FQuaternion::FromAxisAngle(const FVector& Axis, float AngleRad)
{
	FVector N = Axis;
	N.Normalize();
	float s = sinf(AngleRad * 0.5f);
	return FQuaternion(
		N.X * s,
		N.Y * s,
		N.Z * s,
		cosf(AngleRad * 0.5f)
	);
}

FQuaternion FQuaternion::FromEuler(const FVector& EulerDeg)
{
	FVector Radians = FVector::GetDegreeToRadian(EulerDeg);

	float cx = cosf(Radians.X * 0.5f);
	float sx = sinf(Radians.X * 0.5f);
	float cy = cosf(Radians.Y * 0.5f);
	float sy = sinf(Radians.Y * 0.5f);
	float cz = cosf(Radians.Z * 0.5f);
	float sz = sinf(Radians.Z * 0.5f);

	// Yaw-Pitch-Roll (Z, Y, X)
	return FQuaternion(
		sx * cy * cz - cx * sy * sz, // X
		cx * sy * cz + sx * cy * sz, // Y
		cx * cy * sz - sx * sy * cz, // Z
		cx * cy * cz + sx * sy * sz  // W
	);
}

FVector FQuaternion::ToEuler() const
{
	FVector Euler;

	// Roll (X)
	float sinr_cosp = 2.0f * (W * X + Y * Z);
	float cosr_cosp = 1.0f - 2.0f * (X * X + Y * Y);
	Euler.X = atan2f(sinr_cosp, cosr_cosp);

	// Pitch (Y)
	float sinp = 2.0f * (W * Y - Z * X);
	if (fabs(sinp) >= 1)
		Euler.Y = copysignf(PI / 2, sinp); // 90도 고정
	else
		Euler.Y = asinf(sinp);

	// Yaw (Z)
	float siny_cosp = 2.0f * (W * Z + X * Y);
	float cosy_cosp = 1.0f - 2.0f * (Y * Y + Z * Z);
	Euler.Z = atan2f(siny_cosp, cosy_cosp);

	return FVector::GetRadianToDegree(Euler);
}

FQuaternion FQuaternion::operator*(const FQuaternion& Q) const
{
	return FQuaternion(
		W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y,
		W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X,
		W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
		W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z
	);
}

/**
 * @brief Quaternion 정규화 (SIMD 최적화)
 */
void FQuaternion::Normalize()
{
	// SIMD를 사용한 최적화
	__m128 q = _mm_loadu_ps(&X);  // X, Y, Z, W 로드 (unaligned load로 안전성 확보)
	__m128 squared = _mm_mul_ps(q, q);
	__m128 sum = _mm_hadd_ps(squared, squared);
	sum = _mm_hadd_ps(sum, sum);
	
	// 길이가 0에 가까우면 정규화 방지
	float lengthSq = _mm_cvtss_f32(sum);
	if (lengthSq > 0.00000001f)
	{
		__m128 invLength = _mm_rsqrt_ss(sum);  // 떠른소수점 역제곱근 근사
		invLength = _mm_shuffle_ps(invLength, invLength, _MM_SHUFFLE(0, 0, 0, 0));
		q = _mm_mul_ps(q, invLength);
		
		// 결과를 다시 할당
		_mm_storeu_ps(&X, q);
	}
}

FVector FQuaternion::RotateVector(const FQuaternion& q, const FVector& v)
{
	FQuaternion p(v.X, v.Y, v.Z, 0.0f);
	FQuaternion r = q * p * q.Inverse();
	return FVector(r.X, r.Y, r.Z);
}

FVector FQuaternion::RotateVector(const FVector& v) const
{
	FQuaternion p(v.X, v.Y, v.Z, 0.0f);
	FQuaternion r = (*this) * p * this->Inverse();
	return FVector(r.X, r.Y, r.Z);
}
