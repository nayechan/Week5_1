#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(AStaticMeshActor, AActor)

AStaticMeshActor::AStaticMeshActor()
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	SetRootComponent(StaticMeshComponent);

	// UUIDRenderComponent 세팅
	UUIDRenderComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDRenderComponent");
	UUIDRenderComponent->SetParentAttachment(GetRootComponent());
	UUIDRenderComponent->SetText(std::to_string(GetUUID()));
	UUIDRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
}
