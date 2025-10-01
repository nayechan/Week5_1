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

	// UUID Text Component 세팅
	UUIDTextComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDTextComponent");
	UUIDTextComponent->SetParentAttachment(GetRootComponent());
	UUIDTextComponent->SetText("UUID : " + std::to_string(GetUUID()));
	UUIDTextComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
	UUIDTextComponent->SetEnableBillboard(true);
}
