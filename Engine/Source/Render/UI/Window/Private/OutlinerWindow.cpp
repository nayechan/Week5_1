#include "pch.h"
#include "Render/UI/Window/Public/OutlinerWindow.h"

#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

UOutlinerWindow::UOutlinerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Outliner";
	Config.DefaultSize = ImVec2(330, 550);
	Config.DefaultPosition = ImVec2(1565, 33); // 메뉴바만큼 하향 이동
	Config.MinSize = ImVec2(270, 50);
	Config.InitialState = EUIWindowState::Minimized; // 기본적으로 축소된 상태로 시작
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.DockDirection = EUIDockDirection::Center;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	AddWidget(new USceneHierarchyWidget);
}

void UOutlinerWindow::Initialize()
{
	UE_LOG("OutlinerWindow: Window가 성공적으로 생성되었습니다");
}
