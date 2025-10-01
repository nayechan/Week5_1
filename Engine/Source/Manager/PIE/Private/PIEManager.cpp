#include "pch.h"
#include "Manager/PIE/Public/PIEManager.h"
#include "Manager/Level/Public/LevelManager.h"
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

	if (PIELevel)
	{
		UE_LOG("PIEManager: PIE already running");
		return;
	}

	UE_LOG("PIEManager: Starting PIE");

	// LevelManager로부터 Level 복제
	PIELevel = ULevelManager::GetInstance().CloneLevelForPIE();

	if (PIELevel)
	{
		PIELevel->Init();
		PIEViewport = InPIEViewport;

		// Viewport에 PIE Level 할당
		PIEViewport->RenderTargetLevel = PIELevel.get();

		UE_LOG("PIEManager: PIE Started Successfully");
	}
	else
	{
		UE_LOG("PIEManager: Failed to start PIE");
	}
}

void UPIEManager::StopPIE()
{
	if (!PIELevel)
	{
		UE_LOG("PIEManager: PIE not running");
		return;
	}

	UE_LOG("PIEManager: Stopping PIE");

	// Viewport의 RenderTargetLevel 초기화
	if (PIEViewport)
	{
		PIEViewport->RenderTargetLevel = nullptr;
		PIEViewport = nullptr;
	}

	// PIE Level 삭제 (자동)
	PIELevel.reset();
	bIsPaused = false;

	UE_LOG("PIEManager: PIE Stopped");
}

void UPIEManager::TogglePausePIE()
{
	if (!PIELevel)
	{
		UE_LOG("PIEManager: PIE not running, cannot pause");
		return;
	}

	bIsPaused = !bIsPaused;

	if (bIsPaused)
	{
		// 일시정지: Viewport에서 PIE Level 제거 (Editor Level 표시)
		if (PIEViewport)
		{
			PIEViewport->RenderTargetLevel = nullptr;
		}
		UE_LOG("PIEManager: PIE Paused");
	}
	else
	{
		// 재개: Viewport에 PIE Level 다시 할당
		if (PIEViewport)
		{
			PIEViewport->RenderTargetLevel = PIELevel.get();
		}
		UE_LOG("PIEManager: PIE Resumed");
	}
}
