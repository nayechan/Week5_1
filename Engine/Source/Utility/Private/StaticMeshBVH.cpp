#include "pch.h"
#include "Utility/Public/StaticMeshBVH.h"
#include "Utility/Public/ScopeCycleCounter.h"

// ======= FStaticMeshBVH =======
void FStaticMeshBVH::Clear()
{
	Nodes.clear();
	TriRefs.clear();
	TriBounds.clear();
	Vtx = nullptr;
	Idx = nullptr;
	NumTris = 0;

	// clear SoA caches
	TriV0X.clear(); TriV0Y.clear(); TriV0Z.clear();
	TriE1X.clear(); TriE1Y.clear(); TriE1Z.clear();
	TriE2X.clear(); TriE2Y.clear(); TriE2Z.clear();
	TriCentX.clear(); TriCentY.clear(); TriCentZ.clear();
	TriRadiusSq.clear();
}

FAABB FStaticMeshBVH::ComputeTriBounds(int32 TriIdx) const
{
	auto fetch = [&](int32 tri, int v)->FVector {
		if (Idx)
		{
			const int32 i = (*Idx)[tri * 3 + v];
			return (*Vtx)[i].Position;
		}
		else
		{
			return (*Vtx)[tri * 3 + v].Position;
		}
		};

	const FVector a = fetch(TriIdx, 0);
	const FVector b = fetch(TriIdx, 1);
	const FVector c = fetch(TriIdx, 2);

	FVector mn(std::min({ a.X, b.X, c.X }), std::min({ a.Y, b.Y, c.Y }), std::min({ a.Z, b.Z, c.Z }));
	FVector mx(std::max({ a.X, b.X, c.X }), std::max({ a.Y, b.Y, c.Y }), std::max({ a.Z, b.Z, c.Z }));
	return FAABB(mn, mx);
}

float FStaticMeshBVH::SurfaceArea(const FAABB& B)
{
	const FVector e = B.Max - B.Min;
	const float ex = std::max(0.f, e.X);
	const float ey = std::max(0.f, e.Y);
	const float ez = std::max(0.f, e.Z);
	return 2.0f * (ex * ey + ey * ez + ez * ex);
}

static inline void ExpandMinMax(FVector& mn, FVector& mx, const FAABB& a) {
	mn.X = std::min(mn.X, a.Min.X); mn.Y = std::min(mn.Y, a.Min.Y); mn.Z = std::min(mn.Z, a.Min.Z);
	mx.X = std::max(mx.X, a.Max.X); mx.Y = std::max(mx.Y, a.Max.Y); mx.Z = std::max(mx.Z, a.Max.Z);
}

