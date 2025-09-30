#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"

class AActor;

class UTextBillboardComponent : public UPrimitiveComponent
{
public:
	UTextBillboardComponent(AActor* InOwnerActor, float InYOffset);
	~UTextBillboardComponent();

	void UpdateRotationMatrix(const FVector& InCameraLocation);

	FMatrix GetRTMatrix() const { return RTMatrix; }
private:
	FMatrix RTMatrix;
	AActor* POwnerActor;
	float ZOffset;
};
