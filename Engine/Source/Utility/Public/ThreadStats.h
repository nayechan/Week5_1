#pragma once

#include "Utility/Public/ScopeCycleCounter.h"

// 간단한 Stat 연산
enum class EStatOperation : uint8_t
{
	Add
};

// 전역 통계 수집기
class FThreadStats
{
public:
	static void AddMessage(const TStatId& StatId, EStatOperation Op, uint64_t Cycles);
	static double GetLastMilliseconds(const TStatId& StatId);
	static double GetTotalMilliseconds(const TStatId& StatId);
	static double GetAverageMilliseconds(const TStatId& StatId);
	static uint32_t GetCount(const TStatId& StatId);
};