int32 FStaticMeshBVH::BuildRecursive(int32* TriIndexPtr, int32 Count)
{
	const int32 NodeIdx = static_cast<int32>(Nodes.size());
	Nodes.emplace_back();

	// 1) 현재 구간 Bounds 계산
	FVector mn(std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity());
	FVector mx(-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity());

	for (int32 i = 0; i < Count; ++i) {
		const FAABB& tb = TriBounds[TriIndexPtr[i]];
		ExpandMinMax(mn, mx, tb);
	}

	// 2) Leaf 조건
	constexpr int32 LeafMax = 4; // 필요 시 2~8 튜닝
	if (Count <= LeafMax) {
		FNode& n = Nodes[NodeIdx];
		n.Bounds = FAABB(mn, mx);
		n.Start = 0;     // leaf Start는 Build() 말미에 일괄 재설정
		n.Count = Count; // leaf의 삼각형 개수
		n.Left = -1; n.Right = -1;
		return NodeIdx;
	}

	// 3) Centroid AABB 계산
	FVector cmin(std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity());
	FVector cmax(-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity());

	for (int32 i = 0; i < Count; ++i) {
		const FAABB& a = TriBounds[TriIndexPtr[i]];
		const FVector c{ (a.Min.X + a.Max.X) * 0.5f,
						 (a.Min.Y + a.Max.Y) * 0.5f,
						 (a.Min.Z + a.Max.Z) * 0.5f };
		cmin.X = std::min(cmin.X, c.X); cmin.Y = std::min(cmin.Y, c.Y); cmin.Z = std::min(cmin.Z, c.Z);
		cmax.X = std::max(cmax.X, c.X); cmax.Y = std::max(cmax.Y, c.Y); cmax.Z = std::max(cmax.Z, c.Z);
	}

	const FVector cext = cmax - cmin;
	const bool bDegenerate = (cext.X <= 1e-6f) && (cext.Y <= 1e-6f) && (cext.Z <= 1e-6f);

	// 4) Binned SAH
	constexpr int32 BinCount = 32; // 12~32 권장
	int32 BestAxis = -1;
	int32 BestSplitBin = -1;
	float BestCost = std::numeric_limits<float>::infinity();

	auto TryAxis = [&](int Axis)
		{
			const float Ext = (Axis == 0) ? cext.X : (Axis == 1) ? cext.Y : cext.Z;
			if (Ext <= 1e-6f) return;

			int32 Counts[BinCount] = { 0 };
			FVector BinMin[BinCount];
			FVector BinMax[BinCount];
			for (int32 b = 0; b < BinCount; ++b) {
				BinMin[b] = FVector(std::numeric_limits<float>::infinity(),
					std::numeric_limits<float>::infinity(),
					std::numeric_limits<float>::infinity());
				BinMax[b] = FVector(-std::numeric_limits<float>::infinity(),
					-std::numeric_limits<float>::infinity(),
					-std::numeric_limits<float>::infinity());
			}

			const float AxisCMin = (Axis == 0) ? cmin.X : (Axis == 1) ? cmin.Y : cmin.Z;
			const float InvExt = 1.0f / Ext;

			// 4-1) bin 누적
			for (int32 i = 0; i < Count; ++i) {
				const int32 tri = TriIndexPtr[i];
				const FAABB& A = TriBounds[tri];
				const float cx = (A.Min.X + A.Max.X) * 0.5f;
				const float cy = (A.Min.Y + A.Max.Y) * 0.5f;
				const float cz = (A.Min.Z + A.Max.Z) * 0.5f;
				const float CAxis = (Axis == 0) ? cx : (Axis == 1) ? cy : cz;

				int32 BinId = static_cast<int32>(((CAxis - AxisCMin) * InvExt) * BinCount);
				if (BinId < 0) BinId = 0; if (BinId >= BinCount) BinId = BinCount - 1;

				++Counts[BinId];
				ExpandMinMax(BinMin[BinId], BinMax[BinId], A);
			}

			// 4-2) prefix(Left), suffix(Right)
			int32 LeftCount[BinCount] = { 0 };
			FAABB LeftBox[BinCount] = {
				FAABB(FVector(std::numeric_limits<float>::infinity(),
							   std::numeric_limits<float>::infinity(),
							   std::numeric_limits<float>::infinity()),
					  FVector(-std::numeric_limits<float>::infinity(),
							  -std::numeric_limits<float>::infinity(),
							  -std::numeric_limits<float>::infinity()))
			};
			for (int32 b = 0; b < BinCount; ++b) {
				if (b == 0) {
					LeftCount[b] = Counts[b];
					LeftBox[b] = FAABB(BinMin[b], BinMax[b]);
				}
				else {
					LeftCount[b] = LeftCount[b - 1] + Counts[b];
					FVector LMn = LeftBox[b - 1].Min, LMx = LeftBox[b - 1].Max;
					ExpandMinMax(LMn, LMx, FAABB(BinMin[b], BinMax[b]));
					LeftBox[b] = FAABB(LMn, LMx);
				}
			}

			int32 RightCount[BinCount] = { 0 };
			FAABB RightBox[BinCount] = {
				FAABB(FVector(std::numeric_limits<float>::infinity(),
							   std::numeric_limits<float>::infinity(),
							   std::numeric_limits<float>::infinity()),
					  FVector(-std::numeric_limits<float>::infinity(),
							  -std::numeric_limits<float>::infinity(),
							  -std::numeric_limits<float>::infinity()))
			};
			for (int32 b = BinCount - 1; b >= 0; --b) {
				if (b == BinCount - 1) {
					RightCount[b] = Counts[b];
					RightBox[b] = FAABB(BinMin[b], BinMax[b]);
				}
				else {
					RightCount[b] = RightCount[b + 1] + Counts[b];
					FVector RMn = RightBox[b + 1].Min, RMx = RightBox[b + 1].Max;
					ExpandMinMax(RMn, RMx, FAABB(BinMin[b], BinMax[b]));
					RightBox[b] = FAABB(RMn, RMx);
				}
			}

			// 4-3) split 후보 평가 (0..BinCount-2)
			for (int32 s = 0; s < BinCount - 1; ++s) {
				const int32 NL = LeftCount[s];
				const int32 NR = RightCount[s + 1];
				if (NL == 0 || NR == 0) continue;

				const float SAL = SurfaceArea(LeftBox[s]);
				const float SAR = SurfaceArea(RightBox[s + 1]);
				const float Cost = SAL * NL + SAR * NR;

				if (Cost < BestCost) {
					BestCost = Cost;
					BestAxis = Axis;
					BestSplitBin = s;
				}
			}
		};

	if (!bDegenerate) {
		TryAxis(0); TryAxis(1); TryAxis(2); // 축별 SAH 평가  // SceneBVH 참조
	}

	// 5) 분할 불가/이득 없음 → Fallback: longest-axis median
	if (BestAxis < 0 || BestSplitBin < 0)
	{
		const FVector ext = mx - mn;
		int axis = 0;
		if (ext.Y > ext.X) axis = 1;
		if (ext.Z > ((axis == 0) ? ext.X : (axis == 1 ? ext.Y : ext.Z))) axis = 2;

		const float mid =
			(axis == 0) ? (mn.X + mx.X) * 0.5f :
			(axis == 1) ? (mn.Y + mx.Y) * 0.5f : (mn.Z + mx.Z) * 0.5f;

		auto Center = [&](int32 tri)->float {
			const FAABB& a = TriBounds[tri];
			return (axis == 0) ? (a.Min.X + a.Max.X) * 0.5f :
				(axis == 1) ? (a.Min.Y + a.Max.Y) * 0.5f :
				(a.Min.Z + a.Max.Z) * 0.5f;
			};

		int32 midCount = 0;
		for (int32 i = 0; i < Count; ++i) {
			if (Center(TriIndexPtr[i]) < mid)
				std::swap(TriIndexPtr[i], TriIndexPtr[midCount++]);
		}
		if (midCount == 0 || midCount == Count) midCount = Count / 2;

		const int32 L = BuildRecursive(TriIndexPtr, midCount);
		const int32 R = BuildRecursive(TriIndexPtr + midCount, Count - midCount);

		FNode& n = Nodes[NodeIdx];
		n.Left = L; n.Right = R; n.Bounds = FAABB(mn, mx);
		return NodeIdx;
	}

	// 6) SAH 선택 결과로 파티션
	const float AxisCMin = (BestAxis == 0) ? cmin.X : (BestAxis == 1) ? cmin.Y : cmin.Z;
	const float AxisExt = (BestAxis == 0) ? cext.X : (BestAxis == 1) ? cext.Y : cext.Z;
	const float BinSize = AxisExt / static_cast<float>(BinCount);
	const float SplitPos = AxisCMin + BinSize * static_cast<float>(BestSplitBin + 1);

	auto CenterAxis = [&](int32 tri)->float {
		const FAABB& a = TriBounds[tri];
		return (BestAxis == 0) ? (a.Min.X + a.Max.X) * 0.5f :
			(BestAxis == 1) ? (a.Min.Y + a.Max.Y) * 0.5f :
			(a.Min.Z + a.Max.Z) * 0.5f;
		};

	int32 LeftCount = 0;
	for (int32 i = 0; i < Count; ++i) {
		if (CenterAxis(TriIndexPtr[i]) < SplitPos)
			std::swap(TriIndexPtr[i], TriIndexPtr[LeftCount++]);
	}
	if (LeftCount == 0 || LeftCount == Count)
		LeftCount = Count / 2; // 안전 보정 (SceneBVH와 동일)  // filecite

	// 7) 재귀
	const int32 L = BuildRecursive(TriIndexPtr, LeftCount);
	const int32 R = BuildRecursive(TriIndexPtr + LeftCount, Count - LeftCount);

	FNode& n = Nodes[NodeIdx];
	n.Left = L; n.Right = R; n.Bounds = FAABB(mn, mx);
	return NodeIdx;
}

