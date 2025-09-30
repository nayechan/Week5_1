#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"

class AActor;

class UBillBoardComponent : public UPrimitiveComponent
{
public:
	UBillBoardComponent(AActor* InOwnerActor, float InYOffset);
	~UBillBoardComponent();

	void UpdateRotationMatrix(const FVector& InCameraLocation);

	FMatrix GetRTMatrix() const { return RTMatrix; }
private:
	FMatrix RTMatrix;
	AActor* POwnerActor;
	float ZOffset;
};
