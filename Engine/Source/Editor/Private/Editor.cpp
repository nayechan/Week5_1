#include "pch.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Viewport.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/UI/Widget/Public/FPSWidget.h"
#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"
#include "Render/UI/Widget/Public/SplitterDebugWidget.h"
#include "Render/UI/Widget/Public/CameraControlWidget.h"
#include "Render/UI/Widget/Public/ViewportMenuBarWidget.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/TextRenderComponent.h"
#include "Level/Public/Level.h"
#include "Global/Quaternion.h"
#include "Utility/Public/ScopeCycleCounter.h"
#include "Utility/Public/SceneBVH.h"
#include "Source/Core/Public/World.h"

UEditor::UEditor()
{
	const TArray<float>& SplitterRatio = UConfigManager::GetInstance().GetSplitterRatio();

	// ConfigManager에서 불러온 비율을 먼저 Saved 비율로 저장 (멀티모드 복원용)
	SavedRootRatio = SplitterRatio[0];
	SavedLeftRatio = SplitterRatio[1];
	SavedRightRatio = SplitterRatio[2];

	// 스플리터에도 적용
	RootSplitter.SetRatio(SplitterRatio[0]);
	LeftSplitter.SetRatio(SplitterRatio[1]);
	RightSplitter.SetRatio(SplitterRatio[2]);

	auto& UIManager = UUIManager::GetInstance();

	if (auto* FPSWidget = reinterpret_cast<UFPSWidget*>(UIManager.FindWidget("FPS Widget")))
	{
		FPSWidget->SetBatchLine(&BatchLines);
	}

	// Splitter UI에게 Splitter 정보를 전달합니다.
	if (auto* SplitterWidget = reinterpret_cast<USplitterDebugWidget*>(UIManager.FindWidget("Splitter Widget")))
	{
		SplitterWidget->SetSplitters(&RootSplitter, &LeftSplitter, &RightSplitter);
	}

	if (auto* ViewportWidget = reinterpret_cast<UViewportMenuBarWidget*>(UIManager.FindWidget("ViewportMenuBar Widget")))
	{
		ViewportWidget->SetEdtior(this);
	}

	InitializeLayout();
	// 초기 상태를 싱글 뷰포트 모드로 설정 (첫 번째 뷰포트 표시)
	// TODO: ConfigManager에서 초기 뷰포트 모드 설정을 읽어오도록 개선 필요
	// 저장된 멀티모드 비율은 유지하면서 싱글모드로 시작

	// 생성 시점에는 ViewportLayoutState를 Single로 미리 설정하여
	// SetSingleViewportLayout에서 SavedRatio를 덮어쓰지 않도록 함
	ViewportLayoutState = EViewportLayoutState::Single;

	SetSingleViewportLayout(0);

	// 생성 시점에는 애니메이션 없이 즉시 싱글 모드로 설정
	TargetViewportLayoutState = EViewportLayoutState::Single;

	// 스플리터 비율을 즉시 싱글 모드 값으로 설정 (애니메이션 스킵)
	RootSplitter.SetRatio(TargetRootRatio);
	LeftSplitter.SetRatio(TargetLeftRatio);
	RightSplitter.SetRatio(TargetRightRatio);

	// SourceRatio도 현재 Target 값으로 설정하여 다음 애니메이션의 시작점을 명확히 함
	SourceRootRatio = TargetRootRatio;
	SourceLeftRatio = TargetLeftRatio;
	SourceRightRatio = TargetRightRatio;
}

UEditor::~UEditor()
{
	// 전체화면 모드인 경우 백업된 4분할 비율을 저장, 아니면 현재 비율 저장
	if (ViewportLayoutState == EViewportLayoutState::Single)
	{
		UConfigManager::GetInstance().SetSplitterRatio(SavedRootRatio, SavedLeftRatio, SavedRightRatio);
	}
	else
	{
		UConfigManager::GetInstance().SetSplitterRatio(RootSplitter.GetRatio(), LeftSplitter.GetRatio(), RightSplitter.GetRatio());
	}
	SafeDelete(DraggedSplitter);
	SafeDelete(InteractionViewport);
}

