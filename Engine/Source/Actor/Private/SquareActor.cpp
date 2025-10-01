#include "pch.h"
#include "Actor/Public/SquareActor.h"
#include "Component/Mesh/Public/SquareComponent.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(ASquareActor, AActor)

ASquareActor::ASquareActor()
{
	SquareComponent = CreateDefaultSubobject<USquareComponent>("SquareComponent");
	SquareComponent->SetRelativeRotation({ 90, 0, 0 });
	SetRootComponent(SquareComponent);

	// UUIDRenderComponent 세팅
	UUIDRenderComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDRenderComponent");
	UUIDRenderComponent->SetParentAttachment(GetRootComponent());
	UUIDRenderComponent->SetText(std::to_string(GetUUID()));
	UUIDRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
}
