#pragma once
#include <vector>
#include "Global/Types.h"
#include "Physics/Public/AABB.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Utility/Public/ScopeCycleCounter.h"

class FSceneBVH
{
public:
	// 인풋: 씬의 Primitive 포인터 목록 (월드 AABB는 UPrimitiveComponent에서 가져옴)
	void Build(const TArray<UPrimitiveComponent*>& InPrimitives);

	// Ray와 교차 가능한 후보 프리미티브를 outCandidates에 추가 (교차 '가능성'만 필터)
	void QueryRay(const FRay& Ray, TArray<UPrimitiveComponent*>& OutCandidates) const;

	// 전체 리핏 (바텀업)
	void Refit();

	// Dirty-only 리핏 (프리미티브 인덱스 집합)
	void RefitDirty(const TArray<int64>& DirtyPrimIndices);

	// Dirty-only 리핏 (프리미티브 포인터 집합)
	void RefitDirtyByPrims(const TArray<UPrimitiveComponent*>& DirtyPrims);

	void Clear();

	// front-to-back 트래버설 중 조기 가지치기 + 리프에서 정밀 검사 수행 (람다 버전)
	// - InOutBestDist: 초기에는 +inf, 정밀 히트가 갱신하면 더 작은 값으로 업데이트됨
	// - PreciseTest: (Primitive, PrimIndex, PrimAABB_TMin, inOutHitDist) -> true/false, true이면 inOutHitDist에 거리 기록
	// - 반환: 히트 유무
	template<typename TVisitor>
	bool TraverseFrontToBackFirstHit(const FRay& Ray,
		float& InOutBestDist,
		UPrimitiveComponent*& OutHit,
		TVisitor&& PreciseTest) const
	{
#ifdef ENABLE_BVH_STATS
		FScopeCycleCounter Counter(GetSceneBVHTraverseStatId());
#endif

		OutHit = nullptr;
		if (Nodes.empty()) return false;

		FRayCached Cached{ { Ray.Origin.X, Ray.Origin.Y, Ray.Origin.Z }, { 0,0,0 } };
		Cached.InvDir[0] = (fabsf(Ray.Direction.X) > 1e-8f) ? 1.0f / Ray.Direction.X : std::numeric_limits<float>::infinity();
		Cached.InvDir[1] = (fabsf(Ray.Direction.Y) > 1e-8f) ? 1.0f / Ray.Direction.Y : std::numeric_limits<float>::infinity();
		Cached.InvDir[2] = (fabsf(Ray.Direction.Z) > 1e-8f) ? 1.0f / Ray.Direction.Z : std::numeric_limits<float>::infinity();

		struct FEntry { int64 Index; float TMin; };
		FEntry Stack[256];
		int32 SP = 0;

		float RootTMin, RootTMax;
		if (!RayIntersectsAABB(Cached, Nodes[0].Bounds, RootTMin, RootTMax)) return false;
		Stack[SP++] = { 0, RootTMin };

		while (SP > 0)
		{
			const FEntry Entry = Stack[--SP];
			if (Entry.TMin > InOutBestDist) continue;

			const FNode& N = Nodes[Entry.Index];
			if (N.IsLeaf())
			{
				for (int64 i = 0; i < N.Count; ++i)
				{
					const int64 PrimIdx = Indices[N.Start + i];

					float PTMin, PTMax;
					if (!RayIntersectsAABB(Cached, PrimBounds[PrimIdx], PTMin, PTMax)) continue;
					if (PTMin > InOutBestDist) continue;

					UPrimitiveComponent* Prim = Primitives[PrimIdx];
					float HitDist = InOutBestDist;
					// new call: pass PrimIdx and PTMin to the precise test
					if (PreciseTest(Prim, PrimIdx, PTMin, HitDist) && HitDist < InOutBestDist)
					{
						InOutBestDist = HitDist;
						OutHit = Prim;
					}
				}
			}
			else
			{
				float LTMin, LTMax, RTMin, RTMax;
				const bool bHitL = (N.Left >= 0) && RayIntersectsAABB(Cached, Nodes[N.Left].Bounds, LTMin, LTMax) && LTMin <= InOutBestDist;
				const bool bHitR = (N.Right >= 0) && RayIntersectsAABB(Cached, Nodes[N.Right].Bounds, RTMin, RTMax) && RTMin <= InOutBestDist;

				if (bHitL & bHitR)
				{
					// 먼 쪽 먼저 push -> 가까운 쪽이 다음에 먼저 팝됨
					if (LTMin < RTMin) { Stack[SP++] = { N.Right, RTMin }; Stack[SP++] = { N.Left, LTMin }; }
					else { Stack[SP++] = { N.Left, LTMin };  Stack[SP++] = { N.Right, RTMin }; }
				}
				else if (bHitL) { Stack[SP++] = { N.Left,  LTMin }; }
				else if (bHitR) { Stack[SP++] = { N.Right, RTMin }; }
			}
		}
		return OutHit != nullptr;
	}

