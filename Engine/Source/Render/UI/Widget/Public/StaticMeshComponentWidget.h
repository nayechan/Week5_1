#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class UStaticMeshComponent;
class UMaterial;

UCLASS()
class UStaticMeshComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshComponentWidget, UWidget)

public:
	void RenderWidget() override;

private:
	// Helper functions for rendering different sections
	void RenderStaticMeshSelector(UStaticMeshComponent* InTargetComponent);
	void RenderMaterialSections(UStaticMeshComponent* InTargetComponent);
	void RenderAvailableMaterials(UStaticMeshComponent* InTargetComponent, int32 TargetSlotIndex);
	void RenderOptions(UStaticMeshComponent* InTargetComponent);

	// Material utility functions
	FString GetMaterialDisplayName(UMaterial* Material) const;
};