void UEditor::Update()
{
	URenderer& Renderer = URenderer::GetInstance();
	FViewport* Viewport = Renderer.GetViewportClient();
	UInputManager& InputManager = UInputManager::GetInstance();

	// CRITICAL: Update all world transforms BEFORE any picking or BVH operations
	// Level::Update() is called AFTER Editor::Update(), so we must update transforms here first
	ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
	if (CurrentLevel)
	{
		for (auto& Actor : CurrentLevel->GetActors())
		{
			if (Actor && Actor->GetRootComponent())
			{
				Actor->GetRootComponent()->UpdateWorldTransform();
			}
		}
	}

	// 뷰포트 레이아웃은 PIE 모드와 관계없이 항상 업데이트
	// (창 크기 변경, 스플리터 드래그, 뷰포트 전환 애니메이션 등)
	UpdateLayout();

	// 1. 마우스 위치를 기반으로 활성 뷰포트를 결정합니다.
	// Active Viewport는 클릭할 때만 변경 (마우스 호버만으로는 변경 안 됨)
	// PIE 모드와 관계없이 항상 업데이트되어야 함
	bool bMouseOverImGui = ImGui::GetIO().WantCaptureMouse || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
	bool bMouseClicked = InputManager.IsKeyPressed(EKeyInput::MouseLeft) || InputManager.IsKeyPressed(EKeyInput::MouseRight);
	if (!bMouseOverImGui && bMouseClicked)
	{
		FViewportClient* PreviousActive = Viewport->GetActiveViewportClient();
		Viewport->UpdateActiveViewportClient(UInputManager::GetInstance().GetMousePosition());
		FViewportClient* CurrentActive = Viewport->GetActiveViewportClient();

		// Active viewport가 변경되었을 때만 로그
		if (PreviousActive != CurrentActive && CurrentActive != nullptr)
		{
			UE_LOG("Editor: Active Viewport Changed (Address: %p)", CurrentActive);
		}
	}

	// 마우스 입력 처리는 항상 호출 (내부에서 PIE 체크)
	// 기즈모 호버는 현재 마우스 위치의 뷰포트 기준으로 동작해야 하므로
	ProcessMouseInput(ULevelManager::GetInstance().GetCurrentLevel());

	// Active Viewport가 PIE 모드인 경우, 나머지 에디터 기능 스킵
	FViewportClient* ActiveViewportClient = Viewport->GetActiveViewportClient();
	bool bIsPIEViewport = ActiveViewportClient && ActiveViewportClient->RenderTargetWorld && ActiveViewportClient->RenderTargetWorld->IsPIEWorld();
	if (bIsPIEViewport)
	{
		return;
	}

	// 이하 에디터 전용 기능들 (PIE 모드에서는 실행되지 않음)

	// 2. 활성 뷰포트의 카메라 제어를 업데이트합니다.
	if (UCamera* ActiveCamera = Viewport->GetActiveCamera())
	{
		// ✨ 만약 이동량이 있고, 직교 카메라라면 ViewportClient에 알립니다.
		const FVector MovementDelta = ActiveCamera->UpdateInput();
		if (MovementDelta.LengthSquared() > 0.f && ActiveCamera->GetCameraType() == ECameraType::ECT_Orthographic)
		{
			Viewport->UpdateOrthoFocusPointByDelta(MovementDelta);
		}
	}

	// 3. 선택된 액터의 BoundingBox 업데이트
	if (AActor* SelectedActor = ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor())
	{
		for (const auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
			{
				FVector WorldMin, WorldMax;
				PrimitiveComponent->GetWorldAABB(WorldMin, WorldMax);

				uint64 ShowFlags = ULevelManager::GetInstance().GetCurrentLevel()->GetShowFlags();

				if ((ShowFlags & EEngineShowFlags::SF_Primitives) && (ShowFlags & EEngineShowFlags::SF_Bounds))
				{
					BatchLines.UpdateBoundingBoxVertices(FAABB(WorldMin, WorldMax));
				}
				else
				{
					BatchLines.UpdateBoundingBoxVertices({ { 0.0f,0.0f,0.0f }, { 0.0f, 0.0f, 0.0f } });
				}
			}
		}
	}
	else
	{
		BatchLines.DisableRenderBoundingBox();
	}

	BatchLines.UpdateVertexBuffer();

	EnsureBVHUpToDate(ULevelManager::GetInstance().GetCurrentLevel());
}

void UEditor::RenderEditor(UCamera* InCamera)
{
	BatchLines.Render();
	Axis.Render();

	// Render Gizmo & Billboard
	if (InCamera)
	{
		AActor* SelectedActor = ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor();
		if (!SelectedActor) return;

		Gizmo.RenderGizmo(SelectedActor, InCamera);

		// UUID Text Component
		if (UTextRenderComponent* UUIDTextComponent = SelectedActor->GetUUIDTextComponent())
		{
			URenderer::GetInstance().RenderText(UUIDTextComponent, InCamera);
		}
	}
}

void UEditor::SetSingleViewportLayout(int InActiveIndex)
{
	if (ViewportLayoutState == EViewportLayoutState::Animating) return;

	if (ViewportLayoutState == EViewportLayoutState::Multi)
	{
		SavedRootRatio = RootSplitter.GetRatio();
		SavedLeftRatio = LeftSplitter.GetRatio();
		SavedRightRatio = RightSplitter.GetRatio();
	}

	// 싱글 모드로 전환 시 드래그 상태 정리
	DraggedSplitter = nullptr;

	// 뷰포트 가시성 제어 - 지정된 뷰포트만 표시
	if (FViewport* ViewportClient = URenderer::GetInstance().GetViewportClient())
	{
		ViewportClient->SetSingleViewportMode(InActiveIndex);
	}

	// 스플리터 숨기기 (싱글 모드에서는 크기 조절 불필요)
	if (auto* SplitterWidget = reinterpret_cast<USplitterDebugWidget*>(UUIManager::GetInstance().FindWidget("Splitter Widget")))
	{
		SplitterWidget->SetVisible(false);
	}

	SourceRootRatio = RootSplitter.GetRatio();
	SourceLeftRatio = LeftSplitter.GetRatio();
	SourceRightRatio = RightSplitter.GetRatio();

	TargetRootRatio = SourceRootRatio;
	TargetLeftRatio = SourceLeftRatio;
	TargetRightRatio = SourceRightRatio;

	switch (InActiveIndex)
	{
	case 0: // 좌상단
		TargetRootRatio = 1.0f;
		TargetLeftRatio = 1.0f;
		break;
	case 1: // 좌하단
		TargetRootRatio = 1.0f;
		TargetLeftRatio = 0.0f;
		break;
	case 2: // 우상단
		TargetRootRatio = 0.0f;
		TargetRightRatio = 1.0f;
		break;
	case 3: // 우하단
		TargetRootRatio = 0.0f;
		TargetRightRatio = 0.0f;
		break;
	default:
		RestoreMultiViewportLayout();
		return;
	}

	ViewportLayoutState = EViewportLayoutState::Animating;
	TargetViewportLayoutState = EViewportLayoutState::Single;
	AnimationStartTime = UTimeManager::GetInstance().GetGameTime();

	// 애니메이션 완료 후 다음 애니메이션의 시작점으로 사용될 SourceRatio 업데이트
	// (애니메이션이 완료되면 UpdateLayout에서 이 값들이 사용됨)
}

