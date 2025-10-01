#include "pch.h"
#include "Utility/Public/SceneBVH.h"
#include "Utility/Public/ScopeCycleCounter.h"

// FSceneBVH: 씬의 PrimitiveComponent 분할을 위한 SAH(binned) 기반 BVH
// - Build: 입력 프리미티브로 트리를 구성
// - Refit: 트리 토폴로지는 유지한 채 AABB만 갱신 (전체/Dirty 전파 지원)
// - QueryRay: Ray-AABB 테스트로 후보 프리미티브만 수집

void FSceneBVH::Clear()
{
	// 내부 모든 캐시/배열 초기화
	Nodes.clear();
	Primitives.clear();
	Indices.clear();
	PrimBounds.clear();
	Parent.clear();
	PrimToLeaf.clear();
	PrimIndexMap.clear();

	PrimMinX.clear(); PrimMinY.clear(); PrimMinZ.clear();
	PrimMaxX.clear(); PrimMaxY.clear(); PrimMaxZ.clear();
	PrimCentX.clear(); PrimCentY.clear(); PrimCentZ.clear();

	NodeMinX.clear(); NodeMinY.clear(); NodeMinZ.clear();
	NodeMaxX.clear(); NodeMaxY.clear(); NodeMaxZ.clear();
}

void FSceneBVH::Build(const TArray<UPrimitiveComponent*>& InPrimitives)
{
	// 초기화 + 입력 보관
	Clear();
	if (InPrimitives.empty()) return;

	// null 프리미티브 필터링
	Primitives.clear();
	Primitives.reserve(InPrimitives.size());
	for (auto* Prim : InPrimitives)
	{
		if (Prim != nullptr)
		{
			Primitives.push_back(Prim);
		}
	}

	const size_t NumPrimitives = Primitives.size();
	if (NumPrimitives == 0) return;

	// 인덱스/바운드/리프 매핑 준비
	Indices.resize(NumPrimitives);
	PrimBounds.resize(NumPrimitives);
	PrimToLeaf.assign(NumPrimitives, -1);

	// Ensure SoA arrays
	EnsurePrimSoASize();

	// 포인터 → 인덱스 매핑 생성 + 초기 PrimBounds 설정 (fill SoA)
	PrimIndexMap.reserve(NumPrimitives);
	for (size_t I = 0; I < NumPrimitives; ++I)
	{
		Indices[I] = static_cast<int64>(I);

		// Additional safety check: verify primitive pointer is valid
		if (!Primitives[I])
		{
			// Use zero bounds for invalid primitive
			PrimBounds[I] = FAABB(FVector::ZeroVector(), FVector::ZeroVector());
			PrimMinX[I] = PrimMinY[I] = PrimMinZ[I] = 0.0f;
			PrimMaxX[I] = PrimMaxY[I] = PrimMaxZ[I] = 0.0f;
			PrimCentX[I] = PrimCentY[I] = PrimCentZ[I] = 0.0f;
			PrimSphereRadiusSq[I] = 0.0f;
			continue;
		}

		PrimIndexMap[Primitives[I]] = static_cast<int64>(I);

		FVector MinW{}, MaxW{};
		try
		{
			Primitives[I]->GetWorldAABB(MinW, MaxW);
		}
		catch (...)
		{
			// 기본값으로 설정 (삭제된 객체에 접근한 경우)
			MinW = FVector::ZeroVector();
			MaxW = FVector::ZeroVector();
		}
		PrimBounds[I] = FAABB(MinW, MaxW);

		// write SoA
		PrimMinX[I] = MinW.X; PrimMinY[I] = MinW.Y; PrimMinZ[I] = MinW.Z;
		PrimMaxX[I] = MaxW.X; PrimMaxY[I] = MaxW.Y; PrimMaxZ[I] = MaxW.Z;
		PrimCentX[I] = (MinW.X + MaxW.X) * 0.5f;
		PrimCentY[I] = (MinW.Y + MaxW.Y) * 0.5f;
		PrimCentZ[I] = (MinW.Z + MaxW.Z) * 0.5f;
		// compute radius^2 using (Max - Cent) vector (no sqrt)
		{
			const float ex = PrimMaxX[I] - PrimCentX[I];
			const float ey = PrimMaxY[I] - PrimCentY[I];
			const float ez = PrimMaxZ[I] - PrimCentZ[I];
			PrimSphereRadiusSq[I] = ex * ex + ey * ey + ez * ez;
		}
	}

	// 노드/부모 캐시 예약 후 재귀 빌드 시작
	Nodes.reserve(NumPrimitives * 2);
	Parent.reserve(NumPrimitives * 2);

	BuildRecursive(Indices.data(), static_cast<int64>(NumPrimitives));

	// synchronize Node SoA for traversal hot-path
	BuildNodeSoA();
}

