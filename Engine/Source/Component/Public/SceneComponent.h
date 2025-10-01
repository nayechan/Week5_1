#pragma once
#include "Component/Public/ActorComponent.h"

namespace json { class JSON; }
using JSON = json::JSON;

UCLASS()
class USceneComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USceneComponent, UActorComponent)

public:
	USceneComponent();
	virtual ~USceneComponent();

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void SetParentAttachment(USceneComponent* SceneComponent);
	void AddChild(USceneComponent* NewChild);
	void RemoveChild(USceneComponent* ChildDeleted);

	const TArray<USceneComponent*>& GetAttachChildren() const { return Children; }
	USceneComponent* GetAttachParent() const { return ParentAttachment; }

	void MarkAsDirty();

	void SetRelativeLocation(const FVector& Location);
	void SetRelativeRotation(const FVector& Rotation);
	void SetRelativeScale3D(const FVector& Scale);
	void SetUniformScale(bool bIsUniform);

	bool IsUniformScale() const;

	const FVector& GetRelativeLocation() const;
	const FVector& GetRelativeRotation() const;
	const FVector& GetRelativeScale3D() const;

	FVector GetWorldLocation() const;
	FVector GetWorldRotation() const;
	FVector GetWorldScale3D() const;

	const FMatrix& GetWorldTransform() const;
	const FMatrix& GetWorldTransformInverse() const;
	void UpdateWorldTransform();

private:
	mutable bool bIsTransformDirty = true;
	mutable bool bIsTransformDirtyInverse = true;
	mutable FMatrix WorldTransform;
	mutable FMatrix WorldTransformInverse;

	USceneComponent* ParentAttachment = nullptr;
	TArray<USceneComponent*> Children;
	FVector RelativeLocation = FVector{ 0,0,0.f };
	FVector RelativeRotation = FVector{ 0,0,0.f };
	FVector RelativeScale3D = FVector{ 0.3f,0.3f,0.3f };
	bool bIsUniformScale = false;
	const float MinScale = 0.01f;
};
