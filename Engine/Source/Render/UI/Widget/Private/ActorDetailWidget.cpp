#include "pch.h"
#include "Render/UI/Widget/Public/ActorDetailWidget.h"

#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/SceneComponent.h"

UActorDetailWidget::UActorDetailWidget()
	: UWidget("Actor Detail Widget")
{
}

UActorDetailWidget::~UActorDetailWidget() = default;

void UActorDetailWidget::Initialize()
{
	UE_LOG("ActorDetailWidget: Initialized");
}

void UActorDetailWidget::Update()
{
	// 특별한 업데이트 로직이 필요하면 여기에 추가
}

void UActorDetailWidget::RenderWidget()
{
	TObjectPtr<ULevel> CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	TObjectPtr<AActor> SelectedActor = CurrentLevel->GetSelectedActor();
	if (!SelectedActor)
	{
		ImGui::TextUnformatted("No Object Selected");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Detail 확인을 위해 Object를 선택해주세요");
		return;
	}

	// Actor 헤더 렌더링 (이름 + rename 기능)
	RenderActorHeader(SelectedActor);

	ImGui::Separator();

	// 컴포넌트 트리 렌더링
	RenderComponentTree(SelectedActor);
}

void UActorDetailWidget::RenderActorHeader(TObjectPtr<AActor> InSelectedActor)
{
	if (!InSelectedActor)
	{
		return;
	}

	FName ActorName = InSelectedActor->GetName();
	FString ActorDisplayName = ActorName.ToString();

	ImGui::Text("[A]");
	ImGui::SameLine();

	if (bIsRenamingActor)
	{
		// Rename 모드
		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("##ActorRename", ActorNameBuffer, sizeof(ActorNameBuffer),
		                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
		{
			FinishRenamingActor(InSelectedActor);
		}

		// ESC로 취소, InputManager보다 일단 내부 API로 입력 받는 것으로 처리
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			CancelRenamingActor();
		}
	}
	else
	{
		// 더블클릭으로 rename 시작
		ImGui::Text("%s", ActorDisplayName.data());

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			StartRenamingActor(InSelectedActor);
		}

		// 툴팁 UI
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Double-Click to Rename");
		}
	}
}

/**
 * @brief 컴포넌트들을 트리 형태로 표시하는 함수
 * @param InSelectedActor 선택된 Actor
 */
void UActorDetailWidget::RenderComponentTree(TObjectPtr<AActor> InSelectedActor)
{
	if (!InSelectedActor) return;

	const TArray<TObjectPtr<UActorComponent>>& Components = InSelectedActor->GetOwnedComponents();

	ImGui::Text("Components (%d)", static_cast<int>(Components.size()));
	ImGui::Separator();

	if (Components.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No components");
		return;
	}

	for (int32 i = 0; i < static_cast<int32>(Components.size()); ++i)
	{
		if (Components[i])
		{
			RenderComponentNode(Components[i]);
		}
	}
}

/**
 * @brief 컴포넌트에 대한 정보를 표시하는 함수
 * 내부적으로 RTTI를 활용한 GetName 처리가 되어 있음
 * @param InComponent
 */
void UActorDetailWidget::RenderComponentNode(TObjectPtr<UActorComponent> InComponent)
{
	if (!InComponent)
	{
		return;
	}

	// 컴포넌트 타입에 따른 아이콘
	FName ComponentTypeName = InComponent.Get()->GetClass()->GetClassTypeName();
	FString ComponentIcon = "[C]"; // 기본 컴포넌트 아이콘

	if (Cast<UPrimitiveComponent>(InComponent))
	{
		ComponentIcon = "[P]"; // PrimitiveComponent 아이콘
	}
	else if (Cast<USceneComponent>(InComponent))
	{
		ComponentIcon = "[S]"; // SceneComponent 아이콘
	}

	// 트리 노드 생성
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	FString NodeLabel = ComponentIcon + " " + InComponent->GetName().ToString();
	ImGui::TreeNodeEx(NodeLabel.data(), NodeFlags);

	// 컴포넌트 세부 정보를 추가로 표시할 수 있음
	if (ImGui::IsItemHovered())
	{
		// 컴포넌트 타입 정보를 툴팁으로 표시
		ImGui::SetTooltip("Component Type: %s", ComponentTypeName.ToString().data());
	}

	// PrimitiveComponent인 경우 추가 정보
	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(InComponent))
	{
		ImGui::SameLine();
		ImGui::TextColored(
			PrimitiveComponent->IsVisible() ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
			PrimitiveComponent->IsVisible() ? "[Visible]" : "[Hidden]"
		);
	}
}

void UActorDetailWidget::StartRenamingActor(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return;
	}

	bIsRenamingActor = true;
	FString CurrentName = InActor->GetName().ToString();
	strncpy_s(ActorNameBuffer, CurrentName.data(), sizeof(ActorNameBuffer) - 1);
	ActorNameBuffer[sizeof(ActorNameBuffer) - 1] = '\0';

	UE_LOG("ActorDetailWidget: '%s' 에 대한 이름 변경 시작", CurrentName.data());
}

void UActorDetailWidget::FinishRenamingActor(TObjectPtr<AActor> InActor)
{
	if (!InActor || !bIsRenamingActor)
	{
		return;
	}

	FString NewName = ActorNameBuffer;
	if (!NewName.empty() && NewName != InActor->GetName().ToString())
	{
		// Actor 이름 변경
		InActor->SetDisplayName(NewName);
		UE_LOG_SUCCESS("ActorDetailWidget: Actor의 이름을 '%s' (으)로 변경하였습니다", NewName.data());
	}

	bIsRenamingActor = false;
	ActorNameBuffer[0] = '\0';
}

void UActorDetailWidget::CancelRenamingActor()
{
	bIsRenamingActor = false;
	ActorNameBuffer[0] = '\0';
	UE_LOG_WARNING("ActorDetailWidget: 이름 변경 취소");
}
