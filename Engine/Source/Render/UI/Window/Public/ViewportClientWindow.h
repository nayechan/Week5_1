#pragma once
#include "Render/UI/Window/Public/UIWindow.h"

class UViewportMenuBarWidget;

/**
 * @brief 다중 뷰포트의 속성을 제어할 수 있는 UI를 담당하는 Window
 */
class UViewportClientWindow : public UUIWindow
{
public:
	UViewportClientWindow();
	virtual ~UViewportClientWindow() override {}

	void Initialize() override;

private:
	void SetupConfig();

	TObjectPtr<UViewportMenuBarWidget> ViewportMenuBarWidget = nullptr;
};

