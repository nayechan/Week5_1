#include "pch.h"
#include "Render/UI/Widget/Public/ActorDetailWidget.h"

#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/SceneComponent.h"

TArray<TObjectPtr<UClass>> UActorDetailWidget::CreatableComponentClasses;

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
void UActorDetailWidget::RenderComponentTree(AActor* InSelectedActor)
{
	if (!InSelectedActor) return;

	ImGui::Text("Components");
	ImGui::Separator();

	const TArray<TObjectPtr<UActorComponent>>& AllComponents = InSelectedActor->GetOwnedComponents();

	if (AllComponents.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No components");
		return;
	}

	if (USceneComponent* RootComponent = InSelectedActor->GetRootComponent())
	{
		RenderComponentNode(RootComponent);
	}
	for (const auto& Component : AllComponents)
	{
		if (Component && !Cast<USceneComponent>(Component))
		{
			RenderNonSceneComponentNode(Component);
		}
	}

	ImGui::Separator();
	if (ImGui::Button("[+] Add Component", ImVec2(-1, 0)))
	{
		ImGui::OpenPopup("AddComponentPopup");
	}
	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		// --- 팝업창 내부 UI ---
		ImGui::Text("Available Components");
		ImGui::Separator();

		if (CreatableComponentClasses.empty())
		{
			TArray<TObjectPtr<UClass>> AllSubclasses = UClass::GetSubclassesOf(UActorComponent::StaticClass());
			for (const auto& Subclass : AllSubclasses)
			{
				// 생성할 수 없는 클래스(추상 클래스 등)는 제외합니다.
				if (Subclass->CreateDefaultObject() != nullptr)
				{
					CreatableComponentClasses.push_back(Subclass);
				}
			}

			// 이름순으로 정렬하여 일관된 순서를 보장합니다.
			std::sort(CreatableComponentClasses.begin(), CreatableComponentClasses.end(),
			    [](const TObjectPtr<UClass>& A, const TObjectPtr<UClass>& B)
			    {
				    return A->GetClassTypeName().ToString() < B->GetClassTypeName().ToString();
			    });

			// 정렬된 목록에서 중복된 클래스를 제거합니다.
			CreatableComponentClasses.erase(std::unique(CreatableComponentClasses.begin(), CreatableComponentClasses.end(),
				[](const TObjectPtr<UClass>& A, const TObjectPtr<UClass>& B)
				{
					// 클래스 이름이 같으면 중복으로 간주합니다.
					return A->GetClassTypeName() == B->GetClassTypeName();
				}),
				CreatableComponentClasses.end());
		}

		for (const auto& Class : CreatableComponentClasses)
		{
			if (ImGui::Selectable(Class->GetClassTypeName().ToString().data()))
			{
				FString NewComponentName = "New" + Class->GetClassTypeName().ToString();
				UActorComponent* NewComponent = InSelectedActor->AddComponentByClass(Class, FName(NewComponentName));

				if (NewComponent)
				{
					if (USceneComponent* NewSceneComponent = Cast<USceneComponent>(NewComponent))
					{
						USceneComponent* ParentToAttach = nullptr;

						// 1. 현재 선택된 컴포넌트가 있는지, 그리고 SceneComponent인지 확인
						TObjectPtr<UObject> SelectedComponent = UUIManager::GetInstance().GetSelectedObject();
						if (USceneComponent* SelectedSceneComp = Cast<USceneComponent>(SelectedComponent.Get()))
						{
							ParentToAttach = SelectedSceneComp; // 있다면 선택된 컴포넌트에 붙임
						}
						else
						{
							ParentToAttach = InSelectedActor->GetRootComponent(); // 없다면 루트에 붙임
						}

						if (ParentToAttach)
						{
							NewSceneComponent->SetParentAttachment(ParentToAttach);
							this->NodeToOpenNextFrame = ParentToAttach;
						}
						else
						{
							// 루트도 없는 경우, 이 컴포넌트를 새로운 루트로 설정
							InSelectedActor->SetRootComponent(NewSceneComponent);
						}

						ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
						if (CurrentLevel)
						{
							CurrentLevel->AddLevelPrimitiveComponent(InSelectedActor);
							CurrentLevel->AddActorToDynamic(InSelectedActor);
						}
					}
				}
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndPopup();
	}
}

/**
 * @brief 컴포넌트에 대한 정보를 표시하는 함수
 * 내부적으로 RTTI를 활용한 GetName 처리가 되어 있음
 * @param InComponent
 */
void UActorDetailWidget::RenderComponentNode(USceneComponent* InComponent)
{
	ImGui::PushID(InComponent);
	if (!InComponent) return;

	if (this->NodeToOpenNextFrame.Get() == InComponent)
	{
		ImGui::SetNextItemOpen(true);
		this->NodeToOpenNextFrame = nullptr;
	}
	const auto& Children = InComponent->GetAttachChildren();

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
	if (Children.empty())
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf;
	}
	const bool bIsSelected = (UUIManager::GetInstance().GetSelectedObject() == InComponent);
	if (bIsSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
	}

	FString NodeLabel = "[S] " + InComponent->GetName().ToString();
	bool bNodeOpen = ImGui::TreeNodeEx(NodeLabel.data(), NodeFlags);

	// [수정] PushStyleColor를 호출했다면, 반드시 PopStyleColor를 호출해서 스택을 정리해야 합니다.
	if (bIsSelected)
	{
		ImGui::PopStyleColor();
	}

	if (ImGui::IsItemClicked())
	{
		UUIManager::GetInstance().SetSelectedObject(TObjectPtr<UObject>(InComponent));
	}

	if (bNodeOpen)
	{
		if (!Children.empty())
		{
			for (const auto& ChildComponent : Children)
			{
				RenderComponentNode(ChildComponent);
			}
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

/**
 * @brief SceneComponent가 아닌 컴포넌트를 그리는 함수
 * @param InComponent
 */
void UActorDetailWidget::RenderNonSceneComponentNode(UActorComponent* InComponent)
{
	ImGui::PushID(InComponent);
	if (!InComponent) return;

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	const bool bIsSelected = (UUIManager::GetInstance().GetSelectedObject() == InComponent);
	if (bIsSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
	}

	FString NodeLabel = "[C] " + InComponent->GetName().ToString();
	ImGui::TreeNodeEx(NodeLabel.data(), NodeFlags);

	if (bIsSelected)
	{
		ImGui::PopStyleColor();
	}

	if (ImGui::IsItemClicked())
	{
		UUIManager::GetInstance().SetSelectedObject(TObjectPtr<UObject>(InComponent));
	}
	ImGui::PopID();
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