/**
 * @brief 저장된 비율을 사용하여 4분할 뷰포트 레이아웃으로 복원합니다.
 */
void UEditor::RestoreMultiViewportLayout()
{
	if (ViewportLayoutState == EViewportLayoutState::Animating) return;

	// 뷰포트 가시성 제어 - 모든 뷰포트 표시
	if (FViewport* ViewportClient = URenderer::GetInstance().GetViewportClient())
	{
		ViewportClient->SetMultiViewportMode();
	}

	// 스플리터 다시 표시 (멀티 모드에서는 크기 조절 필요)
	if (auto* SplitterWidget = reinterpret_cast<USplitterDebugWidget*>(UUIManager::GetInstance().FindWidget("Splitter Widget")))
	{
		SplitterWidget->SetVisible(true);
	}

	// 싱글 모드에서는 SourceRatio가 이미 설정되어 있으므로 그대로 사용
	// (RootSplitter.GetRatio()는 Resize()에 의해 변경될 수 있음)
	// SourceRootRatio는 이미 생성자나 SetSingleViewportLayout에서 설정됨

	TargetRootRatio = SavedRootRatio;
	TargetLeftRatio = SavedLeftRatio;
	TargetRightRatio = SavedRightRatio;

	ViewportLayoutState = EViewportLayoutState::Animating;
	TargetViewportLayoutState = EViewportLayoutState::Multi;
	AnimationStartTime = UTimeManager::GetInstance().GetGameTime();
}

void UEditor::InitializeLayout()
{
	// 1. 루트 스플리터의 자식으로 2개의 수평 스플리터를 '주소'로 연결합니다.
	RootSplitter.SetChildren(&LeftSplitter, &RightSplitter);

	// 2. 각 수평 스플리터의 자식으로 뷰포트 윈도우들을 '주소'로 연결합니다.
	LeftSplitter.SetChildren(&ViewportWindows[0], &ViewportWindows[1]);
	RightSplitter.SetChildren(&ViewportWindows[2], &ViewportWindows[3]);

	// 3. 초기 레이아웃 계산
	const D3D11_VIEWPORT& ViewportInfo = URenderer::GetInstance().GetDeviceResources()->GetViewportInfo();
	FRect FullScreenRect = { ViewportInfo.TopLeftX, ViewportInfo.TopLeftY, ViewportInfo.Width, ViewportInfo.Height };
	RootSplitter.Resize(FullScreenRect);
}

