#include "pch.h"
#include "Actor/Public/SphereActor.h"
#include "Component/Mesh/Public/SphereComponent.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(ASphereActor, AActor)

ASphereActor::ASphereActor()
{
	SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
	SetRootComponent(SphereComponent);

	// UUID Text Component 세팅
	UUIDTextComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDTextComponent");
	UUIDTextComponent->SetParentAttachment(GetRootComponent());
	UUIDTextComponent->SetText("UUID : " + std::to_string(GetUUID()));
	UUIDTextComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
	UUIDTextComponent->SetEnableBillboard(true);
}

