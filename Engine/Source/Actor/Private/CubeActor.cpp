#include "pch.h"
#include "Actor/Public/CubeActor.h"
#include "Component/Mesh/Public/CubeComponent.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(ACubeActor, AActor)

ACubeActor::ACubeActor()
{
	CubeComponent = CreateDefaultSubobject<UCubeComponent>("CubeComponent");
	SetRootComponent(CubeComponent);

	// UUID Text Component 세팅
	UUIDTextComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDTextComponent");
	UUIDTextComponent->SetParentAttachment(GetRootComponent());
	UUIDTextComponent->SetText("UUID : " + std::to_string(GetUUID()));
	UUIDTextComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
}
