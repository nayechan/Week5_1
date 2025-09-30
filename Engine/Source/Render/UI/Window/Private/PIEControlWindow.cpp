#include "pch.h"
#include "Render/UI/Window/Public/PIEControlWindow.h"
#include "Render/UI/Widget/Public/PIEControlWidget.h"

UPIEControlWindow::UPIEControlWindow()
{
	if (PIEControlWidget = new UPIEControlWidget())
	{
		FUIWindowConfig Config;
		Config.WindowTitle = "PIE Control";
		Config.DefaultSize = ImVec2(256, 64);
		Config.DefaultPosition = ImVec2(1600, 200);
		Config.DockDirection = EUIDockDirection::None;
		//Config.Priority = 999;
		Config.bResizable = false;
		Config.bMovable = false;
		Config.bCollapsible = false;

		// 메뉴바만 보이도록 하기 위해 전체적으로 숨김 처리
		Config.WindowFlags = ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoScrollbar;

		SetConfig(Config);

		PIEControlWidget->Initialize();
		AddWidget(PIEControlWidget);
	}
	else
	{
		return;
	}
}
void UPIEControlWindow::Cleanup()
{

}
void UPIEControlWindow::OnFocusGained()
{

}
void UPIEControlWindow::OnFocusLost()
{

}
void UPIEControlWindow::Initialize()
{

}

