#pragma once
#include "Component/Public/PrimitiveComponent.h"

UCLASS()
class USquareComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USquareComponent, UPrimitiveComponent)

public:
	USquareComponent();
};
