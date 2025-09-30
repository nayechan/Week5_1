#pragma once
#include "Widget.h"

class UUIManager;

/**
 * @brief ImGui 메인 메뉴바를 관리하는 위젯 클래스
 * 화면 상단에 메인 메뉴바를 표시하고 다른 윈도우들의 토글 기능을 제공
 */
UCLASS()
class UMainBarWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UMainBarWidget, UWidget)

public:
	UMainBarWidget();
	~UMainBarWidget() override = default;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	float GetMenuBarHeight() const { return MenuBarHeight; }
	bool IsMenuBarVisible() const { return bIsMenuBarVisible; }

private:
	bool bIsMenuBarVisible = true;
	float MenuBarHeight = 0.0f;
	TObjectPtr<UUIManager> UIManager = nullptr;

	void RenderWindowsMenu() const;

	static void RenderFileMenu();
	static void RenderViewMenu();
	static void RenderShowFlagsMenu();
	static void RenderHelpMenu();

	static void SaveCurrentLevel();
	static void LoadLevel();
	static void CreateNewLevel();

	static path OpenSaveFileDialog();
	static path OpenLoadFileDialog();

};