void FStaticMeshBVH::Build(const TArray<FNormalVertex>& Vertices, const TArray<uint32>* Indices)
{
	Clear();
	Vtx = &Vertices;
	Idx = Indices;

	if (Idx && (Idx->size() % 3 == 0)) NumTris = static_cast<int32>(Idx->size() / 3);
	else if (!Idx && (Vertices.size() % 3 == 0)) NumTris = static_cast<int32>(Vertices.size() / 3);
	else { NumTris = 0; return; }

	TriBounds.resize(NumTris);
	for (int32 t = 0; t < NumTris; ++t) TriBounds[t] = ComputeTriBounds(t);

	TriRefs.resize(NumTris);
	for (int32 t = 0; t < NumTris; ++t) TriRefs[t] = { t };

	// --- Build SoA triangle caches (v0, e1, e2, centroid, radius^2) ---
	EnsureTriSoASize();
	for (int32 t = 0; t < NumTris; ++t)
	{
		auto fetch = [&](int tri, int v)->FVector {
			if (Idx)
			{
				const int32 i = (*Idx)[tri * 3 + v];
				return (*Vtx)[i].Position;
			}
			else
			{
				return (*Vtx)[tri * 3 + v].Position;
			}
			};

		const FVector v0 = fetch(t, 0);
		const FVector v1 = fetch(t, 1);
		const FVector v2 = fetch(t, 2);

		TriV0X[t] = v0.X; TriV0Y[t] = v0.Y; TriV0Z[t] = v0.Z;
		TriE1X[t] = v1.X - v0.X; TriE1Y[t] = v1.Y - v0.Y; TriE1Z[t] = v1.Z - v0.Z;
		TriE2X[t] = v2.X - v0.X; TriE2Y[t] = v2.Y - v0.Y; TriE2Z[t] = v2.Z - v0.Z;

		const float cx = (v0.X + v1.X + v2.X) / 3.0f;
		const float cy = (v0.Y + v1.Y + v2.Y) / 3.0f;
		const float cz = (v0.Z + v1.Z + v2.Z) / 3.0f;
		TriCentX[t] = cx; TriCentY[t] = cy; TriCentZ[t] = cz;

		const float dx0 = v0.X - cx, dy0 = v0.Y - cy, dz0 = v0.Z - cz;
		const float dx1 = v1.X - cx, dy1 = v1.Y - cy, dz1 = v1.Z - cz;
		const float dx2 = v2.X - cx, dy2 = v2.Y - cy, dz2 = v2.Z - cz;
		const float r20 = dx0 * dx0 + dy0 * dy0 + dz0 * dz0;
		const float r21 = dx1 * dx1 + dy1 * dy1 + dz1 * dz1;
		const float r22 = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;
		TriRadiusSq[t] = std::max(std::max(r20, r21), r22);
	}

	TriRefs.resize(NumTris);
	for (int32 t = 0; t < NumTris; ++t) TriRefs[t] = { t };

	Nodes.reserve(NumTris * 2);
	// 파티션 인덱스 버퍼(연속 메모리 필요)
	TArray<int32> Temp; Temp.resize(NumTris);
	for (int32 t = 0; t < NumTris; ++t) Temp[t] = t;

	// TriRefs 배열을 인덱싱하는 대신 오프셋 계산을 위해 reinterpret_cast 사용(동일 메모리 레이아웃 전제)
	BuildRecursive(Temp.data(), NumTris);

	// leaf 구간의 Start는 TriRefs 기준 오프셋이므로 맞춰줌
	int32 offset = 0;
	for (auto& n : Nodes)
	{
		if (n.IsLeaf())
		{
			n.Start = offset;
			offset += n.Count;
		}
	}

	// Temp의 순서에 맞춰 TriRefs 재배열
	TArray<FTriRef> NewRefs; NewRefs.resize(NumTris);
	int32 idx = 0;
	for (int32 i = 0; i < NumTris; ++i) NewRefs[idx++] = { Temp[i] };
	TriRefs.swap(NewRefs);
}

