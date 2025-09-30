#include "pch.h"
#include "Render/Spatial/Public/CullingStrategy.h"
#include "Render/Spatial/Public/Frustum.h"
#include "Render/Culling/Public/OcclusionCuller.h"
#include "Editor/Public/Camera.h"

// FFrustumCullingStrategy Implementation
bool FFrustumCullingStrategy::ShouldCullNode(const FAABB& NodeBounds) const
{
	// Frustum 밖이면 컬링
	return !Frustum.IsBoxInFrustum(NodeBounds);
}

bool FFrustumCullingStrategy::ShouldCullPrimitive(const FAABB& PrimBounds) const
{
	// Frustum 밖이면 컬링
	return !Frustum.IsBoxInFrustum(PrimBounds);
}

// FFrustumWithOcclusionStrategy Implementation
bool FFrustumWithOcclusionStrategy::ShouldCullNode(const FAABB& NodeBounds) const
{
	// 1. Frustum 테스트
	if (!Frustum.IsBoxInFrustum(NodeBounds))
	{
		return true;  // Frustum 밖이면 즉시 컬링
	}

	// 2. Occlusion 테스트 (활성화된 경우)
	if (bUseOcclusion)
	{
		// 노드는 보수적으로 판단 (ExpansionFactor 적용)
		bool bIsOccluded = OcclusionCuller->IsOccluded(
			NodeBounds.Min, NodeBounds.Max, Camera, ExpansionFactor);

		if (bIsOccluded)
		{
			return true;  // 가려진 노드
		}
	}

	return false;  // 컬링하지 않음
}

bool FFrustumWithOcclusionStrategy::ShouldCullPrimitive(const FAABB& PrimBounds) const
{
	// 1. Frustum 테스트
	if (!Frustum.IsBoxInFrustum(PrimBounds))
	{
		return true;
	}

	// 2. Primitive 레벨에서는 occlusion 테스트를 하지 않음
	//    (노드 레벨에서 이미 처리되었으므로)

	return false;
}