	size_t GetPrimitiveCount() const { return Primitives.size(); }
	bool GetPrimBounds(UPrimitiveComponent * Prim, FAABB & OutBounds) const;

	bool GetPrimSphereByIndex(int64 PrimIdx, FVector& OutCenter, float& OutRadiusSq) const;

private:
	struct FNode
	{
		FAABB Bounds;
		int64 Left = -1;
		int64 Right = -1;
		int64 Start = 0;   // leaf: Primitives[start..start+count)
		int64 Count = 0;
		bool  IsLeaf() const { return Count > 0; }
	};

	// 내부 저장
	TArray<FNode> Nodes;
	TArray<UPrimitiveComponent*> Primitives; // leaf에 인덱스 저장을 위한 배열
	TArray<int64> Indices;                   // Primitives 인덱스
	TArray<FAABB> PrimBounds;                // 각 Primitive의 월드 AABB

	// Dirty-only 리핏 지원용 캐시
	TArray<int64> Parent;                    // 각 노드의 부모 인덱스 (루트는 -1)
	TArray<int64> PrimToLeaf;                // 각 프리미티브가 속한 leaf 노드 인덱스
	TMap<UPrimitiveComponent*, int64> PrimIndexMap; // 포인터→인덱스

	// SAH 보조: AABB 표면적
	static float SurfaceArea(const FAABB& B);

	int64 BuildRecursive(int64* IndexPtr, int64 Count);

	struct FRayCached
	{
		float Origin[3];
		float InvDir[3];
	};

	bool RayIntersectsAABB(const FRayCached& Ray, const FAABB& Box) const;

	// TMin/TMax까지 계산해 반환
	bool RayIntersectsAABB(const FRayCached& Ray, const FAABB& Box, float& OutTMin, float& OutTMax) const;

	// 바텀업 리핏
	void RefitNode(int64 NodeIndex);

	// 부분 리핏 보조
	void RefitLeafBounds(int64 LeafNodeIndex);
	void RefitInternalBounds(int64 NodeIndex);

	// Structure-of-Arrays caches for cache-friendly access (hot paths) ---
	mutable TArray<float> PrimMinX;
	mutable TArray<float> PrimMinY;
	mutable TArray<float> PrimMinZ;
	mutable TArray<float> PrimMaxX;
	mutable TArray<float> PrimMaxY;
	mutable TArray<float> PrimMaxZ;
	mutable TArray<float> PrimCentX;
	mutable TArray<float> PrimCentY;
	mutable TArray<float> PrimCentZ;
	mutable TArray<float> PrimSphereRadiusSq;

	// Ensure SoA arrays are sized to PrimBounds.size()
	void EnsurePrimSoASize() const;

	// Hot-path primitive-index based AABB slab test (uses SoA arrays) - faster than materializing FAABB
	bool RayIntersectsAABB_PrimIndex(const FRayCached& Ray, int64 PrimIdx, float& OutTMin, float& OutTMax) const;

	// Compute union bounds of PrimBounds[IndexPtr[0..Count-1]] into OutMin/OutMax
	void ComputeBoundsForRange(const int64* IndexPtr, int64 Count, FVector& OutMin, FVector& OutMax) const;

	// Compute centroid bounds (min/max of primitive centroids) for IndexPtr range
	void ComputeCentroidBoundsForRange(const int64* IndexPtr, int64 Count, FVector& OutCentMin, FVector& OutCentMax) const;

	// Initialize a leaf node at NodeIndex using IndexPtr range
	void MakeLeafNode(int64 NodeIndex, const FVector& BMin, const FVector& BMax, int64* IndexPtr, int64 Count);

	// Find best split using binned SAH, returns (BestAxis, BestSplitBin, BestCost).
	// If no valid split found, BestAxis will be -1.
	void FindBestSplit(const int64* IndexPtr, int64 Count, const FVector& CentMin, const FVector& CentMax, int& OutBestAxis, int& OutBestSplitBin, float& OutBestCost) const;

	// Internal AABB intersection that returns TMin/TMax if hit.
	bool RayIntersectsAABBInternal(const FRayCached& Ray, const FAABB& Box, float& OutTMin, float& OutTMax) const;

	// Node bounds SoA (cache-friendly) - mutable because traversal is const
	mutable TArray<float> NodeMinX;
	mutable TArray<float> NodeMinY;
	mutable TArray<float> NodeMinZ;
	mutable TArray<float> NodeMaxX;
	mutable TArray<float> NodeMaxY;
	mutable TArray<float> NodeMaxZ;

	// Ensure Node SoA arrays sized
	void EnsureNodeSoASize() const;

	// Build Node SoA from Nodes[].Bounds (call once after Build and after refit phases)
	void BuildNodeSoA() const;

	// Hot-path node-index based slab test using Node SoA
	bool RayIntersectsAABB_NodeIndex(const FRayCached& Ray, int64 NodeIdx, float& OutTMin, float& OutTMax) const;
};
