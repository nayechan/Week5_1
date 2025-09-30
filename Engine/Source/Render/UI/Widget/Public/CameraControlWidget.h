#pragma once
#include "Widget.h"

class UCamera;

class UCameraControlWidget : public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;
	void SyncFromCamera(UCamera* InCamera);
	void PushToCamera(UCamera* InCamera);

	// Setter
	void SetCamera(UCamera* InCamera, const int Index);

	// Special Member Function
	UCameraControlWidget();
	~UCameraControlWidget() override;

private:
	TArray<UCamera*> Cameras{};
	float UiFovY = 80.f;
	float UiNearZ = 0.1f;
	float UiFarZ = 1000.f;
	int   CameraModeIndex = 0;
};
