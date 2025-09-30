#pragma once
#include "Global/Types.h"
#include "Utility/Public/SIMDInfo.h"
#include <immintrin.h>  // AVX2/SSE intrinsics
#include <cstring>

/**
 * @brief SIMD 기반 Radix Sort 유틸리티
 *
 * 64비트 키를 기준으로 구조체 배열을 정렬합니다.
 * AVX2 명령어를 사용하여 히스토그램 생성을 가속화합니다.
 */
namespace RadixSortUtil
{
	/**
	 * @brief 8비트 단위로 Radix Sort를 수행합니다.
	 *
	 * @tparam T 정렬할 구조체 타입
	 * @tparam KeyExtractor 키 추출 함수 타입 (T -> uint64)
	 * @param InOutData 정렬할 데이터 배열
	 * @param Count 배열 크기
	 * @param GetKey 키 추출 람다/함수
	 */
	template<typename T, typename KeyExtractor>
	void RadixSort64(TArray<T>& InOutData, KeyExtractor&& GetKey)
	{
		const uint32 Count = static_cast<uint32>(InOutData.size());
		if (Count <= 1) return;

		// 임시 버퍼 (ping-pong)
		TArray<T> TempBuffer(Count);

		// 8비트씩 8번 패스 (64비트 키)
		constexpr int RADIX_BITS = 8;
		constexpr int RADIX_SIZE = 1 << RADIX_BITS;  // 256
		constexpr int NUM_PASSES = sizeof(uint64) / (RADIX_BITS / 8);  // 8

		T* Source = InOutData.data();
		T* Dest = TempBuffer.data();

		for (int Pass = 0; Pass < NUM_PASSES; ++Pass)
		{
			const int ShiftAmount = Pass * RADIX_BITS;

			// Step 1: 히스토그램 생성 (SIMD 가속)
			uint32 Histogram[RADIX_SIZE] = {0};
			BuildHistogramSIMD(Source, Count, Histogram, ShiftAmount, GetKey);

			// Step 2: Prefix Sum (누적 합)
			uint32 PrefixSum[RADIX_SIZE];
			PrefixSum[0] = 0;
			for (int i = 1; i < RADIX_SIZE; ++i)
			{
				PrefixSum[i] = PrefixSum[i - 1] + Histogram[i - 1];
			}

			// Step 3: 재배치
			for (uint32 i = 0; i < Count; ++i)
			{
				uint64 Key = GetKey(Source[i]);
				uint8 Digit = (Key >> ShiftAmount) & 0xFF;
				Dest[PrefixSum[Digit]++] = Source[i];
			}

			// Ping-pong swap
			T* Temp = Source;
			Source = Dest;
			Dest = Temp;
		}

		// 최종 결과가 Source에 있음
		// 홀수 번 swap되었으므로 Source가 TempBuffer를 가리킴
		if (NUM_PASSES % 2 == 1)
		{
			// TempBuffer -> InOutData로 복사
			std::memcpy(InOutData.data(), TempBuffer.data(), Count * sizeof(T));
		}
	}

	/**
	 * @brief AVX2를 사용하여 히스토그램을 생성합니다 (8개씩 병렬 처리)
	 */
	template<typename T, typename KeyExtractor>
	inline void BuildHistogramAVX2(const T* Data, uint32 Count, uint32* Histogram,
	                                int ShiftAmount, KeyExtractor&& GetKey)
	{
		constexpr uint32 SIMD_WIDTH = 8;
		const uint32 AlignedCount = (Count / SIMD_WIDTH) * SIMD_WIDTH;

		const __m256i ShiftVec = _mm256_set1_epi64x(ShiftAmount);
		const __m256i MaskVec = _mm256_set1_epi64x(0xFF);

		for (uint32 i = 0; i < AlignedCount; i += SIMD_WIDTH)
		{
			// 8개의 키를 가져옴
			uint64 Keys[SIMD_WIDTH];
			for (uint32 j = 0; j < SIMD_WIDTH; ++j)
			{
				Keys[j] = GetKey(Data[i + j]);
			}

			// 첫 4개 처리
			__m256i Keys0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&Keys[0]));
			__m256i Shifted0 = _mm256_srlv_epi64(Keys0, ShiftVec);
			__m256i Digits0 = _mm256_and_si256(Shifted0, MaskVec);

