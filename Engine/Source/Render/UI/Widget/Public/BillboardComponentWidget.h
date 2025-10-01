#pragma once

#include "Render/UI/Widget/Public/Widget.h"

class UBillboardComponent;
class UTexture;

UCLASS()
class UBillboardComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UBillboardComponentWidget, UWidget)

public:
	void RenderWidget() override;

private:
	UBillboardComponent* BillboardComponent{};

	// Helper functions for rendering different sections
	void RenderSpriteSelector();

	// Sprite utility functions
	FString GetSpriteDisplayName(UTexture* Sprite) const;
};