bool FStaticMeshBVH::RayIntersectsAABB(const FRayCached& Ray, const FAABB& Box, float& OutTMin, float& OutTMax)
{
	float tmin = 0.f, tmax = std::numeric_limits<float>::max();
	const float bmin[3] = { Box.Min.X, Box.Min.Y, Box.Min.Z };
	const float bmax[3] = { Box.Max.X, Box.Max.Y, Box.Max.Z };
	for (int i = 0; i < 3; ++i)
	{
		float t0 = (bmin[i] - Ray.Origin[i]) * Ray.InvDir[i];
		float t1 = (bmax[i] - Ray.Origin[i]) * Ray.InvDir[i];
		if (t0 > t1) std::swap(t0, t1);
		tmin = std::max(tmin, t0);
		tmax = std::min(tmax, t1);
		if (tmax < tmin) return false;
	}
	OutTMin = tmin; OutTMax = tmax; return true;
}

// NOTE: old std::function overload may remain but the templated version in the header is used.

void FStaticMeshBVH::EnsureTriSoASize() const
{
	const size_t N = static_cast<size_t>(NumTris);
	if (TriV0X.size() != N)
	{
		TriV0X.resize(N); TriV0Y.resize(N); TriV0Z.resize(N);
		TriE1X.resize(N); TriE1Y.resize(N); TriE1Z.resize(N);
		TriE2X.resize(N); TriE2Y.resize(N); TriE2Z.resize(N);
		TriCentX.resize(N); TriCentY.resize(N); TriCentZ.resize(N);
		TriRadiusSq.resize(N);
	}
}