static inline void ExpandMinMax(FVector& MIn, FVector& Mx, const FAABB& A)
{
	MIn.X = std::min(MIn.X, A.Min.X); MIn.Y = std::min(MIn.Y, A.Min.Y); MIn.Z = std::min(MIn.Z, A.Min.Z);
	Mx.X = std::max(Mx.X, A.Max.X); Mx.Y = std::max(Mx.Y, A.Max.Y); Mx.Z = std::max(Mx.Z, A.Max.Z);
}

float FSceneBVH::SurfaceArea(const FAABB& B)
{
	const FVector E = B.Max - B.Min;
	const float Ex = std::max(0.f, E.X);
	const float Ey = std::max(0.f, E.Y);
	const float Ez = std::max(0.f, E.Z);
	return 2.0f * (Ex * Ey + Ey * Ez + Ez * Ex);
}

// Reworked BuildRecursive using helpers
int64 FSceneBVH::BuildRecursive(int64* IndexPtr, int64 Count)
{
	const int64 NodeIndex = static_cast<int64>(Nodes.size());
	Nodes.emplace_back();
	Parent.push_back(-1);

	// Compute union bounds
	FVector BMin, BMax;
	ComputeBoundsForRange(IndexPtr, Count, BMin, BMax);

	// Leaf condition
	constexpr int32 LeafMax = 4;
	if (Count <= LeafMax)
	{
		MakeLeafNode(NodeIndex, BMin, BMax, IndexPtr, Count);
		return NodeIndex;
	}

	// centroid bounds
	FVector CMin, CMax;
	ComputeCentroidBoundsForRange(IndexPtr, Count, CMin, CMax);
	const FVector CExt = CMax - CMin;
	const bool bDegenerate = (CExt.X <= 1e-6f) && (CExt.Y <= 1e-6f) && (CExt.Z <= 1e-6f);

	// attempt binned SAH split
	int64 BestAxis = -1;
	int32 BestSplitBin = -1;
	float BestCost = std::numeric_limits<float>::infinity();

	if (!bDegenerate)
	{
		FindBestSplit(IndexPtr, Count, CMin, CMax, reinterpret_cast<int&>(BestAxis), BestSplitBin, BestCost);
	}

	// fallback to median if split not found
	if (BestAxis < 0 || BestSplitBin < 0)
	{
		const FVector Ext = BMax - BMin;
		int64 Axis = 0;
		if (Ext.Y > Ext.X) Axis = 1;
		if (Ext.Z > ((Axis == 0) ? Ext.X : Ext.Y)) Axis = 2;

		const float Mid = ((Axis == 0 ? (BMin.X + BMax.X)
			: (Axis == 1 ? (BMin.Y + BMax.Y) : (BMin.Z + BMax.Z)))) * 0.5f;

		auto GetCenter = [&](int64 Idx)->float {
			const FAABB& A = PrimBounds[Idx];
			return Axis == 0 ? (A.Min.X + A.Max.X) * 0.5f
				: Axis == 1 ? (A.Min.Y + A.Max.Y) * 0.5f
				: (A.Min.Z + A.Max.Z) * 0.5f;
			};

		int64 LeftCountMedian = 0;
		for (int64 I = 0; I < Count; ++I)
		{
			if (GetCenter(IndexPtr[I]) < Mid)
				std::swap(IndexPtr[I], IndexPtr[LeftCountMedian++]);
		}
		if (LeftCountMedian == 0 || LeftCountMedian == Count)
			LeftCountMedian = Count / 2;

		const int64 L = BuildRecursive(IndexPtr, LeftCountMedian);
		const int64 R = BuildRecursive(IndexPtr + LeftCountMedian, Count - LeftCountMedian);

		Parent[L] = NodeIndex;
		Parent[R] = NodeIndex;

		FNode& NodeRef = Nodes[NodeIndex];
		NodeRef.Left = L;
		NodeRef.Right = R;
		NodeRef.Start = 0;
		NodeRef.Count = 0;
		NodeRef.Bounds = FAABB(BMin, BMax);
		return NodeIndex;
	}

	// convert BestSplitBin into SplitPos and partition (same as before)
	const float AxisCMin = (BestAxis == 0 ? CMin.X : (BestAxis == 1 ? CMin.Y : CMin.Z));
	const float AxisExt = (BestAxis == 0 ? CExt.X : (BestAxis == 1 ? CExt.Y : CExt.Z));
	const float BinSize = AxisExt / static_cast<float>(32); // BinCount
	const float SplitPos = AxisCMin + BinSize * static_cast<float>(BestSplitBin + 1);

	auto GetCenterAxis = [&](int64 Idx)->float {
		const FAABB& A = PrimBounds[Idx];
		return BestAxis == 0 ? (A.Min.X + A.Max.X) * 0.5f
			: BestAxis == 1 ? (A.Min.Y + A.Max.Y) * 0.5f
			: (A.Min.Z + A.Max.Z) * 0.5f;
		};

	int64 LeftCount = 0;
	for (int64 I = 0; I < Count; ++I)
	{
		if (GetCenterAxis(IndexPtr[I]) < SplitPos)
			std::swap(IndexPtr[I], IndexPtr[LeftCount++]);
	}
	if (LeftCount == 0 || LeftCount == Count)
		LeftCount = Count / 2;

	const int64 L = BuildRecursive(IndexPtr, LeftCount);
	const int64 R = BuildRecursive(IndexPtr + LeftCount, Count - LeftCount);

	Parent[L] = NodeIndex;
	Parent[R] = NodeIndex;

	FNode& NodeRef = Nodes[NodeIndex];
	NodeRef.Left = L;
	NodeRef.Right = R;
	NodeRef.Start = 0;
	NodeRef.Count = 0;
	NodeRef.Bounds = FAABB(BMin, BMax);
	return NodeIndex;
}

