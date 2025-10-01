#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(AStaticMeshActor, AActor)

AStaticMeshActor::AStaticMeshActor()
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	SetRootComponent(StaticMeshComponent);

	// UUID Text Component 세팅
	UUIDTextComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDTextComponent");
	UUIDTextComponent->SetParentAttachment(GetRootComponent());
	UUIDTextComponent->SetText("UUID : " + std::to_string(GetUUID()));
	UUIDTextComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
	UUIDTextComponent->SetEnableBillboard(true);
}
