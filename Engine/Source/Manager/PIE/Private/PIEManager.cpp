#include "pch.h"
#include "Manager/PIE/Public/PIEManager.h"
#include "Manager/World/Public/WorldManager.h"
#include "Core/Public/World.h"
#include "Level/Public/Level.h"
#include "Editor/Public/ViewportClient.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UPIEManager)

UPIEManager::UPIEManager() = default;

UPIEManager::~UPIEManager() = default;

void UPIEManager::StartPIE(FViewportClient* InPIEViewport)
{
	if (!InPIEViewport)
	{
		UE_LOG("PIEManager: Invalid viewport provided");
		return;
	}

	if (PIEWorld)
	{
		UE_LOG("PIEManager: PIE already running");
		return;
	}

	UE_LOG("PIEManager: Starting PIE");

	// WorldManager로부터 Editor World 가져오기
	UWorld* EditorWorld = UWorldManager::GetInstance().GetCurrentWorld().Get();
	if (!EditorWorld)
	{
		UE_LOG("PIEManager: No Editor World found");
		return;
	}

	// Editor World를 복제하여 PIE World 생성
	PIEWorld = UWorld::DuplicateWorldForPIE(EditorWorld);

	if (PIEWorld)
	{
		// World가 Level 초기화 및 Actor BeginPlay 모두 처리
		PIEWorld->InitializeActorsForPlay();
		PIEViewport = InPIEViewport;

		// Viewport에 PIE World 할당
		PIEViewport->RenderTargetWorld = PIEWorld.Get();

		UE_LOG("PIEManager: PIE Started Successfully");
	}
	else
	{
		UE_LOG("PIEManager: Failed to start PIE");
	}
}

void UPIEManager::StopPIE()
{
	if (!PIEWorld)
	{
		UE_LOG("PIEManager: PIE not running");
		return;
	}

	UE_LOG("PIEManager: Stopping PIE");

	// Viewport의 RenderTargetWorld 초기화
	if (PIEViewport)
	{
		PIEViewport->RenderTargetWorld = nullptr;
		PIEViewport = nullptr;
	}

	// PIE World 정리 및 삭제
	PIEWorld->CleanupWorld();
	delete PIEWorld.Get();
	PIEWorld = nullptr;
	bIsPaused = false;

	UE_LOG("PIEManager: PIE Stopped");
}

void UPIEManager::TogglePausePIE()
{
	if (!PIEWorld)
	{
		UE_LOG("PIEManager: PIE not running, cannot pause");
		return;
	}

	bIsPaused = !bIsPaused;

	if (bIsPaused)
	{
		// 일시정지: Viewport에서 PIE World 제거 (Editor World 표시)
		if (PIEViewport)
		{
			PIEViewport->RenderTargetWorld = nullptr;
		}
		UE_LOG("PIEManager: PIE Paused");
	}
	else
	{
		// 재개: Viewport에 PIE World 다시 할당
		if (PIEViewport)
		{
			PIEViewport->RenderTargetWorld = PIEWorld.Get();
		}
		UE_LOG("PIEManager: PIE Resumed");
	}
}