// Replace existing RayIntersectsAABB( cached, box ) to call internal and discard TMin/TMax
bool FSceneBVH::RayIntersectsAABB(const FRayCached& Cached, const FAABB& Box) const
{
	float TMin, TMax;
	return RayIntersectsAABBInternal(Cached, Box, TMin, TMax);
}

// Replace existing RayIntersectsAABB overload that fills OutTMin/OutTMax to call internal
bool FSceneBVH::RayIntersectsAABB(const FRayCached& Ray, const FAABB& Box, float& OutTMin, float& OutTMax) const
{
	return RayIntersectsAABBInternal(Ray, Box, OutTMin, OutTMax);
}

void FSceneBVH::QueryRay(const FRay& Ray, TArray<UPrimitiveComponent*>& OutCandidates) const
{
	OutCandidates.clear();
	if (Nodes.empty()) return;

	FRayCached Cached{
		{ Ray.Origin.X, Ray.Origin.Y, Ray.Origin.Z },
		{ 0, 0, 0 }
	};
	Cached.InvDir[0] = (fabsf(Ray.Direction.X) > 1e-8f) ? 1.0f / Ray.Direction.X : std::numeric_limits<float>::infinity();
	Cached.InvDir[1] = (fabsf(Ray.Direction.Y) > 1e-8f) ? 1.0f / Ray.Direction.Y : std::numeric_limits<float>::infinity();
	Cached.InvDir[2] = (fabsf(Ray.Direction.Z) > 1e-8f) ? 1.0f / Ray.Direction.Z : std::numeric_limits<float>::infinity();

	int64 Stack[128];
	int64 StackPtr = 0;
	Stack[StackPtr++] = 0;

	while (StackPtr > 0)
	{
		const int64 NodeIdx = Stack[--StackPtr];
		const FNode& NodeRef = Nodes[NodeIdx];
		if (!RayIntersectsAABB(Cached, NodeRef.Bounds)) continue;

		if (NodeRef.IsLeaf())
		{
			for (int64 I = 0; I < NodeRef.Count; ++I)
			{
				const int64 PrimIdx = Indices[NodeRef.Start + I];
				OutCandidates.push_back(Primitives[PrimIdx]);
			}
		}
		else
		{
			if (NodeRef.Left >= 0) Stack[StackPtr++] = NodeRef.Left;
			if (NodeRef.Right >= 0) Stack[StackPtr++] = NodeRef.Right;
		}
	}
}

