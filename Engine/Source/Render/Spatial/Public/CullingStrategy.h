#pragma once
#include "Physics/Public/AABB.h"

class UCamera;
class UOcclusionCuller;
struct FFrustum;

/**
 * @brief Culling 전략을 정의하는 추상 인터페이스
 *
 * Octree가 컬링 시스템의 구체적인 구현을 몰라도 되도록
 * Strategy Pattern을 적용한 인터페이스입니다.
 */
struct ICullingStrategy
{
	virtual ~ICullingStrategy() = default;

	/**
	 * @brief 노드 레벨에서 컬링 여부를 판단합니다.
	 * @param NodeBounds 노드의 경계 박스
	 * @return true면 노드와 모든 자식을 스킵
	 */
	virtual bool ShouldCullNode(const FAABB& NodeBounds) const = 0;

	/**
	 * @brief Primitive 레벨에서 컬링 여부를 판단합니다.
	 * @param PrimBounds Primitive의 경계 박스
	 * @return true면 해당 Primitive 스킵
	 */
	virtual bool ShouldCullPrimitive(const FAABB& PrimBounds) const = 0;
};

/**
 * @brief 컬링을 하지 않는 전략 (테스트/디버그용)
 */
struct FNoCullingStrategy : public ICullingStrategy
{
	bool ShouldCullNode(const FAABB& NodeBounds) const override { return false; }
	bool ShouldCullPrimitive(const FAABB& PrimBounds) const override { return false; }
};

/**
 * @brief Frustum Culling만 수행하는 전략
 */
class FFrustumCullingStrategy : public ICullingStrategy
{
public:
	explicit FFrustumCullingStrategy(const FFrustum& InFrustum)
		: Frustum(InFrustum)
	{
	}

	bool ShouldCullNode(const FAABB& NodeBounds) const override;
	bool ShouldCullPrimitive(const FAABB& PrimBounds) const override;

private:
	const FFrustum& Frustum;
};

/**
 * @brief Frustum + Occlusion Culling을 수행하는 전략
 *
 * 노드 레벨에서 n-프레임 연속 occlusion 시스템을 사용합니다.
 */
class FFrustumWithOcclusionStrategy : public ICullingStrategy
{
public:
	FFrustumWithOcclusionStrategy(const FFrustum& InFrustum,
	                              UOcclusionCuller* InOcclusionCuller,
	                              const UCamera* InCamera,
	                              float InExpansionFactor = 2.5f)
		: Frustum(InFrustum)
		, OcclusionCuller(InOcclusionCuller)
		, Camera(InCamera)
		, ExpansionFactor(InExpansionFactor)
		, bUseOcclusion(InOcclusionCuller != nullptr && InCamera != nullptr)
	{
	}

	bool ShouldCullNode(const FAABB& NodeBounds) const override;
	bool ShouldCullPrimitive(const FAABB& PrimBounds) const override;

	bool IsOcclusionEnabled() const { return bUseOcclusion; }

private:
	const FFrustum& Frustum;
	UOcclusionCuller* OcclusionCuller;
	const UCamera* Camera;
	float ExpansionFactor;
	bool bUseOcclusion;
};