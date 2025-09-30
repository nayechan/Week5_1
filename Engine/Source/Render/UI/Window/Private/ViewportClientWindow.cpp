#include "pch.h"
#include "Render/UI/Window/Public/ViewportClientWindow.h"
#include "Render/UI/Widget/Public/ViewportMenuBarWidget.h"
#include "Render/Renderer/Public/Renderer.h" 
#include "Editor/Public/ViewportClient.h"

UViewportClientWindow::UViewportClientWindow()
{
	SetupConfig();

	// 위젯 생성 및 초기화
	if (ViewportMenuBarWidget = new UViewportMenuBarWidget())
	{
		if (FViewport* ViewportClient = URenderer::GetInstance().GetViewportClient())
		{
			ViewportMenuBarWidget->SetViewportClient(ViewportClient);
			ViewportMenuBarWidget->Initialize();
			AddWidget(ViewportMenuBarWidget);
			UE_LOG("ViewportClientWindow: ViewportMenuBarWidget이 생성되고 초기화되었습니다");
		}
	}
	else
	{
		UE_LOG("ViewportClientWindow: Error: ViewportMenuBarWidget 생성에 실패했습니다!");
		return;
	}

	UE_LOG("ViewportClientWindow: 메인 메뉴 윈도우가 초기화되었습니다");
}

void UViewportClientWindow::Initialize()
{
	UE_LOG("ViewportClientWindow: Window가 성공적으로 생성되었습니다.");

	SetWindowState(EUIWindowState::Visible);
}

void UViewportClientWindow::SetupConfig()
{
	const D3D11_VIEWPORT& ViewportInfo = URenderer::GetInstance().GetDeviceResources()->GetViewportInfo();
	FUIWindowConfig Config;
	Config.WindowTitle = "ViewportClient";
	Config.DefaultSize = ImVec2(ViewportInfo.Width, ViewportInfo.Height);
	Config.DefaultPosition = ImVec2(0, 0);
	Config.DockDirection = EUIDockDirection::None;
	Config.Priority = 999;
	Config.bResizable = false;
	Config.bMovable = false;
	Config.bCollapsible = false;

	// 메뉴바만 보이도록 하기 위해 전체적으로 숨김 처리
	Config.WindowFlags = ImGuiWindowFlags_NoTitleBar |
						 ImGuiWindowFlags_NoResize |
						 ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoCollapse |
						 ImGuiWindowFlags_NoScrollbar |
						 ImGuiWindowFlags_NoBackground |
						 ImGuiWindowFlags_NoBringToFrontOnFocus |
						 ImGuiWindowFlags_NoNavFocus |
						 ImGuiWindowFlags_NoDecoration |
						 ImGuiWindowFlags_NoInputs;

	SetConfig(Config);
}
