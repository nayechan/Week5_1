#include "pch.h"
#include "Render/UI/Widget/Public/PIEControlWidget.h"
#include "Manager/Level/Public/LevelManager.h"
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
	// Start 버튼
	if (ImGui::Button("Start"))
	{
		ULevelManager& LevelManager = ULevelManager::GetInstance();
		FViewport* Viewport = URenderer::GetInstance().GetViewportClient();

		if (Viewport)
		{
			FViewportClient* ActiveViewport = Viewport->GetActiveViewportClient();
			if (ActiveViewport)
			{
				// PIE 시작
				LevelManager.StartPIE();

				// Active Viewport에 PIE Level 할당
				if (LevelManager.GetPIELevel())
				{
					ActiveViewport->RenderTargetLevel = LevelManager.GetPIELevel();
					UE_LOG("PIEControlWidget: PIE Started on Active Viewport");
				}
			}
			else
			{
				UE_LOG("PIEControlWidget: No active viewport");
			}
		}
	}

	ImGui::SameLine();

	// Pause 버튼 (TODO: 구현 예정)
	ImGui::Button("Pause");

	ImGui::SameLine();

	// Stop 버튼
	if (ImGui::Button("Stop"))
	{
		ULevelManager& LevelManager = ULevelManager::GetInstance();
		FViewport* Viewport = URenderer::GetInstance().GetViewportClient();

		if (Viewport)
		{
			FViewportClient* ActiveViewport = Viewport->GetActiveViewportClient();
			if (ActiveViewport)
			{
				// Active Viewport의 RenderTargetLevel 초기화
				ActiveViewport->RenderTargetLevel = nullptr;
				UE_LOG("PIEControlWidget: PIE Viewport cleared");
			}
		}

		// PIE 종료 (Level 삭제)
		LevelManager.StopPIE();
		UE_LOG("PIEControlWidget: PIE Stopped");
	}
}
