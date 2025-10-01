#pragma once
#include "Widget.h"

class UTargetActorTransformWidget
	: public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Special Member Function
	UTargetActorTransformWidget();
	~UTargetActorTransformWidget() override;

private:
	// 마지막으로 선택했던 컴포넌트를 기억해서, 선택이 변경되었는지 확인하는 용도
	TObjectPtr<USceneComponent> LastSelectedComponent = nullptr;

	// UI에 표시하고 수정할 Transform 값
	FVector EditLocation = FVector::Zero();
	FVector EditRotation = FVector::Zero();
	FVector EditScale = FVector::One();

	// Transform 값이 변경되었는지 여부를 나타내는 플래그
	bool bScaleChanged = false;
	bool bRotationChanged = false;
	bool bPositionChanged = false;
};
