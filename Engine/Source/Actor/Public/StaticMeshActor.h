#pragma once
#include "Actor/Public/Actor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"

class UCubeComponent;
class UTextRenderComponent;

UCLASS()
class AStaticMeshActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(AStaticMeshActor, AActor)

public:
	AStaticMeshActor();

	virtual UTextRenderComponent* GetUUIDTextComponent() const override { return UUIDTextComponent; }

private:
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	UTextRenderComponent* UUIDTextComponent = nullptr;
};
