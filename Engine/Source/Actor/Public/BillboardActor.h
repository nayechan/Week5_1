#pragma once

#include "Actor/Public/Actor.h"

class UBillboardComponent;
class UTextRenderComponent;

UCLASS()
class UBillboardActor : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(UBillboardActor, AActor)

public:
	UBillboardActor();

	virtual UTextRenderComponent* GetUUIDTextComponent() const override { return TextRenderComponent; }

private:
	UBillboardComponent* BillboardComponent = nullptr;
	UTextRenderComponent* TextRenderComponent = nullptr;
};
