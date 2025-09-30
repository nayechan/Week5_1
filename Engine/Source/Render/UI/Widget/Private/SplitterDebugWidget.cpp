#include "pch.h"
#include "Render/UI/Widget/Public/SplitterDebugWidget.h"
#include "Editor/Public/SplitterWindow.h"
#include "Manager/Input/Public/InputManager.h"

USplitterDebugWidget::~USplitterDebugWidget()
{
	RootSplitter = nullptr;
	LeftSplitter = nullptr;
	RightSplitter = nullptr;
}

void USplitterDebugWidget::RenderWidget()
{
	if (!RootSplitter || !LeftSplitter || !RightSplitter || !bSplittersVisible) { return; }

	// 화면의 최상단에 그려지도록 포그라운드 드로우 리스트를 사용합니다.
	ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

	const FPoint MousePosition = { UInputManager::GetInstance().GetMousePosition().X, UInputManager::GetInstance().GetMousePosition().Y };
	const ImU32 SplitterColor = IM_COL32(11, 11, 11, 100);
	const ImU32 HoveredSplitterColor = IM_COL32(255, 255, 255, 100);

	// 루트 스플리터 그리기
	const FRect& RootRect = RootSplitter->Rect;
	DrawList->AddRectFilled(
		ImVec2(RootRect.Left, RootRect.Top),
		ImVec2(RootRect.GetRight(), RootRect.GetBottom()),
		RootSplitter->IsHovered(MousePosition) ? HoveredSplitterColor : SplitterColor);

	// 왼쪽 스플리터 그리기
	const FRect& LeftRect = LeftSplitter->Rect;
	DrawList->AddRectFilled(
		ImVec2(LeftRect.Left, LeftRect.Top),
		ImVec2(LeftRect.GetRight(), LeftRect.GetBottom()),
		LeftSplitter->IsHovered(MousePosition) ? HoveredSplitterColor : SplitterColor);

	// 오른쪽 스플리터 그리기
	const FRect& RightRect = RightSplitter->Rect;
	DrawList->AddRectFilled(
		ImVec2(RightRect.Left, RightRect.Top),
		ImVec2(RightRect.GetRight(), RightRect.GetBottom()),
		RightSplitter->IsHovered(MousePosition) ? HoveredSplitterColor : SplitterColor);
}