void UEditor::UpdateLayout()
{
	URenderer& Renderer = URenderer::GetInstance();
	UInputManager& Input = UInputManager::GetInstance();
	const FPoint MousePosition = { Input.GetMousePosition().X, Input.GetMousePosition().Y };
	bool bIsHoveredOnSplitter = false;

	// 뷰포트를 전환 중이라면 애니메이션을 적용합니다.
	if (ViewportLayoutState == EViewportLayoutState::Animating)
	{
		float ElapsedTime = UTimeManager::GetInstance().GetGameTime() - AnimationStartTime;
		float Alpha = clamp(ElapsedTime / AnimationDuration, 0.0f, 1.0f);

		float NewRootRatio = Lerp(SourceRootRatio, TargetRootRatio, Alpha);
		float NewLeftRatio = Lerp(SourceLeftRatio, TargetLeftRatio, Alpha);
		float NewRightRatio = Lerp(SourceRightRatio, TargetRightRatio, Alpha);

		RootSplitter.SetRatio(NewRootRatio);
		LeftSplitter.SetRatio(NewLeftRatio);
		RightSplitter.SetRatio(NewRightRatio);

		if (Alpha >= 1.0f)
		{
			ViewportLayoutState = TargetViewportLayoutState;

			// 애니메이션 완료 시 SourceRatio를 현재 Target 값으로 업데이트
			// 다음 애니메이션의 시작점으로 사용됨
			SourceRootRatio = TargetRootRatio;
			SourceLeftRatio = TargetLeftRatio;
			SourceRightRatio = TargetRightRatio;
		}
	}

	// 싱글 뷰포트 모드가 아닐 때만 스플리터 상호작용 허용
	if (ViewportLayoutState != EViewportLayoutState::Single)
	{
		// 1. 드래그 상태가 아니라면 커서의 상태를 감지합니다.
		if (DraggedSplitter == nullptr)
		{
			if (LeftSplitter.IsHovered(MousePosition) || RightSplitter.IsHovered(MousePosition))
			{
				bIsHoveredOnSplitter = true;
			}
			else if (RootSplitter.IsHovered(MousePosition))
			{
				bIsHoveredOnSplitter = true;
			}
		}

		// 2. 스플리터 위에 커서가 있으며 클릭을 한다면, 드래그 상태로 활성화합니다.
		bool bMouseOverImGui = ImGui::GetIO().WantCaptureMouse || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
		if (!bMouseOverImGui && UInputManager::GetInstance().IsKeyPressed(EKeyInput::MouseLeft) && bIsHoveredOnSplitter)
		{
			// 호버 상태에 따라 드래그할 스플리터를 결정합니다.
			if (LeftSplitter.IsHovered(MousePosition)) { DraggedSplitter = &LeftSplitter; }				// 좌상, 좌하
			else if (RightSplitter.IsHovered(MousePosition)) { DraggedSplitter = &RightSplitter; }		// 우상, 우하
			else if (RootSplitter.IsHovered(MousePosition)) { DraggedSplitter = &RootSplitter; }
		}
	}

	// 3. 드래그 상태라면 스플리터 기능을 이행합니다.
	if (DraggedSplitter)
	{
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		FRect WorkableRect = { Viewport->WorkPos.x, Viewport->WorkPos.y, Viewport->WorkSize.x, Viewport->WorkSize.y };

		FRect ParentRect;

		if (DraggedSplitter == &RootSplitter)
		{
			ParentRect = WorkableRect;
		}
		else
		{
			if (DraggedSplitter == &LeftSplitter)
			{
				ParentRect.Left = WorkableRect.Left;
				ParentRect.Top = WorkableRect.Top;
				ParentRect.Width = WorkableRect.Width * RootSplitter.GetRatio();
				ParentRect.Height = WorkableRect.Height;
			}
			else if (DraggedSplitter == &RightSplitter)
			{
				ParentRect.Left = WorkableRect.Left + WorkableRect.Width * RootSplitter.GetRatio();
				ParentRect.Top = WorkableRect.Top;
				ParentRect.Width = WorkableRect.Width * (1.0f - RootSplitter.GetRatio());
				ParentRect.Height = WorkableRect.Height;
			}
		}

		// 마우스 위치를 부모 영역에 대한 비율(0.0 ~ 1.0)로 변환합니다.
		float NewRatio = 0.5f;
		if (dynamic_cast<SSplitterV*>(DraggedSplitter)) // 수직 스플리터
		{
			if (ParentRect.Width > 0)
			{
				NewRatio = (MousePosition.X - ParentRect.Left) / ParentRect.Width;
			}
		}
		else // 수평 스플리터
		{
			if (ParentRect.Height > 0)
			{
				NewRatio = (MousePosition.Y - ParentRect.Top) / ParentRect.Height;
			}
		}

		// 계산된 비율을 스플리터에 적용합니다.
		DraggedSplitter->SetRatio(NewRatio);
	}

	// 4. 매 프레임 현재 비율에 맞게 전체 레이아웃 크기를 다시 계산하고, 그 결과를 실제 FViewport에 반영합니다.
	const ImGuiViewport* Viewport = ImGui::GetMainViewport(); // 사용자에게만 보이는 영역의 정보를 가져옵니다. 
	FRect WorkableRect = { Viewport->WorkPos.x, Viewport->WorkPos.y, Viewport->WorkSize.x, Viewport->WorkSize.y };
	RootSplitter.Resize(WorkableRect);

	if (FViewport* ViewportClient = URenderer::GetInstance().GetViewportClient())
	{
		auto& Viewports = ViewportClient->GetViewports();

		// 싱글 뷰포트 모드일 때는 활성 뷰포트만 전체 화면으로 설정
		if (ViewportLayoutState == EViewportLayoutState::Single && ViewportClient->IsSingleViewportMode())
		{
			for (int i = 0; i < 4; ++i)
			{
				if (i < Viewports.size())
				{
					if (Viewports[i].bIsVisible)
					{
						// 활성 뷰포트는 전체 화면
						Viewports[i].SetViewportInfo({ WorkableRect.Left, WorkableRect.Top, WorkableRect.Width, WorkableRect.Height, 0.0f, 1.0f });
					}
					else
					{
						// 비활성 뷰포트는 크기 0으로 설정
						Viewports[i].SetViewportInfo({ 0, 0, 0, 0, 0.0f, 1.0f });
					}
				}
			}
		}
		else
		{
			// 멀티 뷰포트 모드일 때는 기존 스플리터 영역 사용
			for (int i = 0; i < 4; ++i)
			{
				if (i < Viewports.size())
				{
					const FRect& Rect = ViewportWindows[i].Rect;
					Viewports[i].SetViewportInfo({ Rect.Left, Rect.Top, Rect.Width, Rect.Height, 0.0f, 1.0f });
				}
			}
		}
	}

	// 마우스 클릭 해제를 하면 드래그 비활성화
	if (UInputManager::GetInstance().IsKeyReleased(EKeyInput::MouseLeft)) { DraggedSplitter = nullptr; }
}

void UEditor::MarkActorPrimitivesDirty(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	for (auto& C : InActor->GetOwnedComponents())
	{
		if (auto P = Cast<UPrimitiveComponent>(C))
		{
			PendingDirtyPrims.push_back(P);
		}
	}
}

void UEditor::MarkPrimitiveDirty(UPrimitiveComponent* InPrim)
{
	if (InPrim)
	{
		PendingDirtyPrims.push_back(InPrim);
	}
}

