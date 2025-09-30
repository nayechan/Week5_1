#include "pch.h"
#include "UtilIty/Public/ThreadStats.h"
#include "UtilIty/Public/ScopeCycleCounter.h" // FPlatformTime::ToMilliseconds
#include <unordered_map>

namespace
{
	struct FStatRecord
	{
		uint64_t LastCycles = 0;
		uint64_t TotalCycles = 0;
		uint32_t Count = 0;
	};

	// 싱글스레드 가정
	TMap<const void*, FStatRecord> GStats;
}

void FThreadStats::AddMessage(const TStatId& StatId, EStatOperation Op, uint64_t Cycles)
{
	if (Op != EStatOperation::Add || StatId.Id == nullptr)
	{
		return;
	}

	auto& Rec = GStats[StatId.Id];
	Rec.LastCycles = Cycles;
	Rec.TotalCycles += Cycles;
	Rec.Count += 1;
}

double FThreadStats::GetLastMilliseconds(const TStatId& StatId)
{
	if (StatId.Id == nullptr)
	{
		return 0.0;
	}

	auto It = GStats.find(StatId.Id);
	if (It == GStats.end())
	{
		return 0.0;
	}

	return FPlatformTime::ToMilliseconds(It->second.LastCycles);
}

double FThreadStats::GetTotalMilliseconds(const TStatId& StatId)
{
	if (StatId.Id == nullptr)
	{
		return 0.0;
	}

	auto It = GStats.find(StatId.Id);
	if (It == GStats.end())
	{
		return 0.0;
	}

	return FPlatformTime::ToMilliseconds(It->second.TotalCycles);
}

double FThreadStats::GetAverageMilliseconds(const TStatId& StatId)
{
	if (StatId.Id == nullptr)
	{
		return 0.0;
	}

	auto It = GStats.find(StatId.Id);
	if (It == GStats.end() || It->second.Count == 0)
	{
		return 0.0;
	}

	const uint64_t AvgCycles = It->second.TotalCycles / It->second.Count;
	return FPlatformTime::ToMilliseconds(AvgCycles);
}

uint32_t FThreadStats::GetCount(const TStatId& StatId)
{
	if (StatId.Id == nullptr)
	{
		return 0;
	}

	auto It = GStats.find(StatId.Id);
	if (It == GStats.end())
	{
		return 0;
	}

	return It->second.Count;
}
