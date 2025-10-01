#include "pch.h"
#include "Actor/Public/TriangleActor.h"
#include "Component/Mesh/Public/TriangleComponent.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(ATriangleActor, AActor)

ATriangleActor::ATriangleActor()
{
	TriangleComponent = CreateDefaultSubobject<UTriangleComponent>("TriangleComponent");
	TriangleComponent->SetRelativeRotation({ 90, 0, 0 });
	SetRootComponent(TriangleComponent);

	// UUIDRenderComponent 세팅
	UUIDRenderComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDRenderComponent");
	UUIDRenderComponent->SetParentAttachment(GetRootComponent());
	UUIDRenderComponent->SetText(std::to_string(GetUUID()));
	UUIDRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
}