void FSceneBVH::Refit()
{
	// 모든 프리미티브의 월드 AABB를 갱신하고 루트부터 보텀업으로 결합
	if (Primitives.empty() || Nodes.empty()) return;

	EnsurePrimSoASize();

	for (size_t I = 0; I < Primitives.size(); ++I)
	{
		if (!Primitives[I])
		{
			continue;
		}
		FVector MinW{}, MaxW{};
		try
		{
			Primitives[I]->GetWorldAABB(MinW, MaxW);
		}
		catch (...)
		{
			// 기본값으로 설정
			MinW = FVector::ZeroVector();
			MaxW = FVector::ZeroVector();
		}
		PrimBounds[I] = FAABB(MinW, MaxW);

		// update SoA
		PrimMinX[I] = MinW.X; PrimMinY[I] = MinW.Y; PrimMinZ[I] = MinW.Z;
		PrimMaxX[I] = MaxW.X; PrimMaxY[I] = MaxW.Y; PrimMaxZ[I] = MaxW.Z;
		PrimCentX[I] = (MinW.X + MaxW.X) * 0.5f;
		PrimCentY[I] = (MinW.Y + MaxW.Y) * 0.5f;
		PrimCentZ[I] = (MinW.Z + MaxW.Z) * 0.5f;
		// update radius^2
		{
			const float ex = PrimMaxX[I] - PrimCentX[I];
			const float ey = PrimMaxY[I] - PrimCentY[I];
			const float ez = PrimMaxZ[I] - PrimCentZ[I];
			PrimSphereRadiusSq[I] = ex * ex + ey * ey + ez * ez;
		}
	}
	RefitNode(0);

	// sync Node SoA after refit
	BuildNodeSoA();
}

void FSceneBVH::RefitDirtyByPrims(const TArray<UPrimitiveComponent*>& DirtyPrims)
{
	// 포인터 집합을 인덱스 집합으로 변환하여 부분 리핏
	if (DirtyPrims.empty() || Primitives.empty() || Nodes.empty()) return;

	TArray<int64> DirtyIndices;
	DirtyIndices.reserve(DirtyPrims.size());
	for (auto* Prim : DirtyPrims)
	{
		auto It = PrimIndexMap.find(Prim);
		if (It != PrimIndexMap.end())
			DirtyIndices.push_back(It->second);
	}
	if (!DirtyIndices.empty())
		RefitDirty(DirtyIndices);
}

void FSceneBVH::RefitDirty(const TArray<int64>& DirtyPrimIndices)
{
	// Dirty 프리미티브만 바운드 갱신 후, 해당 리프와 조상만 재계산
	if (DirtyPrimIndices.empty() || Primitives.empty() || Nodes.empty()) return;

	EnsurePrimSoASize();

	// 1) Dirty 프리미티브의 PrimBounds 갱신
	for (int64 PrimIdx : DirtyPrimIndices)
	{
		if (PrimIdx < 0 || PrimIdx >= static_cast<int64>(Primitives.size())) continue;
		if (!Primitives[PrimIdx]) continue;
		FVector MinW{}, MaxW{};
		try
		{
			Primitives[PrimIdx]->GetWorldAABB(MinW, MaxW);
		}
		catch (...)
		{
			// 기본값으로 설정
			MinW = FVector::ZeroVector();
			MaxW = FVector::ZeroVector();
		}
		PrimBounds[PrimIdx] = FAABB(MinW, MaxW);

		// update SoA
		PrimMinX[PrimIdx] = MinW.X; PrimMinY[PrimIdx] = MinW.Y; PrimMinZ[PrimIdx] = MinW.Z;
		PrimMaxX[PrimIdx] = MaxW.X; PrimMaxY[PrimIdx] = MaxW.Y; PrimMaxZ[PrimIdx] = MaxW.Z;
		PrimCentX[PrimIdx] = (MinW.X + MaxW.X) * 0.5f;
		PrimCentY[PrimIdx] = (MinW.Y + MaxW.Y) * 0.5f;
		PrimCentZ[PrimIdx] = (MinW.Z + MaxW.Z) * 0.5f;
		// update radius^2
		{
			const float ex = PrimMaxX[PrimIdx] - PrimCentX[PrimIdx];
			const float ey = PrimMaxY[PrimIdx] - PrimCentY[PrimIdx];
			const float ez = PrimMaxZ[PrimIdx] - PrimCentZ[PrimIdx];
			PrimSphereRadiusSq[PrimIdx] = ex * ex + ey * ey + ez * ez;
		}
	}

	// rest unchanged (compute leaves and parents)
	// 2) 해당 리프 노드 집합 계산(중복 제거)
	TArray<int64> DirtyLeaves;
	DirtyLeaves.reserve(DirtyPrimIndices.size());
	for (int64 PrimIdx : DirtyPrimIndices)
	{
		if (PrimIdx < 0 || PrimIdx >= static_cast<int64>(PrimToLeaf.size())) continue;
		const int64 LeafIdx = PrimToLeaf[PrimIdx];
		if (LeafIdx >= 0) DirtyLeaves.push_back(LeafIdx);
	}
	std::sort(DirtyLeaves.begin(), DirtyLeaves.end());
	DirtyLeaves.erase(std::unique(DirtyLeaves.begin(), DirtyLeaves.end()), DirtyLeaves.end());

	// 3) 리프 바운드 재계산
	for (int64 LeafIdx : DirtyLeaves)
	{
		RefitLeafBounds(LeafIdx);
	}

	// 4) 리프들의 모든 조상 바운드 전파(중복 제거)
	TArray<int64> DirtyNodes;
	for (int64 LeafIdx : DirtyLeaves)
	{
		int64 N = Parent[LeafIdx];
		while (N >= 0)
		{
			DirtyNodes.push_back(N);
			N = Parent[N];
		}
	}
	std::sort(DirtyNodes.begin(), DirtyNodes.end());
	DirtyNodes.erase(std::unique(DirtyNodes.begin(), DirtyNodes.end()), DirtyNodes.end());

	for (int64 NodeIdx : DirtyNodes)
		RefitInternalBounds(NodeIdx);

	// synchronize Node SoA for traversal
	BuildNodeSoA();
}

