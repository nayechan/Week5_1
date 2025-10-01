#pragma once
#include "Actor/Public/Actor.h"

class UCubeComponent;
class UTextRenderComponent;

UCLASS()
class ACubeActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ACubeActor, AActor)

public:
	ACubeActor();

	virtual UTextRenderComponent* GetUUIDTextComponent() const override { return UUIDTextComponent; }

private:
	UCubeComponent* CubeComponent = nullptr;
	UTextRenderComponent* UUIDTextComponent = nullptr;
};
