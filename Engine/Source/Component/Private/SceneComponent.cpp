#include "pch.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"

#include <json.hpp>

IMPLEMENT_CLASS(USceneComponent, UActorComponent)

USceneComponent::USceneComponent()
{
	ComponentType = EComponentType::Scene;
}

void USceneComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		FJsonSerializer::ReadVector(InOutHandle, "Location", RelativeLocation, FVector::ZeroVector());
		FJsonSerializer::ReadVector(InOutHandle, "Rotation", RelativeRotation, FVector::ZeroVector());
		FJsonSerializer::ReadVector(InOutHandle, "Scale", RelativeScale3D, FVector::OneVector());
	}
	// 저장
	else
	{
		InOutHandle["Location"] = FJsonSerializer::VectorToJson(RelativeLocation);
		InOutHandle["Rotation"] = FJsonSerializer::VectorToJson(RelativeRotation);
		InOutHandle["Scale"] = FJsonSerializer::VectorToJson(RelativeScale3D);
	}
}

void USceneComponent::SetParentAttachment(USceneComponent* NewParent)
{
	if (NewParent == this)
	{
		return;
	}

	if (NewParent == ParentAttachment)
	{
		return;
	}

	// [수정] 무한 루프를 유발하던 순환 참조 체크 로직 수정
	for (USceneComponent* Ancester = NewParent; Ancester; Ancester = Ancester->ParentAttachment)
	{
		if (Ancester == this)
		{
			// 새로운 부모의 조상 중에 자기 자신이 있다면 순환 관계가 되므로, 작업을 중단합니다.
			return;
		}
	}

	// 기존 부모가 있었다면, 그 부모의 자식 목록에서 나를 제거합니다.
	if (ParentAttachment)
	{
		ParentAttachment->RemoveChild(this);
	}

	// 새로운 부모를 설정합니다.
	ParentAttachment = NewParent;

	// [추가] 새로운 부모가 있다면, 그 부모의 자식 목록에 나를 추가합니다.
	if (ParentAttachment)
	{
		ParentAttachment->Children.push_back(this);
	}

	MarkAsDirty();
}

void USceneComponent::RemoveChild(USceneComponent* ChildDeleted)
{
	Children.erase(std::remove(Children.begin(), Children.end(), ChildDeleted), Children.end());
}

void USceneComponent::MarkAsDirty()
{
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;

	if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(this))
	{
		PrimitiveComp->MarkWorldAABBDirty();
	}

	for (USceneComponent* Child : Children)
	{
		Child->MarkAsDirty();
	}
}
