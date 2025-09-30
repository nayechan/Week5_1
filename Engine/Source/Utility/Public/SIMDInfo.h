#pragma once
#include <cstdint>
#include <intrin.h>

/**
 * @brief SIMD 지원 여부를 체크하는 유틸리티 (Lazy Initialization)
 *
 * 첫 호출 시 CPUID를 통해 CPU의 SIMD 지원 여부를 확인하고 캐싱합니다.
 */
class FSIMDInfo
{
public:
	enum class ESIMDLevel : uint8_t
	{
		None = 0,
		SSE2,
		SSE4_1,
		AVX,
		AVX2
	};

	/**
	 * @brief Singleton 인스턴스 반환 (Lazy initialization)
	 */
	static const FSIMDInfo& Get()
	{
		static FSIMDInfo Instance;
		return Instance;
	}

	// SIMD 지원 여부 체크
	bool HasSSE2() const { return bHasSSE2; }
	bool HasSSE41() const { return bHasSSE41; }
	bool HasAVX() const { return bHasAVX; }
	bool HasAVX2() const { return bHasAVX2; }

	ESIMDLevel GetMaxSIMDLevel() const { return MaxLevel; }

private:
	bool bHasSSE2 = false;
	bool bHasSSE41 = false;
	bool bHasAVX = false;
	bool bHasAVX2 = false;
	ESIMDLevel MaxLevel = ESIMDLevel::None;

	// Private constructor - CPUID 체크
	FSIMDInfo()
	{
		DetectSIMDSupport();
	}

	void DetectSIMDSupport()
	{
		int cpuInfo[4] = {0};

		// CPUID Function 1: Feature Information
		__cpuid(cpuInfo, 1);

		// ECX register
		const int ecx = cpuInfo[2];
		const int edx = cpuInfo[3];

		// SSE2 check (bit 26 in EDX)
		bHasSSE2 = (edx & (1 << 26)) != 0;
		if (bHasSSE2)
			MaxLevel = ESIMDLevel::SSE2;

		// SSE4.1 check (bit 19 in ECX)
		bHasSSE41 = (ecx & (1 << 19)) != 0;
		if (bHasSSE41)
			MaxLevel = ESIMDLevel::SSE4_1;

		// AVX check (bit 28 in ECX)
		bHasAVX = (ecx & (1 << 28)) != 0;
		if (bHasAVX)
			MaxLevel = ESIMDLevel::AVX;

		// CPUID Function 7: Extended Features
		__cpuidex(cpuInfo, 7, 0);
		const int ebx = cpuInfo[1];

		// AVX2 check (bit 5 in EBX)
		bHasAVX2 = bHasAVX && ((ebx & (1 << 5)) != 0);
		if (bHasAVX2)
			MaxLevel = ESIMDLevel::AVX2;
	}

	// Prevent copying
	FSIMDInfo(const FSIMDInfo&) = delete;
	FSIMDInfo& operator=(const FSIMDInfo&) = delete;
};