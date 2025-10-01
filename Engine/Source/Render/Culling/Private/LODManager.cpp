#include "pch.h"
#include "Render/Culling/Public/LODManager.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Manager/Config/Public/ConfigManager.h"

IMPLEMENT_SINGLETON_CLASS_BASE(ULODManager)

ULODManager::ULODManager() = default;
ULODManager::~ULODManager() = default;

void ULODManager::LoadSettings()
{
	// Config Manager를 통해 LOD 설정 로드
	UConfigManager& ConfigManager = UConfigManager::GetInstance();

	// LOD 활성화 설정
	bLODEnabled = ConfigManager.GetConfigValueBool("LODEnabled", true);

	// LOD 거리 설정 (일반 거리 값을 제곱해서 저장)
	float LODDistance0 = ConfigManager.GetConfigValueFloat("LODDistance0", 10.0f);
	float LODDistance1 = ConfigManager.GetConfigValueFloat("LODDistance1", 80.0f);

	LODDistanceSquared0 = LODDistance0 * LODDistance0;
	LODDistanceSquared1 = LODDistance1 * LODDistance1;

	UE_LOG("LOD Settings Loaded - Enabled: %s, Distance0: %.1f (%.0f), Distance1: %.1f (%.0f)",
		bLODEnabled ? "true" : "false",
		LODDistance0, LODDistanceSquared0,
		LODDistance1, LODDistanceSquared1);
}

__forceinline void ULODManager::UpdateLOD(UStaticMeshComponent* MeshComp, const FVector& CameraPos) const
{
	// 거리 제곱 계산 (매우 빠름)
	const FVector ObjPos = MeshComp->GetRelativeLocation();
	const float dx = CameraPos.X - ObjPos.X;
	const float dy = CameraPos.Y - ObjPos.Y;
	const float dz = CameraPos.Z - ObjPos.Z;
	const float DistSq = dx * dx + dy * dy + dz * dz;

	// Config 기반 LOD 선택
	int NewLOD;
	if (!bLODEnabled)
	{
		NewLOD = 0;  // LOD 비활성화 시 최고 품질
	}
	else if (DistSq > LODDistanceSquared1)
		NewLOD = 2;
	else if (DistSq > LODDistanceSquared0)
		NewLOD = 1;
	else
		NewLOD = 0;

	// LOD 변경이 필요할 때만 업데이트
	if (MeshComp->GetCurrentLODIndex() != NewLOD)
	{
		MeshComp->SetCurrentLODIndex(NewLOD);

		// 통계 업데이트
		Stats.LODUpdatesPerFrame++;
		switch (NewLOD)
		{
		case 0: Stats.LOD0Count++; break;
		case 1: Stats.LOD1Count++; break;
		case 2: Stats.LOD2Count++; break;
		}
	}
}

void ULODManager::UpdateLODBatch(UStaticMeshComponent** MeshComps, int Count, const FVector& CameraPos) const
{
	// SIMD 최적화된 배치 처리는 향후 구현 가능
	for (int i = 0; i < Count; ++i)
	{
		UpdateLOD(MeshComps[i], CameraPos);
	}
}

void ULODManager::ResetStats()
{
	Stats.LODUpdatesPerFrame = 0;
	Stats.LODUpdateTimeMs = 0.0f;
	Stats.LOD0Count = 0;
	Stats.LOD1Count = 0;
	Stats.LOD2Count = 0;
}
