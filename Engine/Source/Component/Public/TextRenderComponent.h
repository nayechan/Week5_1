#pragma once

#include "Component/Public/PrimitiveComponent.h"

UCLASS()
class UTextRenderComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UTextRenderComponent, UPrimitiveComponent)

public:
	UTextRenderComponent();
	~UTextRenderComponent() override;

	void SetText(const FString& InText);
	const FString& GetText() const { return Text; }

private:
	FString Text;
};
