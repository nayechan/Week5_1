#include "pch.h"
#include "Render/UI/Factory/Public/UIWindowFactory.h"

#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Render/UI/Window/Public/ControlPanelWindow.h"
#include "Render/UI/Window/Public/ExperimentalFeatureWindow.h"
#include "Render/UI/Window/Public/OutlinerWindow.h"
#include "Render/UI/Window/Public/DetailWindow.h"
#include "Render/UI/Window/Public/MainMenuWindow.h"
#include "Render/UI/Window/Public/EditorWindow.h"
#include "Render/UI/Window/Public/ViewportClientWindow.h"

UMainMenuWindow& UUIWindowFactory::CreateMainMenuWindow()
{
	UMainMenuWindow& Instance = UMainMenuWindow::GetInstance();
	return Instance;
}

UConsoleWindow* UUIWindowFactory::CreateConsoleWindow(EUIDockDirection InDockDirection)
{
	auto& Window = UConsoleWindow::GetInstance();
	Window.GetMutableConfig().DockDirection = InDockDirection;
	return &Window;
}

UControlPanelWindow* UUIWindowFactory::CreateControlPanelWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UControlPanelWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

UOutlinerWindow* UUIWindowFactory::CreateOutlinerWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UOutlinerWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

UDetailWindow* UUIWindowFactory::CreateDetailWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UDetailWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

UExperimentalFeatureWindow* UUIWindowFactory::CreateExperimentalFeatureWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UExperimentalFeatureWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

UEditorWindow* UUIWindowFactory::CreateEditorWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UEditorWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}

UViewportClientWindow* UUIWindowFactory::CreateViewportClientWindow(EUIDockDirection InDockDirection)
{
	auto* Window = new UViewportClientWindow();
	Window->GetMutableConfig().DockDirection = InDockDirection;
	return Window;
}


void UUIWindowFactory::CreateDefaultUILayout()
{
	auto& UIManager = UUIManager::GetInstance();

	// 메인 메뉴바 우선 생성 및 등록
	auto& MainMenu = CreateMainMenuWindow();
	UIManager.RegisterUIWindow(&MainMenu);
	UIManager.RegisterMainMenuWindow(&MainMenu);

	// 기본 레이아웃 생성
	UIManager.RegisterUIWindow(CreateConsoleWindow(EUIDockDirection::Bottom));
	UIManager.RegisterUIWindow(CreateControlPanelWindow(EUIDockDirection::Left));
	UIManager.RegisterUIWindow(CreateOutlinerWindow(EUIDockDirection::Center));
	UIManager.RegisterUIWindow(CreateDetailWindow(EUIDockDirection::Right));
	UIManager.RegisterUIWindow(CreateExperimentalFeatureWindow(EUIDockDirection::Right));
	UIManager.RegisterUIWindow(CreateEditorWindow(EUIDockDirection::None));
	UIManager.RegisterUIWindow(CreateViewportClientWindow(EUIDockDirection::None));
	UE_LOG_SUCCESS("UIWindowFactory: UI 생성이 성공적으로 완료되었습니다");
}