void FSceneBVH::RefitLeafBounds(int64 LeafNodeIndex)
{
	// 단일 리프 노드의 Bounds를 그 리프가 보유한 프리미티브로부터 재계산
	if (LeafNodeIndex < 0 || LeafNodeIndex >= static_cast<int64>(Nodes.size())) return;
	FNode& NodeRef = Nodes[LeafNodeIndex];
	if (!NodeRef.IsLeaf()) return;

	FVector BMin(std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity());
	FVector BMax(-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity());
	for (int64 I = 0; I < NodeRef.Count; ++I)
	{
		const int64 PrimIdx = Indices[NodeRef.Start + I];
		const FAABB& A = PrimBounds[PrimIdx];
		BMin.X = std::min(BMin.X, A.Min.X); BMin.Y = std::min(BMin.Y, A.Min.Y); BMin.Z = std::min(BMin.Z, A.Min.Z);
		BMax.X = std::max(BMax.X, A.Max.X); BMax.Y = std::max(BMax.Y, A.Max.Y); BMax.Z = std::max(BMax.Z, A.Max.Z);
	}
	NodeRef.Bounds = FAABB(BMin, BMax);
}

void FSceneBVH::RefitInternalBounds(int64 NodeIndex)
{
	// 내부 노드의 Bounds를 두 자식의 합집합으로 갱신
	if (NodeIndex < 0 || NodeIndex >= static_cast<int64>(Nodes.size())) return;
	FNode& NodeRef = Nodes[NodeIndex];
	if (NodeRef.IsLeaf()) return;

	const FAABB& L = Nodes[NodeRef.Left].Bounds;
	const FAABB& R = Nodes[NodeRef.Right].Bounds;

	FVector BMin(std::min(L.Min.X, R.Min.X),
		std::min(L.Min.Y, R.Min.Y),
		std::min(L.Min.Z, R.Min.Z));
	FVector BMax(std::max(L.Max.X, R.Max.X),
		std::max(L.Max.Y, R.Max.Y),
		std::max(L.Max.Z, R.Max.Z));
	NodeRef.Bounds = FAABB(BMin, BMax);
}

void FSceneBVH::RefitNode(int64 NodeIndex)
{
	// 전체 보텀업 리핏(재귀)
	FNode& NodeRef = Nodes[NodeIndex];
	if (NodeRef.IsLeaf())
	{
		RefitLeafBounds(NodeIndex);
		return;
	}
	if (NodeRef.Left >= 0) RefitNode(NodeRef.Left);
	if (NodeRef.Right >= 0) RefitNode(NodeRef.Right);
	RefitInternalBounds(NodeIndex);
}

// --- New helper: ensure SoA arrays match PrimBounds size ---
void FSceneBVH::EnsurePrimSoASize() const
{
	const size_t N = PrimBounds.size();
	if (PrimMinX.size() != N)
	{
		PrimMinX.resize(N); PrimMinY.resize(N); PrimMinZ.resize(N);
		PrimMaxX.resize(N); PrimMaxY.resize(N); PrimMaxZ.resize(N);
		PrimCentX.resize(N); PrimCentY.resize(N); PrimCentZ.resize(N);
		PrimSphereRadiusSq.resize(N);
	}
}

