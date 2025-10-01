#pragma once

#include "Component/Public/PrimitiveComponent.h"

class UTexture;
class UCamera;

UCLASS()
class UBillboardComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UBillboardComponent, UPrimitiveComponent)

public:
	UBillboardComponent();
	~UBillboardComponent() override;

	void UpdateFacingCamera(const UCamera* InCamera, bool bYawOnly = true);

	void SetSprite(const FName& InFilePath);

	UTexture* GetSprite() const { return Sprite; }

private:
	UTexture* Sprite = nullptr;
};
