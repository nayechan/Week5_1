#pragma once
#include "Core/Public/Object.h"

class UStaticMeshComponent;
class UConfigManager;

/**
 * @brief LOD (Level of Detail) 관리 클래스
 *
 * 카메라와의 거리에 따라 메시의 LOD 레벨을 동적으로 조정하여 렌더링 성능을 최적화합니다.
 */
UCLASS()
class ULODManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(ULODManager, UObject)

public:
	/**
	 * @brief 설정 파일에서 LOD 관련 파라미터를 로드합니다.
	 */
	void LoadSettings();

	/**
	 * @brief 단일 메시의 LOD를 업데이트합니다.
	 * @param MeshComp 업데이트할 메시 컴포넌트
	 * @param CameraPos 카메라 위치
	 */
	__forceinline void UpdateLOD(UStaticMeshComponent* MeshComp, const FVector& CameraPos) const;

	/**
	 * @brief 여러 메시의 LOD를 배치로 업데이트합니다 (SIMD 최적화 버전).
	 * @param MeshComps 업데이트할 메시 컴포넌트 배열
	 * @param Count 메시 개수
	 * @param CameraPos 카메라 위치
	 */
	void UpdateLODBatch(UStaticMeshComponent** MeshComps, int Count, const FVector& CameraPos) const;

	/**
	 * @brief LOD 통계 정보를 초기화합니다.
	 */
	void ResetStats();

	/**
	 * @brief 현재 프레임의 LOD 통계를 가져옵니다.
	 */
	struct FLODStats
	{
		uint32 LODUpdatesPerFrame = 0;
		float LODUpdateTimeMs = 0.0f;
		uint32 LOD0Count = 0;
		uint32 LOD1Count = 0;
		uint32 LOD2Count = 0;
	};

	const FLODStats& GetStats() const { return Stats; }

	// Getters
	bool IsLODEnabled() const { return bLODEnabled; }
	float GetLODDistanceSquared0() const { return LODDistanceSquared0; }
	float GetLODDistanceSquared1() const { return LODDistanceSquared1; }

private:
	// LOD 설정
	bool bLODEnabled = true;
	float LODDistanceSquared0 = 100.0f;   // 10^2 = 100
	float LODDistanceSquared1 = 6400.0f;  // 80^2 = 6400

	// 통계
	mutable FLODStats Stats;
};