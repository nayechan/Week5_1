#include "pch.h"
#include "Render/UI/Widget/Public/ActorTerminationWidget.h"

#include "Level/Public/Level.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Level/Public/LevelManager.h"


UActorTerminationWidget::UActorTerminationWidget()
	: UWidget("Actor Termination Widget")
	  , SelectedActor(nullptr)
{
}

UActorTerminationWidget::~UActorTerminationWidget() = default;

void UActorTerminationWidget::Initialize()
{
	// Do Nothing Here
}

void UActorTerminationWidget::Update()
{
	// 매 프레임 Level의 선택된 Actor를 확인해서 정보 반영
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	TObjectPtr<ULevel> CurrentLevel = LevelManager.GetCurrentLevel();

	if (CurrentLevel)
	{
		AActor* CurrentSelectedActor = CurrentLevel->GetSelectedActor();

		// Update Current Selected Actor
		if (SelectedActor != CurrentSelectedActor)
		{
			SelectedActor = CurrentSelectedActor;
		}

		// null이어도 갱신 필요
		if (!CurrentSelectedActor)
		{
			SelectedActor = nullptr;
		}
	}
}

void UActorTerminationWidget::RenderWidget()
{
	auto& InputManager = UInputManager::GetInstance();

	// Always get the current selected actor directly from the level to avoid sync issues
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	TObjectPtr<ULevel> CurrentLevel = LevelManager.GetCurrentLevel();
	AActor* CurrentSelectedActor = CurrentLevel ? CurrentLevel->GetSelectedActor() : nullptr;

	if (CurrentSelectedActor)
	{
		// ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Selected: %s (%p)",
		//                    CurrentSelectedActor->GetName().c_str(), CurrentSelectedActor);

		// Use IsKeyPressed instead of IsKeyDown to only trigger once per key press
		// Also check if ImGui is not capturing keyboard input
		bool bImGuiWantsKeyboard = ImGui::GetIO().WantCaptureKeyboard;
		if (!bImGuiWantsKeyboard && InputManager.IsKeyPressed(EKeyInput::Delete))
		{
			UE_LOG("ActorTerminationWidget::RenderWidget: Delete 키 감지됨");
			// Update SelectedActor before deletion
			SelectedActor = CurrentSelectedActor;
			DeleteSelectedActor();
		}
	}
	else
	{
		// ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Actor Selected For Deletion");
	}
}

/**
 * @brief Selected Actor 삭제 함수
 */
void UActorTerminationWidget::DeleteSelectedActor()
{
	UE_LOG("===============================================");
	UE_LOG("ActorTerminationWidget: 삭제를 위한 Actor Marking 시작");
	if (!SelectedActor)
	{
		UE_LOG_ERROR("ActorTerminationWidget: 삭제를 위한 Actor가 선택되지 않았습니다");
		return;
	}

	ULevelManager& LevelManager = ULevelManager::GetInstance();
	TObjectPtr<ULevel> CurrentLevel = LevelManager.GetCurrentLevel();

	if (!CurrentLevel)
	{
		UE_LOG_ERROR("ActorTerminationWidget: No Current Level To Delete Actor From");
		return;
	}

	UE_LOG_INFO("ActorTerminationWidget: 선택된 Actor를 삭제를 위해 마킹 처리: %s (ptr: %p)",
	       SelectedActor->GetName() == FName::GetNone() ? "UnNamed" : SelectedActor->GetName().ToString().data(),
	       SelectedActor.Get());

	// 지연 삭제를 사용하여 안전하게 다음 틱에서 삭제
	CurrentLevel->MarkActorForDeletion(SelectedActor);

	UE_LOG("ActorTerminationWidget: MarkActorForDeletion 호출 완료");
	UE_LOG("===============================================");

	// MarkActorForDeletion에서 선택 해제도 처리하므로 여기에서는 단순히 nullptr로 설정
	SelectedActor = nullptr;
}
