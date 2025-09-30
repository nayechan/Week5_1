#include "pch.h"
#include "Actor/Public/SquareActor.h"
#include "Component/Mesh/Public/SquareComponent.h"

IMPLEMENT_CLASS(ASquareActor, AActor)

ASquareActor::ASquareActor()
{
	SquareComponent = CreateDefaultSubobject<USquareComponent>("SquareComponent");
	SquareComponent->SetRelativeRotation({ 90, 0, 0 });
	SetRootComponent(SquareComponent);
}
