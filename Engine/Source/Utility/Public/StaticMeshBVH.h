#pragma once
#include <functional>
#include "Physics/Public/AABB.h"
#include "ScopeCycleCounter.h"

// 정적 메시의 Triangle BVH
class FStaticMeshBVH
{
public:
	// vertices: 모델 공간 정점, indices: null이면 0..N-1 삼각형 리스트(3개씩)
	void Build(const TArray<FNormalVertex>& Vertices, const TArray<uint32>* Indices);

	// front-to-back 순회. cutoffT는 모델T 컷(없으면 +inf). 방문 콜백은 (triIndex)->void
	template<typename TVisitor>
	inline void TraverseFrontToBack(const FRay& ModelRay, float CutoffT, TVisitor&& VisitTriangle) const
	{
#ifdef ENABLE_BVH_STATS
		FScopeCycleCounter Counter(GetStaticMeshBVHTraverseStatId());
#endif

		if (Nodes.empty()) return;

		const FRayCached RC = CacheRay(ModelRay);

		struct FEntry { int32 Index; float TMin; };
		FEntry stack[256];
		int sp = 0;

		float rootTMin = 0, rootTMax = 0;
		if (!RayIntersectsAABB(RC, Nodes[0].Bounds, rootTMin, rootTMax)) return;
		stack[sp++] = { 0, rootTMin };

		const float cutoff = CutoffT > 0.f ? CutoffT : std::numeric_limits<float>::infinity();

		while (sp > 0)
		{
			const FEntry e = stack[--sp];
			if (e.TMin > cutoff) continue;

			const FNode& n = Nodes[e.Index];
			if (n.IsLeaf())
			{
				for (int32 i = 0; i < n.Count; ++i)
				{
					const int32 tri = TriRefs[n.Start + i].Index;
					VisitTriangle(tri);
				}
			}
			else
			{
				float lT0 = 0, lT1 = 0, rT0 = 0, rT1 = 0;
				const bool hitL = (n.Left >= 0) && RayIntersectsAABB(RC, Nodes[n.Left].Bounds, lT0, lT1) && lT0 <= cutoff;
				const bool hitR = (n.Right >= 0) && RayIntersectsAABB(RC, Nodes[n.Right].Bounds, rT0, rT1) && rT0 <= cutoff;

				if (hitL && hitR)
				{
					// push farther first so nearer is popped/processed next
					if (lT0 < rT0) { stack[sp++] = { n.Right, rT0 }; stack[sp++] = { n.Left, lT0 }; }
					else { stack[sp++] = { n.Left, lT0 };  stack[sp++] = { n.Right, rT0 }; }
				}
				else if (hitL) { stack[sp++] = { n.Left,  lT0 }; }
				else if (hitR) { stack[sp++] = { n.Right, rT0 }; }
			}
		}
	}

	bool IsBuilt() const { return !Nodes.empty(); }
	void Clear();

	// cheap inline accessors
	inline void GetTriV0E1E2(int32 TriIdx, FVector& OutV0, FVector& OutE1, FVector& OutE2) const
	{
		OutV0.X = TriV0X[TriIdx]; OutV0.Y = TriV0Y[TriIdx]; OutV0.Z = TriV0Z[TriIdx];
		OutE1.X = TriE1X[TriIdx]; OutE1.Y = TriE1Y[TriIdx]; OutE1.Z = TriE1Z[TriIdx];
		OutE2.X = TriE2X[TriIdx]; OutE2.Y = TriE2Y[TriIdx]; OutE2.Z = TriE2Z[TriIdx];
	}

	inline bool GetTriCentroidRadiusSq(int32 TriIdx, FVector& OutCent, float& OutRadiusSq) const
	{
		if (TriIdx < 0 || TriIdx >= static_cast<int32>(TriCentX.size())) return false;
		OutCent.X = TriCentX[TriIdx]; OutCent.Y = TriCentY[TriIdx]; OutCent.Z = TriCentZ[TriIdx];
		OutRadiusSq = TriRadiusSq[TriIdx];
		return true;
	}

private:
	struct FTriRef { int32 Index; }; // 삼각형 인덱스(0..NumTris-1)

	struct FNode
	{
		FAABB Bounds;
		int32 Left = -1;
		int32 Right = -1;
		int32 Start = 0; // leaf: TriRefs[start..start+count)
		int32 Count = 0;
		bool IsLeaf() const { return Count > 0; }
	};

	TArray<FNode>   Nodes;
	TArray<FTriRef> TriRefs;        // 삼각형 인덱스 배열(빌드 중 파티션)
	TArray<FAABB>   TriBounds;      // 각 삼각형의 AABB
	const TArray<FNormalVertex>* Vtx = nullptr;
	const TArray<uint32>* Idx = nullptr;
	int32 NumTris = 0;

	// SAH 보조: AABB 표면적
	static float SurfaceArea(const FAABB& B);

	// 빌드 보조
	int32 BuildRecursive(int32* TriIndexPtr, int32 Count);
	FAABB ComputeTriBounds(int32 TriIdx) const;

	// 레이/AABB
	struct FRayCached { float Origin[3]; float InvDir[3]; };
	static bool RayIntersectsAABB(const FRayCached& Ray, const FAABB& Box, float& OutTMin, float& OutTMax);

	// 모델 공간 레이 캐싱
	static inline FRayCached CacheRay(const FRay& R)
	{
		FRayCached C{
			{ R.Origin.X, R.Origin.Y, R.Origin.Z },
			{ 0,0,0 }
		};
		C.InvDir[0] = (fabsf(R.Direction.X) > 1e-8f) ? 1.0f / R.Direction.X : std::numeric_limits<float>::infinity();
		C.InvDir[1] = (fabsf(R.Direction.Y) > 1e-8f) ? 1.0f / R.Direction.Y : std::numeric_limits<float>::infinity();
		C.InvDir[2] = (fabsf(R.Direction.Z) > 1e-8f) ? 1.0f / R.Direction.Z : std::numeric_limits<float>::infinity();
		return C;
	}

	mutable TArray<float> TriV0X, TriV0Y, TriV0Z;
	mutable TArray<float> TriE1X, TriE1Y, TriE1Z;
	mutable TArray<float> TriE2X, TriE2Y, TriE2Z;
	mutable TArray<float> TriCentX, TriCentY, TriCentZ;
	mutable TArray<float> TriRadiusSq;

	void EnsureTriSoASize() const;
};
