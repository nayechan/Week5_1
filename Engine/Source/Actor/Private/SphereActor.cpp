#include "pch.h"
#include "Actor/Public/SphereActor.h"
#include "Component/Mesh/Public/SphereComponent.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(ASphereActor, AActor)

ASphereActor::ASphereActor()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SetRootComponent(SphereComponent);

	// UUIDRenderComponent 세팅
	UUIDRenderComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDRenderComponent");
	UUIDRenderComponent->SetParentAttachment(GetRootComponent());
	UUIDRenderComponent->SetText(std::to_string(GetUUID()));
	UUIDRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
}

