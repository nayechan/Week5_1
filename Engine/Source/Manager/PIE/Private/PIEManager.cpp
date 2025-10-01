#include "pch.h"
#include "Manager/PIE/Public/PIEManager.h"
#include "Manager/World/Public/WorldManager.h"
#include "Core/Public/World.h"
#include "Level/Public/Level.h"
#include "Editor/Public/ViewportClient.h"
#include "Editor/Public/Viewport.h"
#include "Render/Renderer/Public/Renderer.h"

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

	UE_LOG("PIEManager: Editor World found: %s", EditorWorld->GetName().ToString().data());
	if (EditorWorld->GetLevel())
	{
		// 처음 5개 에디터 액터의 위치 확인
		for (size_t i = 0; i < EditorWorld->GetLevel()->GetActors().size() && i < 5; ++i)
		{
			AActor* Actor = EditorWorld->GetLevel()->GetActors()[i];
			if (Actor)
			{
				FVector ActorLocation = Actor->GetActorLocation();
			}
		}
	}
	else
	{
		UE_LOG("PIEManager: Editor World has no level!");
	}


	PIEWorld = UWorld::DuplicateWorldForPIE(EditorWorld);

	if (PIEWorld)
	{
		PIEWorld->InitializeActorsForPlay();
		PIEViewport = InPIEViewport;

		// PIE 시작 전 Editor 카메라 설정을 PIE 카메라에 복사
		FViewport* EditorViewport = URenderer::GetInstance().GetViewportClient();
		if (EditorViewport)
		{
			FViewportClient* ActiveEditorViewportClient = EditorViewport->GetActiveViewportClient();
			if (ActiveEditorViewportClient && !ActiveEditorViewportClient->RenderTargetWorld)
			{
				// Editor 카메라의 설정을 PIE 카메라에 복사
				UCamera& EditorCamera = ActiveEditorViewportClient->Camera;
				UCamera& PIECamera = InPIEViewport->Camera;
				
				PIECamera.SetLocation(EditorCamera.GetLocation());
				PIECamera.SetRotation(EditorCamera.GetRotation());
				PIECamera.SetCameraType(EditorCamera.GetCameraType());
				PIECamera.SetFovY(EditorCamera.GetFovY());
				PIECamera.SetNearZ(EditorCamera.GetNearZ());
				PIECamera.SetFarZ(EditorCamera.GetFarZ());
				PIECamera.SetOrthoWidth(EditorCamera.GetOrthoWidth());
				PIECamera.SetMoveSpeed(EditorCamera.GetMoveSpeed());
				
				// ViewportClient 카메라 타입도 복사
				InPIEViewport->SetCameraType(ActiveEditorViewportClient->GetCameraType());
			}
		}
		
		// Viewport에 PIE World 할당
		PIEViewport->RenderTargetWorld = PIEWorld.Get();
		UE_LOG("PIE 모드 ON");
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

void UPIEManager::Update(float DeltaTime)
{
	// PIE가 실행 중이고 일시정지 상태가 아닐 때만 PIE World Tick
	if (PIEWorld && !bIsPaused)
	{
		PIEWorld->Tick(DeltaTime);
	}
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
