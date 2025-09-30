#pragma once
#include "Core/Public/Object.h"

class UFrustumCuller;
class UOcclusionCuller;
class ULODManager;
class UDeviceResources;
class UCamera;

/**
 * @brief 모든 Culling 시스템을 통합 관리하는 매니저 클래스
 *
 * Frustum Culling, Occlusion Culling, LOD 관리를 통합하여
 * 렌더링 최적화를 위한 단일 인터페이스를 제공합니다.
 */
UCLASS()
class UCullingManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UCullingManager, UObject)

public:
	/**
	 * @brief Culling 시스템을 초기화합니다.
	 * @param InDeviceResources Direct3D 디바이스 리소스
	 */
	void Initialize(UDeviceResources* InDeviceResources);

	/**
	 * @brief Culling 시스템을 해제합니다.
	 */
	void Release();

	/**
	 * @brief 화면 크기 변경 시 호출됩니다.
	 */
	void OnResize();

	/**
	 * @brief 설정 파일에서 culling 관련 파라미터를 로드합니다.
	 */
	void LoadSettings();

	// Getters
	UFrustumCuller* GetFrustumCuller() const { return FrustumCuller; }
	UOcclusionCuller* GetOcclusionCuller() const { return OcclusionCuller; }
	ULODManager* GetLODManager() const { return LODManager; }

private:
	UFrustumCuller* FrustumCuller = nullptr;
	UOcclusionCuller* OcclusionCuller = nullptr;
	ULODManager* LODManager = nullptr;
};