#include "pch.h"
#include "Render/UI/Widget/Public/PIEControlWidget.h"
#include "Manager/PIE/Public/PIEManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/Viewport.h"

void UPIEControlWidget::Initialize()
{

}

void UPIEControlWidget::Update()
{

}

void UPIEControlWidget::RenderWidget()
{
	UPIEManager& PIEManager = UPIEManager::GetInstance();

	// Start 버튼
	if (ImGui::Button("Start"))
	{
		FViewport* Viewport = URenderer::GetInstance().GetViewportClient();
		if (Viewport)
		{
			FViewportClient* ActiveViewport = Viewport->GetActiveViewportClient();
			if (ActiveViewport)
			{
				UE_LOG("PIEControlWidget: Starting PIE on Active Viewport (Address: %p)", ActiveViewport);
				PIEManager.StartPIE(ActiveViewport);
			}
			else
			{
				UE_LOG("PIEControlWidget: No active viewport - using first viewport");
				// Active viewport가 없으면 첫 번째 viewport 사용
				auto& viewports = Viewport->GetViewports();
				if (!viewports.empty())
				{
					PIEManager.StartPIE(&viewports[0]);
				}
			}
		}
	}

	ImGui::SameLine();

	// Pause/Resume 버튼
	bool bIsPaused = PIEManager.IsPIEPaused();
	const char* pauseLabel = bIsPaused ? "Resume" : "Pause";

	if (ImGui::Button(pauseLabel))
	{
		PIEManager.TogglePausePIE();
	}

	ImGui::SameLine();

	// Stop 버튼
	if (ImGui::Button("Stop"))
	{
		PIEManager.StopPIE();
	}
}