void UEditor::ProcessMouseInput(ULevel* InLevel)
{
	// 선택된 뷰포트의 정보들을 가져옵니다.
	FViewport* ViewportClient = URenderer::GetInstance().GetViewportClient();
	FViewportClient* CurrentViewport = nullptr;
	UCamera* CurrentCamera = nullptr;

	// 이미 드래그 중이라면 InteractionViewport를 유지
	if (InteractionViewport)
	{
		CurrentViewport = InteractionViewport;
	}
	// 드래그 중이 아니라면 현재 마우스 위치의 뷰포트를 사용 (호버 처리용)
	else
	{
		const FVector& MousePos = UInputManager::GetInstance().GetMousePosition();
		auto& Viewports = ViewportClient->GetViewports();

		// 마우스 위치에 있는 뷰포트 찾기
		for (auto& Viewport : Viewports)
		{
			if (!Viewport.bIsVisible) continue;

			const D3D11_VIEWPORT& ViewportInfo = Viewport.GetViewportInfo();
			if (MousePos.X >= ViewportInfo.TopLeftX && MousePos.X <= (ViewportInfo.TopLeftX + ViewportInfo.Width) &&
				MousePos.Y >= ViewportInfo.TopLeftY && MousePos.Y <= (ViewportInfo.TopLeftY + ViewportInfo.Height))
			{
				CurrentViewport = &Viewport;
				break;
			}
		}
	}

	// 처리할 영역이 존재하지 않으면 진행을 중단합니다.
	if (CurrentViewport == nullptr) { return; }

	// PIE 모드 뷰포트에서는 에디터 입력 처리를 하지 않음
	bool bIsPIEViewport = CurrentViewport->RenderTargetWorld && CurrentViewport->RenderTargetWorld->IsPIEWorld();
	if (bIsPIEViewport) { return; }

	CurrentCamera = &CurrentViewport->Camera;

	TObjectPtr<AActor> ActorPicked = InLevel->GetSelectedActor();

	if (ActorPicked)
	{
		// 피킹 전 현재 카메라에 맞는 기즈모 스케일 업데이트
		Gizmo.UpdateScale(CurrentCamera);
	}

	const UInputManager& InputManager = UInputManager::GetInstance();
	const FVector& MousePos = InputManager.GetMousePosition();
	const D3D11_VIEWPORT& ViewportInfo = CurrentViewport->GetViewportInfo();

	// 다중 뷰포트에서의 정확한 NDC 변환
	float RelativeX = MousePos.X - ViewportInfo.TopLeftX;
	float RelativeY = MousePos.Y - ViewportInfo.TopLeftY;
	
	// 뷰포트 경계 체크 및 클램핑
	RelativeX = max(0.0f, min(RelativeX, ViewportInfo.Width));
	RelativeY = max(0.0f, min(RelativeY, ViewportInfo.Height));
	
	const float NdcX = (RelativeX / ViewportInfo.Width) * 2.0f - 1.0f;
	const float NdcY = -((RelativeY / ViewportInfo.Height) * 2.0f - 1.0f);

	FRay WorldRay = CurrentCamera->ConvertToWorldRay(NdcX, NdcY);

	static EGizmoDirection PreviousGizmoDirection = EGizmoDirection::None;
	FVector CollisionPoint;
	float ActorDistance = -1;
	// gizmo 클릭으로 피킹을 시작한 경우 중복 시작을 막기 위한 플래그
	bool bGizmoDragStarted = false;

	if (InputManager.IsKeyPressed(EKeyInput::Tab))
	{
		Gizmo.IsWorldMode() ? Gizmo.SetLocal() : Gizmo.SetWorld();
	}
	if (InputManager.IsKeyPressed(EKeyInput::Space))
	{
		Gizmo.ChangeGizmoMode();
	}
	if (InputManager.IsKeyReleased(EKeyInput::MouseLeft))
	{
		if (Gizmo.IsDragging() && Gizmo.GetSelectedActor())
		{
			// 드래그 끝나면 선택 액터의 프리미티브를 더티 큐에 등록
			MarkActorPrimitivesDirty(Gizmo.GetSelectedActor());

			// Update transform of the dragged component and its children
			if (USceneComponent* DraggedComponent = Gizmo.GetTargetComponent())
			{
				DraggedComponent->UpdateWorldTransform();
			}
			// Also update root component in case it wasn't the dragged component
			else if (Gizmo.GetSelectedActor()->GetRootComponent())
			{
				Gizmo.GetSelectedActor()->GetRootComponent()->UpdateWorldTransform();
			}

			// Octree 동적 목록으로 이동
			for (const auto& Comp : Gizmo.GetSelectedActor()->GetOwnedComponents())
			{
				if (auto* Prim = Cast<UPrimitiveComponent>(Comp.Get()))
				{
					if (Prim->GetPrimitiveType() != EPrimitiveType::BillBoard)
					{
						ULevelManager::GetInstance().GetCurrentLevel()->MoveToDynamic(Prim);
					}
				}
			}
		}

		Gizmo.EndDrag();
		// 드래그가 끝나면 선택된 뷰포트를 비활성화 합니다.
		InteractionViewport = nullptr;
	}

	if (Gizmo.IsDragging() && Gizmo.GetSelectedActor())
	{
		switch (Gizmo.GetGizmoMode())
		{
		case EGizmoMode::Translate:
		{
			FVector GizmoDragLocation = GetGizmoDragLocation(CurrentCamera, WorldRay);
			Gizmo.SetLocation(GizmoDragLocation);
			break;
		}
		case EGizmoMode::Rotate:
		{
			FVector GizmoDragRotation = GetGizmoDragRotation(CurrentCamera, WorldRay);
			Gizmo.SetActorRotation(GizmoDragRotation);
			break;
		}
		case EGizmoMode::Scale:
		{
			FVector GizmoDragScale = GetGizmoDragScale(CurrentCamera, WorldRay);
			Gizmo.SetActorScale(GizmoDragScale);
		}
		}
	}
	else
	{
		if (InLevel->GetSelectedActor() && Gizmo.HasActor())
		{
			// 매 프레임 마우스 포인터에 의한 기즈모 호버/충돌 계산
			ObjectPicker.PickGizmo(CurrentCamera, WorldRay, Gizmo, CollisionPoint);
		}
		else
		{
			Gizmo.SetGizmoDirection(EGizmoDirection::None);
		}

		// ImGui 호버 체크: 마우스가 ImGui 창 위에 있으면 picking 하지 않음
		bool bMouseOverImGui = ImGui::GetIO().WantCaptureMouse || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);

		if (!bMouseOverImGui && InputManager.IsKeyPressed(EKeyInput::MouseLeft))
		{
			// 퍼포먼스 측정용 카운터 시작 (BVH 탐색을 수행하는 경우에만 측정)
			FScopeCycleCounter PickCounter(GetPickingStatId());

			// 만약 기즈모가 활성화되어 있고(선택된 액터가 있고), 현재 마우스가 기즈모 위에 있다면
			if (Gizmo.HasActor() && Gizmo.GetGizmoDirection() != EGizmoDirection::None)
			{
				Gizmo.OnMouseDragStart(CollisionPoint);
				InteractionViewport = CurrentViewport;
				ActorPicked = Gizmo.GetSelectedActor();
				bGizmoDragStarted = true;
			}
			else
			{
				if (ULevelManager::GetInstance().GetCurrentLevel()->GetShowFlags() & EEngineShowFlags::SF_Primitives)
				{
					// BVH 사용 피킹
					UPrimitiveComponent* HitPrim = PickPrimitiveUsingBVH(CurrentCamera, WorldRay, InLevel, &ActorDistance);
					ActorPicked = HitPrim ? HitPrim->GetOwner() : nullptr;
				}
			}

			// 퍼포먼스 측정 종료 및 시간, 카운트 누적 -> RAII 객체 소멸 시점에 자동으로 처리
		}

		if (Gizmo.GetGizmoDirection() == EGizmoDirection::None)
		{
			InLevel->SetSelectedActor(ActorPicked);
			if (PreviousGizmoDirection != EGizmoDirection::None)
			{
				Gizmo.OnMouseRelease(PreviousGizmoDirection);
			}
		}
		else
		{
			PreviousGizmoDirection = Gizmo.GetGizmoDirection();
			bool bMouseOverImGui = ImGui::GetIO().WantCaptureMouse || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
			if (!bMouseOverImGui && InputManager.IsKeyPressed(EKeyInput::MouseLeft))
			{
				// 이미 위에서 드래그를 시작했다면 다시 호출하지 않음
				if (!bGizmoDragStarted)
				{
					Gizmo.OnMouseDragStart(CollisionPoint);
					InteractionViewport = CurrentViewport;
				}
				// else: 이미 시작한 상태이므로 중복 호출 생략
			}
			else
			{
				Gizmo.OnMouseHovering();
			}
		}
	}
}

