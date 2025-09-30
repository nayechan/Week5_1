#include "pch.h"
#include "Utility/Public/ScopeCycleCounter.h"	
#include "Utility/Public/ThreadStats.h" // FThreadStats, EStatOperation

// 정적 멤버 정의
double FWindowsPlatformTime::GSecondsPerCycle = 0.0;
bool   FWindowsPlatformTime::bInitialized = false;

void FWindowsPlatformTime::InitTiming()
{
	if (!bInitialized)
	{
		bInitialized = true;

		double Frequency = static_cast<double>(GetFrequency());
		if (Frequency <= 0.0)
		{
			Frequency = 1.0;
		}

		GSecondsPerCycle = 1.0 / Frequency;
	}
}

float FWindowsPlatformTime::GetSecondsPerCycle()
{
	if (!bInitialized)
	{
		InitTiming();
	}
	return static_cast<float>(GSecondsPerCycle);
}

uint64 FWindowsPlatformTime::GetFrequency()
{
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	return Frequency.QuadPart;
}

double FWindowsPlatformTime::ToMilliseconds(uint64 CycleDiff)
{
	const double Ms = static_cast<double>(CycleDiff)
		* GetSecondsPerCycle()
		* 1000.0;
	return Ms;
}

uint64 FWindowsPlatformTime::Cycles64()
{
	LARGE_INTEGER CycleCount;
	QueryPerformanceCounter(&CycleCount);
	return static_cast<uint64>(CycleCount.QuadPart);
}

/*---------------------------------*
 * 공용 태그 정의 및 StatId 생성 함수 *
 *---------------------------------*/

char GStat_Picking_Tag = 0;

TStatId GetPickingStatId()
{
	return TStatId(&GStat_Picking_Tag, FName("Picking"));
}

char GStat_PickPrimitive_Tag = 0;

TStatId GetPickPrimitiveStatId()
{
	return TStatId(&GStat_PickPrimitive_Tag, FName("PickPrimitive"));
}

char GStat_SceneBVHTraverse_Tag = 0;

TStatId GetSceneBVHTraverseStatId()
{
	return TStatId(&GStat_SceneBVHTraverse_Tag, FName("SceneBVHTraverse"));
}

char GStat_StaticMeshBVHTraverse_Tag = 0;

TStatId GetStaticMeshBVHTraverseStatId()
{
	return TStatId(&GStat_StaticMeshBVHTraverse_Tag, FName("StaticMeshBVHTraverse"));
}

char GStat_Culling_Tag = 0;

TStatId GetCullingStatId()
{
	return TStatId(&GStat_Culling_Tag, FName("Culling"));
}

char GStat_VisitTriangle_Tag = 0;

TStatId GetVisitTriangleStatId()
{
	return TStatId(&GStat_VisitTriangle_Tag, FName("VisitTriangle"));
}

/*---------------------------------*/

// FScopeCycleCounter
FScopeCycleCounter::FScopeCycleCounter(TStatId StatId)
	: StartCycles(FPlatformTime::Cycles64())
	, UsedStatId(StatId)
{
}

FScopeCycleCounter::~FScopeCycleCounter()
{
	Finish();
}

uint64 FScopeCycleCounter::Finish()
{
	const uint64 EndCycles = FPlatformTime::Cycles64();
	const uint64 CycleDiff = EndCycles - StartCycles;

	FThreadStats::AddMessage(UsedStatId, EStatOperation::Add, CycleDiff);

	return CycleDiff;
}
