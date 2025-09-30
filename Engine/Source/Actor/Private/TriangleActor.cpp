#include "pch.h"
#include "Actor/Public/TriangleActor.h"
#include "Component/Mesh/Public/TriangleComponent.h"

IMPLEMENT_CLASS(ATriangleActor, AActor)

ATriangleActor::ATriangleActor()
{
	TriangleComponent = CreateDefaultSubobject<UTriangleComponent>("TriangleComponent");
	TriangleComponent->SetRelativeRotation({ 90, 0, 0 });
	SetRootComponent(TriangleComponent);
}