TArray<UPrimitiveComponent*> UEditor::FindCandidatePrimitives(ULevel* InLevel)
{
	TArray<UPrimitiveComponent*> Candidate;
	for (AActor* Actor : InLevel->GetActors())
	{
		for (auto& ActorComponent : Actor->GetOwnedComponents())
		{
			if (TObjectPtr<UPrimitiveComponent> Primitive = Cast<UPrimitiveComponent>(ActorComponent))
			{
				Candidate.push_back(Primitive);
			}
		}
	}
	return Candidate;
}

void UEditor::EnsureBVHUpToDate(ULevel* InLevel)
{
	if (!InLevel)
	{
		return;
	}

	const TArray<TObjectPtr<UPrimitiveComponent>>& LevelPrims = InLevel->GetLevelPrimitiveComponents();

	// 1) 프리미티브 개수가 변했으면 즉시 재빌드
	if (SceneBVHLastCount != LevelPrims.size())
	{
		TArray<UPrimitiveComponent*> RawPrims;
		RawPrims.reserve(LevelPrims.size());
		for (auto& Prim : LevelPrims)
		{
			RawPrims.push_back(Prim);
		}

		SceneBVH.Build(RawPrims);
		SceneBVHLastCount = LevelPrims.size();

		PendingDirtyPrims.clear();
		SceneBVHRefitTick = 0;
		bPickingWarmed = true;

		return;
	}

	// 2) 더티가 있으면 부분 리핏
	if (!PendingDirtyPrims.empty())
	{
		SceneBVH.RefitDirtyByPrims(PendingDirtyPrims);
		PendingDirtyPrims.clear();
		SceneBVHRefitTick = 0;
		return;
	}

	// 3) 주기적 전체 리핏(품질 유지용)
	/*if ((++SceneBVHRefitTick % 60) == 0)
	{
		SceneBVH.Refit();
	}*/

	// 4) 빌드 직후 프레임에서 카메라 생겼으면 피킹 워밍업
	if (!bPickingWarmed)
	{
		if (FViewport* ViewportClient = URenderer::GetInstance().GetViewportClient())
		{
			if (UCamera* ActiveCamera = ViewportClient->GetActiveCamera())
			{
				PrewarmPicking(InLevel, ActiveCamera);
			}
		}
	}
}

