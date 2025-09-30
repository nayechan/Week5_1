#pragma once
#include "Actor/Public/Actor.h"

class UCubeComponent;

UCLASS()
class ACubeActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(ACubeActor, AActor)

public:
	ACubeActor();

private:
	UCubeComponent* CubeComponent = nullptr;
};
