#include "pch.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"
#include "Global/Matrix.h"
#include "Global/Quaternion.h"

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

void USceneComponent::SetRelativeLocation(const FVector& Location)
{
	RelativeLocation = Location;
	MarkAsDirty();
}

void USceneComponent::SetRelativeRotation(const FVector& Rotation)
{
	RelativeRotation = Rotation;
	MarkAsDirty();
}

void USceneComponent::SetRelativeScale3D(const FVector& Scale)
{
	FVector ActualScale = Scale;
	if (ActualScale.X < MinScale)
		ActualScale.X = MinScale;
	if (ActualScale.Y < MinScale)
		ActualScale.Y = MinScale;
	if (ActualScale.Z < MinScale)
		ActualScale.Z = MinScale;
	RelativeScale3D = ActualScale;
	MarkAsDirty();
}

void USceneComponent::SetUniformScale(bool bIsUniform)
{
	bIsUniformScale = bIsUniform;
}

bool USceneComponent::IsUniformScale() const
{
	return bIsUniformScale;
}

const FVector& USceneComponent::GetRelativeLocation() const
{
	return RelativeLocation;
}

const FVector& USceneComponent::GetRelativeRotation() const
{
	return RelativeRotation;
}

const FVector& USceneComponent::GetRelativeScale3D() const
{
	return RelativeScale3D;
}

const FMatrix& USceneComponent::GetWorldTransform() const
{
	return WorldTransform;
}

const FMatrix& USceneComponent::GetWorldTransformInverse() const
{
	return WorldTransformInverse;
}

FVector USceneComponent::GetWorldLocation() const
{
	return WorldTransform.GetLocation();
}

FVector USceneComponent::GetWorldRotation() const
{
	return FVector::GetDegreeToRadian(FQuaternion::FromMatrix(WorldTransform).ToEuler());
}

FVector USceneComponent::GetWorldScale3D() const
{
	return WorldTransform.GetScale();
}

void USceneComponent::UpdateWorldTransform()
{
	if (!bIsTransformDirty)
	{
		for (USceneComponent* Child : Children)
		{
			Child->UpdateWorldTransform();
		}
		return;
	}

	const FMatrix RelativeMatrix = FMatrix::GetModelMatrix(RelativeLocation, FVector::GetDegreeToRadian(RelativeRotation), RelativeScale3D);

	if (ParentAttachment)
	{
		WorldTransform = RelativeMatrix * ParentAttachment->GetWorldTransform();
	}
	else
	{
		WorldTransform = RelativeMatrix;
	}

	bIsTransformDirty = false;

	for (USceneComponent* Child : Children)
	{
		Child->UpdateWorldTransform();
	}
}
