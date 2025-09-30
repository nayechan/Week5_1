#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/StaticMeshActor.h"

IMPLEMENT_CLASS(AStaticMeshActor, AActor)

AStaticMeshActor::AStaticMeshActor()
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	SetRootComponent(StaticMeshComponent);
}
