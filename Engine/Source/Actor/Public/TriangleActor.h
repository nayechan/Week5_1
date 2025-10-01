#pragma once
#include "Actor/Public/Actor.h"

class UTriangleComponent;
class UTextRenderComponent;

UCLASS()
class ATriangleActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ATriangleActor, AActor)

	using Super = AActor;
public:
	ATriangleActor();
	virtual ~ATriangleActor() override {}

	virtual UTextRenderComponent* GetUUIDTextComponent() const override { return UUIDTextComponent; }

private:
	UTriangleComponent* TriangleComponent = nullptr;
	UTextRenderComponent* UUIDTextComponent = nullptr;
};
