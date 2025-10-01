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

private:
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	UTextRenderComponent* UUIDRenderComponent = nullptr;
};
