#pragma once
#include "Render/UI/Window/Public/UIWindow.h"

class USplitterDebugWidget;

/**
 * @brief Editor의 전반적인 UI를 담당하는 Window
 * 스플리터 UI 렌더링, 기즈모 조작 등 Editor 관련 위젯을 포함합니다.
 */
class UEditorWindow : public UUIWindow
{
public:
	UEditorWindow();
	virtual ~UEditorWindow() override {}

	void Initialize() override;

private:
	TObjectPtr<USplitterDebugWidget> SplitterDebugWidget = nullptr;
};
