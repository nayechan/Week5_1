#include "pch.h"
#include "Actor/Public/CubeActor.h"
#include "Component/Mesh/Public/CubeComponent.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(ACubeActor, AActor)

ACubeActor::ACubeActor()
{
	// RootComponent 세팅
	CubeComponent = CreateDefaultSubobject<UCubeComponent>("CubeComponent");
	SetRootComponent(CubeComponent);

	// UUIDRenderComponent 세팅
	UUIDRenderComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDRenderComponent");
	UUIDRenderComponent->SetParentAttachment(GetRootComponent());
	UUIDRenderComponent->SetText(std::to_string(GetUUID()));
	UUIDRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
}
