#pragma once

#include "Actor/Public/Actor.h"

class USphereComponent;
class UTextRenderComponent;

UCLASS()
class ASphereActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ASphereActor, AActor)

public:
	ASphereActor();
private:
	USphereComponent* SphereComponent = nullptr;
	UTextRenderComponent* UUIDRenderComponent = nullptr;
};