bool FSceneBVH::RayIntersectsAABB_PrimIndex(const FRayCached& Ray, int64 PrimIdx, float& OutTMin, float& OutTMax) const
{
	// read SoA in tight order (cache-friendly)
	const float BMin[3] = { PrimMinX[PrimIdx], PrimMinY[PrimIdx], PrimMinZ[PrimIdx] };
	const float BMax[3] = { PrimMaxX[PrimIdx], PrimMaxY[PrimIdx], PrimMaxZ[PrimIdx] };

	float TMin = 0.f, TMax = std::numeric_limits<float>::max();

	for (int I = 0; I < 3; ++I)
	{
		const float InvD = Ray.InvDir[I];
		float T0 = (BMin[I] - Ray.Origin[I]) * InvD;
		float T1 = (BMax[I] - Ray.Origin[I]) * InvD;
		if (T0 > T1) std::swap(T0, T1);
		TMin = std::max(TMin, T0);
		TMax = std::min(TMax, T1);
		if (TMax < TMin) return false;
	}
	OutTMin = TMin;
	OutTMax = TMax;
	return true;
}

// ComputeBoundsForRange: use SoA arrays for faster linear access
void FSceneBVH::ComputeBoundsForRange(const int64* IndexPtr, int64 Count, FVector& OutMin, FVector& OutMax) const
{
	OutMin = FVector(std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity());
	OutMax = FVector(-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity());

	for (int64 i = 0; i < Count; ++i)
	{
		const int64 Idx = IndexPtr[i];
		OutMin.X = std::min(OutMin.X, PrimMinX[Idx]);
		OutMin.Y = std::min(OutMin.Y, PrimMinY[Idx]);
		OutMin.Z = std::min(OutMin.Z, PrimMinZ[Idx]);
		OutMax.X = std::max(OutMax.X, PrimMaxX[Idx]);
		OutMax.Y = std::max(OutMax.Y, PrimMaxY[Idx]);
		OutMax.Z = std::max(OutMax.Z, PrimMaxZ[Idx]);
	}
}

// ComputeCentroidBoundsForRange: use PrimCent arrays
void FSceneBVH::ComputeCentroidBoundsForRange(const int64* IndexPtr, int64 Count, FVector& OutCentMin, FVector& OutCentMax) const
{
	OutCentMin = FVector(std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity());
	OutCentMax = FVector(-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity());

	for (int64 i = 0; i < Count; ++i)
	{
		const int64 Idx = IndexPtr[i];
		const float cx = PrimCentX[Idx];
		const float cy = PrimCentY[Idx];
		const float cz = PrimCentZ[Idx];
		OutCentMin.X = std::min(OutCentMin.X, cx);
		OutCentMin.Y = std::min(OutCentMin.Y, cy);
		OutCentMin.Z = std::min(OutCentMin.Z, cz);
		OutCentMax.X = std::max(OutCentMax.X, cx);
		OutCentMax.Y = std::max(OutCentMax.Y, cy);
		OutCentMax.Z = std::max(OutCentMax.Z, cz);
	}
}

// Helper: create leaf node and set PrimToLeaf entries
void FSceneBVH::MakeLeafNode(int64 NodeIndex, const FVector& BMin, const FVector& BMax, int64* IndexPtr, int64 Count)
{
	FNode& NodeRef = Nodes[NodeIndex];
	NodeRef.Bounds = FAABB(BMin, BMax);
	NodeRef.Start = static_cast<int64>(IndexPtr - Indices.data());
	NodeRef.Count = Count;
	NodeRef.Left = -1;
	NodeRef.Right = -1;

	for (int64 I = 0; I < Count; ++I)
	{
		const int64 P = Indices[NodeRef.Start + I];
		PrimToLeaf[P] = NodeIndex;
	}
}

