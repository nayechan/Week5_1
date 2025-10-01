#include "pch.h"
#include "Render/Culling/Public/CullingManager.h"
#include "Render/Culling/Public/FrustumCuller.h"
#include "Render/Culling/Public/OcclusionCuller.h"
#include "Render/Culling/Public/LODManager.h"
#include "Render/Renderer/Public/DeviceResources.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UCullingManager)

UCullingManager::UCullingManager() = default;
UCullingManager::~UCullingManager() = default;

void UCullingManager::Initialize(UDeviceResources* InDeviceResources)
{
	// Singleton 인스턴스 가져오기
	FrustumCuller = &UFrustumCuller::GetInstance();
	OcclusionCuller = &UOcclusionCuller::GetInstance();
	LODManager = &ULODManager::GetInstance();

	// Occlusion Culler 초기화 (Device Resources 필요)
	OcclusionCuller->Initialize(InDeviceResources);

	// LOD 설정 로드
	LODManager->LoadSettings();

	UE_LOG("CullingManager 초기화 완료");
}

void UCullingManager::Release()
{
	if (OcclusionCuller)
	{
		OcclusionCuller->Release();
	}

	UE_LOG("CullingManager 해제 완료");
}

void UCullingManager::OnResize()
{
	if (OcclusionCuller)
	{
		OcclusionCuller->ReleaseResources();
		OcclusionCuller->OnResize();
	}
}

void UCullingManager::LoadSettings()
{
	if (LODManager)
	{
		LODManager->LoadSettings();
	}
}
