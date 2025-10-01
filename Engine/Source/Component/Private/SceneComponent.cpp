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

	// 순환 참조 방지: NewParent의 조상들 중에 this가 있는지 확인
	for (USceneComponent* Ancestor = NewParent; Ancestor; Ancestor = Ancestor->ParentAttachment)
	{
		if (Ancestor == this)
		{
			return;
		}
	}

	//부모가 될 자격이 있음, 이제 부모를 바꿈.

	if (ParentAttachment) //부모 있었으면 이제 그 부모의 자식이 아님
	{
		ParentAttachment->RemoveChild(this);
	}

	// 새로운 부모와 연결
	ParentAttachment = NewParent;
	if (ParentAttachment)
	{
		ParentAttachment->AddChild(this);
	}

	// TODO: 트랜스폼 설정

	MarkAsDirty();
}

void USceneComponent::AddChild(USceneComponent* NewChild)
{
	Children.push_back(NewChild);
}

void USceneComponent::RemoveChild(USceneComponent* ChildDeleted)
{
	Children.erase(std::remove(Children.begin(), Children.end(), this), Children.end());
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