// Helper: FindBestSplit using binned SAH (refactored from TryAxis)
void FSceneBVH::FindBestSplit(const int64* IndexPtr, int64 Count, const FVector& CentMin, const FVector& CentMax,
	int& OutBestAxis, int& OutBestSplitBin, float& OutBestCost) const
{
	constexpr int32 BinCount = 32;
	OutBestAxis = -1;
	OutBestSplitBin = -1;
	OutBestCost = std::numeric_limits<float>::infinity();

	const FVector CExt = CentMax - CentMin;
	auto TryAxisFn = [&](int Axis)
		{
			const float Ext = (Axis == 0 ? CExt.X : (Axis == 1 ? CExt.Y : CExt.Z));
			if (Ext <= 1e-6f) return;

			int32 Counts[BinCount] = { 0 };
			FVector BinMin[BinCount];
			FVector BinMax[BinCount];
			for (int32 b = 0; b < BinCount; ++b)
			{
				BinMin[b] = FVector(std::numeric_limits<float>::infinity(),
					std::numeric_limits<float>::infinity(),
					std::numeric_limits<float>::infinity());
				BinMax[b] = FVector(-std::numeric_limits<float>::infinity(),
					-std::numeric_limits<float>::infinity(),
					-std::numeric_limits<float>::infinity());
			}

			const float InvExt = 1.0f / Ext;
			for (int64 I = 0; I < Count; ++I)
			{
				const int64 PrimIdx = IndexPtr[I];
				const FAABB& A = PrimBounds[PrimIdx];
				const FVector C{ (A.Min.X + A.Max.X) * 0.5f, (A.Min.Y + A.Max.Y) * 0.5f, (A.Min.Z + A.Max.Z) * 0.5f };
				const float CAxis = (Axis == 0 ? C.X : (Axis == 1 ? C.Y : C.Z));
				int32 BinId = static_cast<int32>(((CAxis - (Axis == 0 ? CentMin.X : (Axis == 1 ? CentMin.Y : CentMin.Z))) * InvExt) * BinCount);
				if (BinId < 0) BinId = 0;
				if (BinId >= BinCount) BinId = BinCount - 1;

				++Counts[BinId];
				const FAABB& PB = PrimBounds[PrimIdx];
				BinMin[BinId].X = std::min(BinMin[BinId].X, PB.Min.X);
				BinMin[BinId].Y = std::min(BinMin[BinId].Y, PB.Min.Y);
				BinMin[BinId].Z = std::min(BinMin[BinId].Z, PB.Min.Z);
				BinMax[BinId].X = std::max(BinMax[BinId].X, PB.Max.X);
				BinMax[BinId].Y = std::max(BinMax[BinId].Y, PB.Max.Y);
				BinMax[BinId].Z = std::max(BinMax[BinId].Z, PB.Max.Z);
			}

			// prefix/suffix
			int32 LeftCount[BinCount] = { 0 };
			FAABB LeftBox[BinCount];
			for (int32 b = 0; b < BinCount; ++b)
			{
				if (b == 0)
				{
					LeftCount[b] = Counts[b];
					LeftBox[b] = FAABB(BinMin[b], BinMax[b]);
				}
				else
				{
					LeftCount[b] = LeftCount[b - 1] + Counts[b];
					FVector LMn = LeftBox[b - 1].Min;
					FVector LMx = LeftBox[b - 1].Max;
					FAABB Add(BinMin[b], BinMax[b]);
					ExpandMinMax(LMn, LMx, Add);
					LeftBox[b] = FAABB(LMn, LMx);
				}
			}

			int32 RightCount[BinCount] = { 0 };
			FAABB RightBox[BinCount];
			for (int32 b = BinCount - 1; b >= 0; --b)
			{
				if (b == BinCount - 1)
				{
					RightCount[b] = Counts[b];
					RightBox[b] = FAABB(BinMin[b], BinMax[b]);
				}
				else
				{
					RightCount[b] = RightCount[b + 1] + Counts[b];
					FVector RMn = RightBox[b + 1].Min;
					FVector RMx = RightBox[b + 1].Max;
					FAABB Add(BinMin[b], BinMax[b]);
					ExpandMinMax(RMn, RMx, Add);
					RightBox[b] = FAABB(RMn, RMx);
				}
			}

			for (int32 s = 0; s < BinCount - 1; ++s)
			{
				const int32 NL = LeftCount[s];
				const int32 NR = RightCount[s + 1];
				if (NL == 0 || NR == 0) continue;

				const float SAL = SurfaceArea(LeftBox[s]);
				const float SAR = SurfaceArea(RightBox[s + 1]);
				const float Cost = SAL * NL + SAR * NR;

				if (Cost < OutBestCost)
				{
					OutBestCost = Cost;
					OutBestAxis = Axis;
					OutBestSplitBin = s;
				}
			}
		};

	// try axes
	if (!((CentMax.X - CentMin.X) <= 1e-6f && (CentMax.Y - CentMin.Y) <= 1e-6f && (CentMax.Z - CentMin.Z) <= 1e-6f))
	{
		TryAxisFn(0);
		TryAxisFn(1);
		TryAxisFn(2);
	}
}

