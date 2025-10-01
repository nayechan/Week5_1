#include "pch.h"
#include "Actor/Public/BillboardActor.h"
#include "Component/Public/BillboardComponent.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(UBillboardActor, AActor)

UBillboardActor::UBillboardActor()
{
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComponent");
	SetRootComponent(BillboardComponent);
	BillboardComponent->SetSprite("Asset/Editor/Icon/Pawn_64x.png");

	// UUID Text Component 세팅
	TextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>("UUIDTextComponent");
	TextRenderComponent->SetParentAttachment(GetRootComponent());
	TextRenderComponent->SetText("UUID : " + std::to_string(GetUUID()));
	TextRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
	TextRenderComponent->SetEnableBillboard(true);
}
