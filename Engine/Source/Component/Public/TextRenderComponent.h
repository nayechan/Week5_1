#pragma once

#include "Component/Public/PrimitiveComponent.h"

class UMaterial;

UCLASS()
class UTextRenderComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UTextRenderComponent, UPrimitiveComponent)

public:
	UTextRenderComponent();
	~UTextRenderComponent() override;

	void SetText(const FString& InText) { Text = InText; }
	const FString& GetText() const { return Text; }
	void SetMaterial(UMaterial* InMaterial) { Material = InMaterial; }
	UMaterial* GetMaterial() const { return Material; }

private:
	FString Text;
	UMaterial* Material = nullptr;
};
