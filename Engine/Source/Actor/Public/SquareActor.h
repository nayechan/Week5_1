#pragma once
#include "Actor/Public/Actor.h"

class USquareComponent;
class UTextRenderComponent;

UCLASS()
class ASquareActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ASquareActor, AActor)

	using Super = AActor;
public:
	ASquareActor();
	virtual ~ASquareActor() override {}

private:
	USquareComponent* SquareComponent = nullptr;
	UTextRenderComponent* UUIDRenderComponent = nullptr;
};
