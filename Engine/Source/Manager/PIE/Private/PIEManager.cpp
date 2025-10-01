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
		UE_LOG("PIEManager: Editor World has %zu actors", EditorWorld->GetLevel()->GetActors().size());
		// 처음 5개 에디터 액터의 위치 확인
		for (size_t i = 0; i < EditorWorld->GetLevel()->GetActors().size() && i < 5; ++i)
		{
			AActor* Actor = EditorWorld->GetLevel()->GetActors()[i];
			if (Actor)
			{
				FVector ActorLocation = Actor->GetActorLocation();
				UE_LOG("PIEManager: - Editor Actor[%zu]: %s, Pos(%.2f,%.2f,%.2f)", 
				       i, Actor->GetName().ToString().data(),
				       ActorLocation.X, ActorLocation.Y, ActorLocation.Z);
			}
		}
	}
	else
	{
		UE_LOG("PIEManager: Editor World has no level!");
	}

	// Editor World를 복제하여 PIE World 생성
	UE_LOG("PIEManager: Starting world duplication...");
	PIEWorld = UWorld::DuplicateWorldForPIE(EditorWorld);
	UE_LOG("PIEManager: World duplication completed");

	if (PIEWorld)
	{
		UE_LOG("PIEManager: PIE World created: %s", PIEWorld->GetName().ToString().data());
		if (PIEWorld->GetLevel())
		{
			UE_LOG("PIEManager: PIE World has %zu actors", PIEWorld->GetLevel()->GetActors().size());
			for (size_t i = 0; i < PIEWorld->GetLevel()->GetActors().size(); ++i)
			{
				AActor* Actor = PIEWorld->GetLevel()->GetActors()[i];
				if (Actor)
				{
					UE_LOG("PIEManager: - Actor[%zu]: %s", i, Actor->GetName().ToString().data());
				}
			}
		}
		else
		{
			UE_LOG("PIEManager: PIE World has no level!");
		}

		UE_LOG("PIEManager: Initializing actors for play...");
		PIEWorld->InitializeActorsForPlay();
		PIEViewport = InPIEViewport;

	// PIE 시작 전 Editor 카메라 설정을 PIE 카메라에 복사
	UE_LOG("PIEManager: Copying editor camera settings to PIE camera...");
	// 현재 Editor World에서 사용 중인 에디터 카메라 찾기
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
			
			UE_LOG("PIEManager: Camera settings copied - Pos(%.2f,%.2f,%.2f) Rot(%.2f,%.2f,%.2f) Type:%s",
			       PIECamera.GetLocation().X, PIECamera.GetLocation().Y, PIECamera.GetLocation().Z,
			       PIECamera.GetRotation().X, PIECamera.GetRotation().Y, PIECamera.GetRotation().Z,
			       ClientCameraTypeToString(InPIEViewport->GetCameraType()));
		}
		else
		{
			UE_LOG("PIEManager: No active editor viewport client found for camera sync");
		}
	}
	else
	{
		UE_LOG("PIEManager: No editor viewport found for camera sync");
	}

	// Viewport에 PIE World 할당
	UE_LOG("PIEManager: Assigning PIE World to viewport...");
	PIEViewport->RenderTargetWorld = PIEWorld.Get();
		
		// PIE World 상태 자세히 확인
		if (PIEWorld->GetLevel())
		{
			ULevel* PIELevel = PIEWorld->GetLevel();
			UE_LOG("PIEManager: PIE Level has %zu LevelPrimitiveComponents", PIELevel->GetLevelPrimitiveComponents().size());
			UE_LOG("PIEManager: PIE Level ShowFlags: %llu", PIELevel->GetShowFlags());
			
			// 각 Actor의 컴포넌트 상태 및 위치 확인
			for (size_t i = 0; i < PIELevel->GetActors().size() && i < 10; ++i) // 처음 10개 확인
			{
				AActor* Actor = PIELevel->GetActors()[i];
				if (Actor)
				{
					FVector ActorLocation = Actor->GetActorLocation();
					UE_LOG("PIEManager: - PIE Actor[%zu]: %s, Pos(%.2f,%.2f,%.2f), Components: %zu", 
					       i, Actor->GetName().ToString().data(), 
					       ActorLocation.X, ActorLocation.Y, ActorLocation.Z,
					       Actor->GetOwnedComponents().size());
					       
					for (const auto& Component : Actor->GetOwnedComponents())
					{
						if (Component)
						{
							UE_LOG("PIEManager:   - Component: %s, Owner: %s", 
							       Component->GetName().ToString().data(), 
							       Component->GetOwner() ? Component->GetOwner()->GetName().ToString().data() : "null");
						}
					}
				}
			}
		}
		
		// Viewport 상태 확인
		UE_LOG("PIEManager: Viewport RenderTargetWorld set to: %s", 
		       PIEViewport->RenderTargetWorld ? PIEViewport->RenderTargetWorld->GetName().ToString().data() : "null");

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
