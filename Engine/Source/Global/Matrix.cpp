#include "pch.h"
#include <immintrin.h>  // SSE/AVX 지원
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2


FMatrix FMatrix::UEToDx = FMatrix(
	{
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

FMatrix FMatrix::DxToUE = FMatrix(
	{
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

/**
* @brief float 타입의 배열을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix()
	: Data{ {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0} }
{
}


/**
* @brief float 타입의 param을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix(
	float M00, float M01, float M02, float M03,
	float M10, float M11, float M12, float M13,
	float M20, float M21, float M22, float M23,
	float M30, float M31, float M32, float M33)
	: Data{ {M00,M01,M02,M03},
			{M10,M11,M12,M13},
			{M20,M21,M22,M23},
			{M30,M31,M32,M33} }
{
}

FMatrix::FMatrix(const FVector& x, const FVector& y, const FVector& z)
	:Data{ {x.X, x.Y, x.Z, 0.0f},
			{y.X, y.Y, y.Z, 0.0f},
			{z.X, z.Y, z.Z, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f} }
{
}

FMatrix::FMatrix(const FVector4& x, const FVector4& y, const FVector4& z)
	:Data{ {x.X,x.Y,x.Z, x.W},
			{y.X,y.Y,y.Z,y.W},
			{z.X,z.Y,z.Z,z.W},
			{0.0f, 0.0f, 0.0f, 1.0f} }
{
}

/**
* @brief 항등행렬
*/
FMatrix FMatrix::Identity()
{
	return FMatrix(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
}


/**
* @brief 두 행렬곱을 진행한 행렬을 반환하는 연산자 함수 (SIMD 최적화)
*/
FMatrix FMatrix::operator*(const FMatrix& InOtherMatrix) const
{
	FMatrix Result;

	// SIMD를 사용한 4x4 행렬 곱셈 최적화
	for (int32 i = 0; i < 4; ++i)
	{
		// 각 행을 SIMD 레지스터에 로드 (aligned load로 최대 성능 확보)
		__m128 row = _mm_load_ps(&Data[i][0]);
		
		for (int32 j = 0; j < 4; ++j)
		{
			// 다른 행렬의 열을 SIMD 레지스터에 로드
			__m128 col = _mm_set_ps(
				InOtherMatrix.Data[3][j],
				InOtherMatrix.Data[2][j],
				InOtherMatrix.Data[1][j],
				InOtherMatrix.Data[0][j]
			);
			
			// 벡터 내적 계산
			__m128 mul = _mm_mul_ps(row, col);
			__m128 sum = _mm_hadd_ps(mul, mul);
			sum = _mm_hadd_ps(sum, sum);
			
			Result.Data[i][j] = _mm_cvtss_f32(sum);
		}
	}

	return Result;
}

void FMatrix::operator*=(const FMatrix& InOtherMatrix)
{
	*this = (*this) * InOtherMatrix;
}

/**
* @brief 행렬에 스칼라를 곱한 새로운 행렬을 반환하는 연산자 함수 (SIMD 최적화)
*/
FMatrix FMatrix::operator*(float Scalar) const
{
	FMatrix result;
	__m128 scalarVec = _mm_set1_ps(Scalar);  // 스칼라 값을 4개 복사한 벡터 생성
	
	for (int i = 0; i < 4; ++i)
	{
		// 한 번에 한 행(4개 요소)을 처리 (aligned load로 최대 성능 확보)
		__m128 row = _mm_load_ps(&Data[i][0]);
		__m128 result_row = _mm_mul_ps(row, scalarVec);
		_mm_store_ps(&result.Data[i][0], result_row);
	}
	return result;
}

/**
* @brief 행렬에 스칼라를 곱한 결과를 자신에게 적용하는 연산자 함수 (SIMD 최적화)
*/
void FMatrix::operator*=(float Scalar)
{
	__m128 scalarVec = _mm_set1_ps(Scalar);  // 스칼라 값을 4개 복사한 벡터 생성
	
	for (int i = 0; i < 4; ++i)
	{
		// 한 번에 한 행(4개 요소)을 처리 (aligned load로 최대 성능 확보)
		__m128 row = _mm_load_ps(&Data[i][0]);
		__m128 result_row = _mm_mul_ps(row, scalarVec);
		_mm_store_ps(&Data[i][0], result_row);
	}
}

/**
* @brief 두 행렬합을 진행한 행렬을 반환하는 연산자 함수 (SIMD 최적화)
*/
FMatrix FMatrix::operator+(const FMatrix& InOtherMatrix) const
{
	FMatrix Result;
	for (int32 i = 0; i < 4; ++i)
	{
		// 한 번에 한 행(4개 요소)을 처리 (aligned load로 최대 성능 확보)
		__m128 row1 = _mm_load_ps(&Data[i][0]);
		__m128 row2 = _mm_load_ps(&InOtherMatrix.Data[i][0]);
		__m128 result_row = _mm_add_ps(row1, row2);
		_mm_store_ps(&Result.Data[i][0], result_row);
	}
	return Result;
}

/**
* @brief 두 행렬합을 진행한 결과를 자신에게 적용하는 연산자 함수 (SIMD 최적화)
*/
void FMatrix::operator+=(const FMatrix& InOtherMatrix)
{
	for (int32 i = 0; i < 4; ++i)
	{
		// 한 번에 한 행(4개 요소)을 처리 (aligned load로 최대 성능 확보)
		__m128 row1 = _mm_load_ps(&Data[i][0]);
		__m128 row2 = _mm_load_ps(&InOtherMatrix.Data[i][0]);
		__m128 result_row = _mm_add_ps(row1, row2);
		_mm_store_ps(&Data[i][0], result_row);
	}
}

/**
* @brief Position의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::TranslationMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = InOtherVector.X;
	Result.Data[3][1] = InOtherVector.Y;
	Result.Data[3][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

FMatrix FMatrix::TranslationMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = -InOtherVector.X;
	Result.Data[3][1] = -InOtherVector.Y;
	Result.Data[3][2] = -InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Scale의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::ScaleMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = InOtherVector.X;
	Result.Data[1][1] = InOtherVector.Y;
	Result.Data[2][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}
FMatrix FMatrix::ScaleMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = 1 / InOtherVector.X;
	Result.Data[1][1] = 1 / InOtherVector.Y;
	Result.Data[2][2] = 1 / InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Rotation의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::RotationMatrix(const FVector& InOtherVector)
{
	// Dx11 yaw(y), pitch(x), roll(z)
	// UE yaw(z), pitch(y), roll(x)
	// 회전 축이 바뀌어서 각 회전행렬 함수에 바뀐 값을 적용

	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;
	//return RotationZ(yaw) * RotationY(pitch) * RotationX(roll);
	//return RotationX(yaw) * RotationY(roll) * RotationZ(pitch);
	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::CreateFromYawPitchRoll(const float yaw, const float pitch, const float roll)
{
	//return RotationZ(yaw) * RotationY(pitch)* RotationX(roll);
	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::RotationMatrixInverse(const FVector& InOtherVector)
{
	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;

	return RotationZ(-roll) * RotationY(-yaw) * RotationX(-pitch);
}

/**
* @brief X의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationX(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[1][1] = C;
	Result.Data[1][2] = S;
	Result.Data[2][1] = -S;
	Result.Data[2][2] = C;

	return Result;
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationY(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][2] = -S;
	Result.Data[2][0] = S;
	Result.Data[2][2] = C;

	return Result;
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationZ(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][1] = S;
	Result.Data[1][0] = -S;
	Result.Data[1][1] = C;

	return Result;
}

//
FMatrix FMatrix::GetModelMatrix(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrix(Location);
	FMatrix R = RotationMatrix(Rotation);
	FMatrix S = ScaleMatrix(Scale);
	FMatrix modelMatrix = S * R * T;

	// Dx11 y-up 왼손좌표계에서 정의된 물체의 정점을 UE z-up 왼손좌표계로 변환
	return  FMatrix::UEToDx * modelMatrix;
}

FMatrix FMatrix::GetModelMatrixInverse(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrixInverse(Location);
	FMatrix R = RotationMatrixInverse(Rotation);
	FMatrix S = ScaleMatrixInverse(Scale);
	FMatrix modelMatrixInverse = T * R * S;

	// UE 좌표계로 변환된 물체의 정점을 원래의 Dx 11 왼손좌표계 정점으로 변환
	return modelMatrixInverse * FMatrix::DxToUE;
}

/**
* @brief 4D 벡터와 행렬의 곱셈 (SIMD 최적화)
*/
FVector4 FMatrix::VectorMultiply(const FVector4& v, const FMatrix& m)
{
	FVector4 result = {};
	
	// 벡터를 SIMD 레지스터에 로드
	__m128 vec = _mm_set_ps(v.W, v.Z, v.Y, v.X);  // W, Z, Y, X 순서로 로드
	
	// 각 행과 벡터의 내적을 계산
	for (int i = 0; i < 4; ++i)
	{
		__m128 row = _mm_load_ps(&m.Data[0][i]);  // 열을 행으로 로드 (전치된 방식)
		__m128 col = _mm_set_ps(m.Data[3][i], m.Data[2][i], m.Data[1][i], m.Data[0][i]);
		__m128 mul = _mm_mul_ps(vec, col);
		__m128 sum = _mm_hadd_ps(mul, mul);
		sum = _mm_hadd_ps(sum, sum);
		
		// 결과 저장
		((float*)&result)[i] = _mm_cvtss_f32(sum);
	}
	
	return result;
}

/**
* @brief 3D 벡터와 행렬의 곱셈 (SIMD 최적화)
*/
FVector FMatrix::VectorMultiply(const FVector& v, const FMatrix& m)
{
	FVector result = {};
	
	// 벡터를 SIMD 레지스터에 로드 (W=0으로 설정)
	__m128 vec = _mm_set_ps(0.0f, v.Z, v.Y, v.X);
	
	// 각 열과 벡터의 내적을 계산 (3D이므로 3개만)
	for (int i = 0; i < 3; ++i)
	{
		__m128 col = _mm_set_ps(0.0f, m.Data[2][i], m.Data[1][i], m.Data[0][i]);
		__m128 mul = _mm_mul_ps(vec, col);
		__m128 sum = _mm_hadd_ps(mul, mul);
		sum = _mm_hadd_ps(sum, sum);
		
		// 결과 저장
		((float*)&result)[i] = _mm_cvtss_f32(sum);
	}
	
	return result;
}

/**
* @brief 행렬 전치 (SIMD 최적화)
*/
FMatrix FMatrix::Transpose() const
{
	FMatrix result = {};
	
	// 4개 행을 SIMD 레지스터로 로드
	__m128 row0 = _mm_load_ps(&Data[0][0]);
	__m128 row1 = _mm_load_ps(&Data[1][0]);
	__m128 row2 = _mm_load_ps(&Data[2][0]);
	__m128 row3 = _mm_load_ps(&Data[3][0]);
	
	// 전치를 위한 SIMD 연산
	// 단계 1: 각 요소들을 섭계
	__m128 tmp0 = _mm_unpacklo_ps(row0, row1);  // [a00 a10 a01 a11]
	__m128 tmp1 = _mm_unpacklo_ps(row2, row3);  // [a20 a30 a21 a31]
	__m128 tmp2 = _mm_unpackhi_ps(row0, row1);  // [a02 a12 a03 a13]
	__m128 tmp3 = _mm_unpackhi_ps(row2, row3);  // [a22 a32 a23 a33]
	
	// 단계 2: 최종 전치 결과 생성
	__m128 col0 = _mm_movelh_ps(tmp0, tmp1);   // [a00 a10 a20 a30]
	__m128 col1 = _mm_movehl_ps(tmp1, tmp0);   // [a01 a11 a21 a31]
	__m128 col2 = _mm_movelh_ps(tmp2, tmp3);   // [a02 a12 a22 a32]
	__m128 col3 = _mm_movehl_ps(tmp3, tmp2);   // [a03 a13 a23 a33]
	
	// 결과 저장
	_mm_store_ps(&result.Data[0][0], col0);
	_mm_store_ps(&result.Data[1][0], col1);
	_mm_store_ps(&result.Data[2][0], col2);
	_mm_store_ps(&result.Data[3][0], col3);
	
	return result;
}
