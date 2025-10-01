#include "pch.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Utility/Public/JsonSerializer.h"
#include "Actor/Public/Actor.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Utility/Public/JsonSerializer.h"
#include "Global/Matrix.h"
#include "Global/Quaternion.h"

#include <json.hpp>

IMPLEMENT_CLASS(USceneComponent, UActorComponent)

USceneComponent::USceneComponent()
{
	ComponentType = EComponentType::Scene;

	// Initialize with identity transform
	WorldTransform = FMatrix::Identity();
	WorldTransformInverse = FMatrix::Identity();
	bIsTransformDirty = true;
	bIsTransformDirtyInverse = true;
}

USceneComponent::~USceneComponent()
{
	UE_LOG("USceneComponent::~USceneComponent(): Destroying %s with %d children", 
	       GetName().ToString().c_str(), Children.size());
	
	// 자식들의 부모 참조를 해제 (자식들 자체는 삭제하지 않음 - Actor가 소유하고 있음)
	for (USceneComponent* Child : Children)
	{
		if (Child)
		{
			UE_LOG("  Clearing parent reference for child: %s", 
			       Child->GetName().ToString().c_str());
			Child->ParentAttachment = nullptr;  // 부모 참조만 해제
		}
	}
	Children.clear();
	
	// 내가 다른 컴포넌트의 자식이었다면, 부모에서 나를 제거
	if (ParentAttachment)
	{
		UE_LOG("  Removing self from parent: %s", 
		       ParentAttachment->GetName().ToString().c_str());
		ParentAttachment->RemoveChild(this);
		ParentAttachment = nullptr;
	}
	
	UE_LOG("USceneComponent::~USceneComponent(): Destruction completed for %s", 
	       GetName().ToString().c_str());
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

		// Mark as dirty and update transform after loading
		MarkAsDirty();
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
	if (ParentAttachment)
	{
		ParentAttachment->AddChild(this);
	}

	MarkAsDirty();
	// Immediately update transform after parent change
	UpdateWorldTransform();
}

void USceneComponent::AddChild(USceneComponent* NewChild)
{
	if (!NewChild)
	{
		return;
	}

	Children.push_back(NewChild);

	// Immediately update child's world transform
	if (NewChild)
	{
		NewChild->MarkAsDirty();
		NewChild->UpdateWorldTransform();

		// CRITICAL: Register child PrimitiveComponents with the Level for rendering
		if (UPrimitiveComponent* PrimitiveChild = Cast<UPrimitiveComponent>(NewChild))
		{
			// Get current level and register the component
			ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
			if (CurrentLevel)
			{
				CurrentLevel->RegisterPrimitiveComponent(PrimitiveChild);
			}
		}
	}

	// 자식 컴포넌트를 Actor의 소유 목록에도 추가
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && NewChild->GetOwner() != OwnerActor)
	{
		OwnerActor->RegisterComponent(NewChild);
	}
}

void USceneComponent::RemoveChild(USceneComponent* ChildDeleted)
{
	if (!ChildDeleted)
	{
		return;
	}

	Children.erase(std::remove(Children.begin(), Children.end(), ChildDeleted), Children.end());

	// 자식 컴포넌트를 Actor의 소유 목록에서도 제거
	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		OwnerActor->UnregisterComponent(ChildDeleted);
	}
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
	// Immediately update world transform so changes are visible right away
	UpdateWorldTransform();
}

void USceneComponent::SetRelativeRotation(const FVector& Rotation)
{
	RelativeRotation = Rotation;
	MarkAsDirty();
	// Immediately update world transform so changes are visible right away
	UpdateWorldTransform();
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
	// Immediately update world transform so changes are visible right away
	UpdateWorldTransform();
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
	// ToEuler() already returns degrees, no need to convert
	return FQuaternion::FromMatrix(WorldTransform).ToEuler();
}

FVector USceneComponent::GetWorldScale3D() const
{
	return WorldTransform.GetScale();
}

void USceneComponent::UpdateWorldTransform()
{
	if (!bIsTransformDirty && !bIsTransformDirtyInverse)
	{
		return;
	}

	// Update WorldTransform if needed
	if (bIsTransformDirty)
	{
		if (ParentAttachment)
		{
			// CRITICAL: Ensure parent is updated first
			ParentAttachment->UpdateWorldTransform();

			// For child components: build local transform same as root (without UEToDx)
			FMatrix T = FMatrix::TranslationMatrix(RelativeLocation);
			FMatrix R = FMatrix::RotationMatrix(FVector::GetDegreeToRadian(RelativeRotation));
			FMatrix S = FMatrix::ScaleMatrix(RelativeScale3D);
			FMatrix LocalTransform = S * R * T;

			// Debug logging
			FVector ParentWorldPos = ParentAttachment->GetWorldTransform().GetLocation();
			FVector ChildRelativePos = RelativeLocation;
			UE_LOG("Child Transform Debug: Parent World Pos=(%.2f, %.2f, %.2f), Child Relative=(%.2f, %.2f, %.2f), Child Scale=(%.2f, %.2f, %.2f)",
				ParentWorldPos.X, ParentWorldPos.Y, ParentWorldPos.Z,
				ChildRelativePos.X, ChildRelativePos.Y, ChildRelativePos.Z,
				RelativeScale3D.X, RelativeScale3D.Y, RelativeScale3D.Z);

			// Apply in parent's world space
			WorldTransform = LocalTransform * ParentAttachment->GetWorldTransform();

			FVector ChildWorldPos = WorldTransform.GetLocation();
			UE_LOG("  -> Child World Pos=(%.2f, %.2f, %.2f)", ChildWorldPos.X, ChildWorldPos.Y, ChildWorldPos.Z);
		}
		else
		{
			// Root component: apply UEToDx coordinate conversion
			WorldTransform = FMatrix::GetModelMatrix(RelativeLocation, FVector::GetDegreeToRadian(RelativeRotation), RelativeScale3D);
		}

		bIsTransformDirty = false;
		// WorldTransform changed, so inverse must be updated too
		bIsTransformDirtyInverse = true;
	}

	// IMPORTANT: Update inverse transform whenever WorldTransform changes
	if (bIsTransformDirtyInverse)
	{
		// Calculate inverse using the same transform components that created WorldTransform
		FVector WorldLocation, WorldRotation, WorldScale;
		if (ParentAttachment)
		{
			// For child components, we need to compute world-space transform components
			// This is a simplified approach - in practice, you might want more sophisticated decomposition
			WorldLocation = RelativeLocation;
			WorldRotation = RelativeRotation;
			WorldScale = RelativeScale3D;
		}
		else
		{
			// For root components, relative = world
			WorldLocation = RelativeLocation;
			WorldRotation = RelativeRotation;
			WorldScale = RelativeScale3D;
		}
		
		WorldTransformInverse = FMatrix::GetModelMatrixInverse(WorldLocation, WorldRotation, WorldScale);
		bIsTransformDirtyInverse = false;
	}

	bIsTransformDirty = false;

	for (USceneComponent* Child : Children)
	{
		Child->UpdateWorldTransform();
	}
}
