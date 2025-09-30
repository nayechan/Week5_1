#pragma once

class FWindowsPlatformTime
{
public:
	static double GSecondsPerCycle; // 0
	static bool bInitialized;       // false

	static void   InitTiming();
	static float  GetSecondsPerCycle();
	static uint64 GetFrequency();
	static double ToMilliseconds(uint64 CycleDiff);
	static uint64 Cycles64();
};

struct TStatId
{
	// 식별용 포인터 (주소값으로 구분)
	const void* Id = nullptr;
	FName Name = FName::GetNone();

	TStatId() = default;
	TStatId(const void* InId, FName InName) : Id(InId), Name(InName) {}
};


/*---------------------------------*
 * 공용 태그 정의 및 StatId 생성 함수 *
 *---------------------------------*/

extern char GStat_Picking_Tag;
TStatId GetPickingStatId();
extern char GStat_PickPrimitive_Tag;
TStatId GetPickPrimitiveStatId();
extern char GStat_SceneBVHTraverse_Tag;
TStatId GetSceneBVHTraverseStatId();
extern char GStat_StaticMeshBVHTraverse_Tag;
TStatId GetStaticMeshBVHTraverseStatId();
extern char GStat_Culling_Tag;
TStatId GetCullingStatId();
extern char GStat_VisitTriangle_Tag;
TStatId GetVisitTriangleStatId();

/*---------------------------------*
 *        통계 활성화 매크로         *
 *---------------------------------*/

// BVH 통계 활성화 매크로
// #define ENABLE_BVH_STATS

/*---------------------------------*/

typedef FWindowsPlatformTime FPlatformTime;

class FScopeCycleCounter
{
public:
	FScopeCycleCounter(TStatId StatId);
	~FScopeCycleCounter();

	uint64 Finish();

private:
	uint64 StartCycles;
	TStatId UsedStatId;
};
