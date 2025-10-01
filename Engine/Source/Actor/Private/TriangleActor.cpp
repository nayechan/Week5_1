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

	// UUID Text Component 세팅
	UUIDTextComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDTextComponent");
	UUIDTextComponent->SetParentAttachment(GetRootComponent());
	UUIDTextComponent->SetText("UUID : " + std::to_string(GetUUID()));
	UUIDTextComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
}
