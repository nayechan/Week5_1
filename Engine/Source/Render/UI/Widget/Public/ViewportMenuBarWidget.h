#pragma once
#include "Widget.h"

class FViewport;
class UEditor;

class UViewportMenuBarWidget : public UWidget
{
public:
	UViewportMenuBarWidget() : UWidget("ViewportMenuBar Widget") {}
	virtual ~UViewportMenuBarWidget() override;

	void Initialize() override {}
	void Update() override {}
	void RenderWidget() override;

	void SetViewportClient(FViewport* InViewportClient) { Viewport = InViewportClient; }
	void SetEdtior(UEditor* InEditor) { Editor = InEditor; }

private:
	void RenderCameraControls(UCamera& InCamera); // 특정 카메라의 제어 UI를 렌더링하는 헬퍼 함수

	FViewport* Viewport = nullptr; // 참조할 뷰포트 클라이언트 대상
	UEditor* Editor = nullptr;
};