			// 나머지 4개 처리
			__m256i Keys1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&Keys[4]));
			__m256i Shifted1 = _mm256_srlv_epi64(Keys1, ShiftVec);
			__m256i Digits1 = _mm256_and_si256(Shifted1, MaskVec);

			// 히스토그램 증가
			alignas(32) uint64 DigitArray[SIMD_WIDTH];
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(&DigitArray[0]), Digits0);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(&DigitArray[4]), Digits1);

			for (uint32 j = 0; j < SIMD_WIDTH; ++j)
			{
				++Histogram[DigitArray[j]];
			}
		}

		// 나머지 처리
		for (uint32 i = AlignedCount; i < Count; ++i)
		{
			uint64 Key = GetKey(Data[i]);
			uint8 Digit = (Key >> ShiftAmount) & 0xFF;
			++Histogram[Digit];
		}
	}

	/**
	 * @brief SSE2를 사용하여 히스토그램을 생성합니다 (4개씩 병렬 처리)
	 */
	template<typename T, typename KeyExtractor>
	inline void BuildHistogramSSE2(const T* Data, uint32 Count, uint32* Histogram,
	                                int ShiftAmount, KeyExtractor&& GetKey)
	{
		constexpr uint32 SIMD_WIDTH = 4;
		const uint32 AlignedCount = (Count / SIMD_WIDTH) * SIMD_WIDTH;

		for (uint32 i = 0; i < AlignedCount; i += SIMD_WIDTH)
		{
			// 4개의 키를 가져와서 처리
			uint64 Keys[SIMD_WIDTH];
			for (uint32 j = 0; j < SIMD_WIDTH; ++j)
			{
				Keys[j] = GetKey(Data[i + j]);
			}

			// SSE2로 2개씩 처리 (128비트 = 64비트 * 2)
			__m128i Keys0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&Keys[0]));
			__m128i Keys1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&Keys[2]));

			// Shift and mask (스칼라 방식 - SSE2에는 가변 시프트 없음)
			alignas(16) uint64 KeyArray0[2];
			alignas(16) uint64 KeyArray1[2];
			_mm_storeu_si128(reinterpret_cast<__m128i*>(KeyArray0), Keys0);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(KeyArray1), Keys1);

			for (uint32 j = 0; j < 2; ++j)
			{
				uint8 Digit0 = (KeyArray0[j] >> ShiftAmount) & 0xFF;
				uint8 Digit1 = (KeyArray1[j] >> ShiftAmount) & 0xFF;
				++Histogram[Digit0];
				++Histogram[Digit1];
			}
		}

		// 나머지 처리
		for (uint32 i = AlignedCount; i < Count; ++i)
		{
			uint64 Key = GetKey(Data[i]);
			uint8 Digit = (Key >> ShiftAmount) & 0xFF;
			++Histogram[Digit];
		}
	}

	/**
	 * @brief 스칼라 방식으로 히스토그램을 생성합니다 (Fallback)
	 */
	template<typename T, typename KeyExtractor>
	inline void BuildHistogramScalar(const T* Data, uint32 Count, uint32* Histogram,
	                                  int ShiftAmount, KeyExtractor&& GetKey)
	{
		for (uint32 i = 0; i < Count; ++i)
		{
			uint64 Key = GetKey(Data[i]);
			uint8 Digit = (Key >> ShiftAmount) & 0xFF;
			++Histogram[Digit];
		}
	}

	/**
	 * @brief SIMD를 사용하여 히스토그램을 생성합니다 (런타임 분기)
	 */
	template<typename T, typename KeyExtractor>
	inline void BuildHistogramSIMD(const T* Data, uint32 Count, uint32* Histogram,
	                                int ShiftAmount, KeyExtractor&& GetKey)
	{
		const FSIMDInfo& SIMDInfo = FSIMDInfo::Get();

		if (SIMDInfo.HasAVX2())
		{
			BuildHistogramAVX2(Data, Count, Histogram, ShiftAmount, GetKey);
		}
		else if (SIMDInfo.HasSSE2())
		{
			BuildHistogramSSE2(Data, Count, Histogram, ShiftAmount, GetKey);
		}
		else
		{
			BuildHistogramScalar(Data, Count, Histogram, ShiftAmount, GetKey);
		}
	}

	/**
	 * @brief SIMD 최적화된 Radix Sort (FRenderCommand 특화 버전)
	 *
	 * SortKey가 구조체에 직접 저장되어 있어 더 빠릅니다.
	 */
	template<typename T>
	void RadixSort64Direct(TArray<T>& InOutData)
	{
		RadixSort64(InOutData, [](const T& Item) -> uint64 { return Item.SortKey; });
	}
}