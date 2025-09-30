#pragma once
#include "Core/Public/Object.h"

struct FFrustum;
struct FAABB;
class UCamera;

/**
 * @brief Frustum Culling 관리 클래스
 *
 * 카메라의 view frustum과 객체의 AABB를 테스트하여
 * 화면에 보이지 않는 객체를 빠르게 제거합니다.
 */
UCLASS()
class UFrustumCuller : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UFrustumCuller, UObject)

public:
	/**
	 * @brief AABB가 Frustum 내부에 있는지 테스트합니다.
	 * @param Bounds 테스트할 AABB
	 * @param Frustum 카메라 Frustum
	 * @return Frustum 내부에 있으면 true
	 */
	bool IsInFrustum(const FAABB& Bounds, const FFrustum& Frustum) const;

	/**
	 * @brief 월드 공간 AABB가 Frustum 내부에 있는지 테스트합니다.
	 * @param WorldMin AABB 최소 좌표
	 * @param WorldMax AABB 최대 좌표
	 * @param Frustum 카메라 Frustum
	 * @return Frustum 내부에 있으면 true
	 */
	bool IsInFrustum(const FVector& WorldMin, const FVector& WorldMax, const FFrustum& Frustum) const;
};