// Internal ray-AABB slab test that returns TMin/TMax
bool FSceneBVH::RayIntersectsAABBInternal(const FRayCached& Ray, const FAABB& Box, float& OutTMin, float& OutTMax) const
{
	float TMin = 0.f, TMax = std::numeric_limits<float>::max();
	const float BMin[3] = { Box.Min.X, Box.Min.Y, Box.Min.Z };
	const float BMax[3] = { Box.Max.X, Box.Max.Y, Box.Max.Z };

	for (int I = 0; I < 3; ++I)
	{
		const float InvD = Ray.InvDir[I];
		float T0 = (BMin[I] - Ray.Origin[I]) * InvD;
		float T1 = (BMax[I] - Ray.Origin[I]) * InvD;
		if (T0 > T1) std::swap(T0, T1);
		TMin = std::max(TMin, T0);
		TMax = std::min(TMax, T1);
		if (TMax < TMin) return false;
	}
	OutTMin = TMin;
	OutTMax = TMax;
	return true;
}

// Ensure Node SoA arrays sized
void FSceneBVH::EnsureNodeSoASize() const
{
	const size_t N = Nodes.size();
	if (NodeMinX.size() != N)
	{
		NodeMinX.resize(N); NodeMinY.resize(N); NodeMinZ.resize(N);
		NodeMaxX.resize(N); NodeMaxY.resize(N); NodeMaxZ.resize(N);
	}
}

// Build Node SoA from Nodes[].Bounds
void FSceneBVH::BuildNodeSoA() const
{
	EnsureNodeSoASize();
	for (size_t i = 0; i < Nodes.size(); ++i)
	{
		const FAABB& B = Nodes[i].Bounds;
		NodeMinX[i] = B.Min.X; NodeMinY[i] = B.Min.Y; NodeMinZ[i] = B.Min.Z;
		NodeMaxX[i] = B.Max.X; NodeMaxY[i] = B.Max.Y; NodeMaxZ[i] = B.Max.Z;
	}
}

// Hot-path node-index based slab test using Node SoA
bool FSceneBVH::RayIntersectsAABB_NodeIndex(const FRayCached& Ray, int64 NodeIdx, float& OutTMin, float& OutTMax) const
{
	// read Node SoA contiguously for cache friendliness
	const float BMin[3] = { NodeMinX[NodeIdx], NodeMinY[NodeIdx], NodeMinZ[NodeIdx] };
	const float BMax[3] = { NodeMaxX[NodeIdx], NodeMaxY[NodeIdx], NodeMaxZ[NodeIdx] };

	float TMin = 0.f, TMax = std::numeric_limits<float>::max();

	for (int I = 0; I < 3; ++I)
	{
		const float InvD = Ray.InvDir[I];
		float T0 = (BMin[I] - Ray.Origin[I]) * InvD;
		float T1 = (BMax[I] - Ray.Origin[I]) * InvD;
		if (T0 > T1) std::swap(T0, T1);
		TMin = std::max(TMin, T0);
		TMax = std::min(TMax, T1);
		if (TMax < TMin) return false;
	}
	OutTMin = TMin;
	OutTMax = TMax;
	return true;
}

bool FSceneBVH::GetPrimBounds(UPrimitiveComponent* Prim, FAABB& OutBounds) const
{
	if (!Prim)
	{
		return false;
	}

	auto It = PrimIndexMap.find(Prim);
	if (It == PrimIndexMap.end())
	{
		return false;
	}
	
	const int64 Index = It->second;
	if (Index < 0 || Index >= static_cast<int64>(PrimBounds.size()))
	{
		return false;
	}
	
	OutBounds = PrimBounds[Index];
	return true;
}

bool FSceneBVH::GetPrimSphereByIndex(int64 PrimIdx, FVector& OutCenter, float& OutRadiusSq) const
{
	if (PrimIdx < 0 || PrimIdx >= static_cast<int64>(PrimCentX.size()))
		return false;
	OutCenter = FVector(PrimCentX[PrimIdx], PrimCentY[PrimIdx], PrimCentZ[PrimIdx]);
	OutRadiusSq = PrimSphereRadiusSq[PrimIdx];
	return true;
}
