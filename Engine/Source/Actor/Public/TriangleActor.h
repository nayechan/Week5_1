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


private:
	UTriangleComponent* TriangleComponent = nullptr;
	UTextRenderComponent* UUIDRenderComponent = nullptr;
};
