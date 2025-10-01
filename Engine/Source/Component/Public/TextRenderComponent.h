#pragma once

#include "Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"

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

	void UpdateRotationMatrix(const FVector& InCameraLocation);
	FMatrix GetRTMatrix() const { return RTMatrix; }

	void SetEnableBillboard(bool bInEnable) { bEnableBillboard = bInEnable; }
	bool IsBillboardEnabled() const { return bEnableBillboard; }

private:
	FString Text;
	FMatrix RTMatrix;
	bool bEnableBillboard = false;
};