void UEditor::PrewarmPicking(ULevel* InLevel, UCamera* InCamera)
{
	if (!InLevel || !InCamera)
	{
		return;
	}

	// 화면 중앙 레이(또는 카메라 전방)로 워밍
	FRay Ray = InCamera->ConvertToWorldRay(0.0f, 0.0f);

	float BestDist = std::numeric_limits<float>::infinity();
	UPrimitiveComponent* BestPrim = nullptr;

	// Precompute ray origin/direction used by PreciseTestForPick
	const FVector RayO{ Ray.Origin.X, Ray.Origin.Y, Ray.Origin.Z };
	FVector RayD{ Ray.Direction.X, Ray.Direction.Y, Ray.Direction.Z };
	const float DirLen = RayD.Length();
	if (DirLen > 1e-8f) RayD *= (1.0f / DirLen);

	// Narrow-phase warm: use new visitor signature (Prim, PrimIdx, PTMin, InOutDist)
	auto PreciseTest = [&](UPrimitiveComponent* Prim, int64 PrimIdx, float PrimPTMin, float& InOutHitDist) -> bool
		{
			return PreciseTestForPick(InCamera, Ray, Prim, PrimIdx, PrimPTMin, InOutHitDist, RayO, RayD);
		};

	SceneBVH.TraverseFrontToBackFirstHit(Ray, BestDist, BestPrim, PreciseTest);

	bPickingWarmed = true;
}

UPrimitiveComponent* UEditor::PickPrimitiveUsingBVH(UCamera* InCamera, const FRay& WorldRay, ULevel* InLevel, float* OutDistance)
{
	// front-to-back traversal with early precise tests
	float BestDist = std::numeric_limits<float>::infinity();
	UPrimitiveComponent* BestPrim = nullptr;

	// Precompute reusable values to reduce per-primitive overhead
	const FVector RayO{ WorldRay.Origin.X, WorldRay.Origin.Y, WorldRay.Origin.Z };
	FVector RayD{ WorldRay.Direction.X, WorldRay.Direction.Y, WorldRay.Direction.Z };
	const float DirLen = RayD.Length();
	if (DirLen > 1e-8f) RayD *= (1.0f / DirLen); // normalize once

	// Thin wrapper lambda matching new visitor signature (Prim, PrimIdx, PTMin, InOutHitDist)
	auto PreciseTest = [&](UPrimitiveComponent* Prim, int64 PrimIdx, float PrimPTMin, float& InOutHitDist) -> bool
		{
			return PreciseTestForPick(InCamera, WorldRay, Prim, PrimIdx, PrimPTMin, InOutHitDist, RayO, RayD);
		};

	SceneBVH.TraverseFrontToBackFirstHit(WorldRay, BestDist, BestPrim, PreciseTest);

	if (OutDistance)
	{
		*OutDistance = (BestPrim ? BestDist : -1.0f);
	}

	return BestPrim;
}

bool UEditor::PreciseTestForPick(UCamera* InCamera, const FRay& WorldRay, UPrimitiveComponent* Prim, int64 PrimIndex, float PrimPTMin,
	float& InOutHitDist, const FVector& RayO, const FVector& RayD)
{
#ifdef ENABLE_BVH_STATS
	FScopeCycleCounter Counter(GetPickPrimitiveStatId());
#endif

	// 1) BVH-provided conservative TMin early-out (already present)
	if (PrimPTMin >= InOutHitDist)
	{
		return false;
	}

	// 2) Extremely cheap rejects to avoid heavy work:
	if (!Prim) return false;
	if (!Prim->IsVisible()) return false;
	if (Prim->ShouldCullForOcclusion()) return false;

	// 3) Very cheap sphere reject using BVH SoA (no map lookup)
	FVector PrimCenter;
	float PrimRadiusSq = 0.0f;
	if (SceneBVH.GetPrimSphereByIndex(PrimIndex, PrimCenter, PrimRadiusSq))
	{
		// Ray-sphere test in world space using precomputed radius^2 (avoid sqrt)
		const FVector L = RayO - PrimCenter;
		const float b = RayD.Dot(L);
		const float c = L.Dot(L) - PrimRadiusSq;
		const float disc = b * b - c;
		if (disc < 0.0f)
		{
			return false; // sphere miss -> reject
		}

		const float sqrtD = sqrtf(disc);
		const float t0 = -b - sqrtD;
		const float t1 = -b + sqrtD;
		float t = (t0 >= 0.0f) ? t0 : ((t1 >= 0.0f) ? t1 : -1.0f);
		if (t < 0.0f) return false;

		// If conservative t is already >= current best, skip heavy test.
		if (t >= InOutHitDist) return false;
	}
	// else: no sphere data available -> fall through to heavy test

	// 4) Heavy precise test
	float Dist = InOutHitDist;
	if (ObjectPicker.PickPrimitive(InCamera, WorldRay, Prim, &Dist) && Dist < InOutHitDist)
	{
		InOutHitDist = Dist;
		return true;
	}

	return false;
}

FVector UEditor::GetGizmoDragLocation(UCamera* InActiveCamera, FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	if (!Gizmo.IsWorldMode())
	{
		FVector4 GizmoAxis4{ GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f };
		FVector RadRotation = FVector::GetDegreeToRadian(Gizmo.GetActorRotation());
		GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
	}

	// 다중 뷰포트에서 안정적인 평면 법선 버그 계산
	FVector PlaneNormal = InActiveCamera->CalculatePlaneNormal(GizmoAxis).Cross(GizmoAxis);
	
	// 평면 법선이 너무 작을 때 대체 법선 사용
	if (PlaneNormal.LengthSquared() < 1e-6f)
	{
		// 카메라 Forward와 기즈모 축이 평행할 때 대체 법선 사용
		FVector CameraRight = InActiveCamera->GetRight();
		FVector CameraUp = InActiveCamera->GetUp();
		
		// 기쥸모 축과 수직인 방향 중 더 안정적인 것 선택
		float RightDot = abs(CameraRight.Dot(GizmoAxis));
		float UpDot = abs(CameraUp.Dot(GizmoAxis));
		
		if (RightDot < UpDot)
		{
			PlaneNormal = CameraRight.Cross(GizmoAxis);
		}
		else
		{
			PlaneNormal = CameraUp.Cross(GizmoAxis);
		}
		
		PlaneNormal.Normalize();
	}
	
	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
	{
		FVector MouseDistance = MouseWorld - Gizmo.GetDragStartMouseLocation();
		return Gizmo.GetDragStartActorLocation() + GizmoAxis * MouseDistance.Dot(GizmoAxis);
	}
	return Gizmo.GetGizmoLocation();
}

FVector UEditor::GetGizmoDragRotation(UCamera* InActiveCamera, FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin{ Gizmo.GetGizmoLocation() };
	FVector GizmoAxis = Gizmo.GetGizmoAxis();

	if (!Gizmo.IsWorldMode())
	{
		FVector4 GizmoAxis4{ GizmoAxis.X, GizmoAxis.Y, GizmoAxis.Z, 0.0f };
		FVector RadRotation = FVector::GetDegreeToRadian(Gizmo.GetActorRotation());
		GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);
	}

	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, GizmoAxis, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		PlaneOriginToMouse.Normalize();
		PlaneOriginToMouseStart.Normalize();
		float DotResult = (PlaneOriginToMouseStart).Dot(PlaneOriginToMouse);
		float Angle = acosf(std::max(-1.0f, std::min(1.0f, DotResult)));
		if ((PlaneOriginToMouse.Cross(PlaneOriginToMouseStart)).Dot(GizmoAxis) < 0)
		{
			Angle = -Angle;
		}

		// Use stored Quaternion to avoid Euler wrapping issues
		FQuaternion StartRotQuat = Gizmo.GetDragStartActorRotationQuat();
		FQuaternion DeltaRotQuat = FQuaternion::FromAxisAngle(Gizmo.GetGizmoAxis(), Angle);
		FQuaternion ResultQuat;

		if (Gizmo.IsWorldMode())
		{
			ResultQuat = DeltaRotQuat * StartRotQuat;
		}
		else
		{
			ResultQuat = StartRotQuat * DeltaRotQuat;
		}

		// Convert to Euler only once at the end
		return ResultQuat.ToEuler();
	}
	return Gizmo.GetActorRotation();
}

FVector UEditor::GetGizmoDragScale(UCamera* InActiveCamera, FRay& WorldRay)
{
	FVector MouseWorld;
	FVector PlaneOrigin = Gizmo.GetGizmoLocation();
	FVector CardinalAxis = Gizmo.GetGizmoAxis();

	FVector4 GizmoAxis4{ CardinalAxis.X, CardinalAxis.Y, CardinalAxis.Z, 0.0f };
	FVector RadRotation = FVector::GetDegreeToRadian(Gizmo.GetActorRotation());
	FVector GizmoAxis = Gizmo.GetGizmoAxis();
	GizmoAxis = GizmoAxis4 * FMatrix::RotationMatrix(RadRotation);


	FVector PlaneNormal = InActiveCamera->CalculatePlaneNormal(GizmoAxis).Cross(GizmoAxis);
	if (ObjectPicker.IsRayCollideWithPlane(WorldRay, PlaneOrigin, PlaneNormal, MouseWorld))
	{
		FVector PlaneOriginToMouse = MouseWorld - PlaneOrigin;
		FVector PlaneOriginToMouseStart = Gizmo.GetDragStartMouseLocation() - PlaneOrigin;
		float DragStartAxisDistance = PlaneOriginToMouseStart.Dot(GizmoAxis);
		float DragAxisDistance = PlaneOriginToMouse.Dot(GizmoAxis);
		float ScaleFactor = 1.0f;
		if (abs(DragStartAxisDistance) > 0.1f)
		{
			ScaleFactor = DragAxisDistance / DragStartAxisDistance;
		}

		FVector DragStartScale = Gizmo.GetDragStartActorScale();
		if (ScaleFactor > MinScale)
		{
			if (Gizmo.GetSelectedActor()->IsUniformScale())
			{
				float UniformValue = DragStartScale.Dot(CardinalAxis);
				return FVector(1.0f, 1.0f, 1.0f) * UniformValue * ScaleFactor;
			}
			else
			{
				return DragStartScale + CardinalAxis * (ScaleFactor - 1.0f) * DragStartScale.Dot(CardinalAxis);
			}
		}
		return Gizmo.GetActorScale();
	}
	return Gizmo.GetActorScale();
}

void UEditor::ResetBVH()
{
	SceneBVH.Clear();
	SceneBVHLastCount = 0;
	SceneBVHRefitTick = 0;
	bPickingWarmed = false;
	PendingDirtyPrims.clear();
}
