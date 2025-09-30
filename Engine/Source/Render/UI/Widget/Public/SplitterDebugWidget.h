#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class SSplitter;
class UEditor;

class USplitterDebugWidget : public UWidget
{
public:
	USplitterDebugWidget(const FString& InName) : UWidget(InName) {}
	virtual ~USplitterDebugWidget() override;

	void Initialize() override {}
	void Update() override {}
	void RenderWidget() override;

	void SetSplitters(const SSplitter* InRoot, const SSplitter* InLeft, const SSplitter* InRight)
	{
		RootSplitter = InRoot;
		LeftSplitter = InLeft;
		RightSplitter = InRight;
	}

	void SetVisible(bool bInVisible) { bSplittersVisible = bInVisible; }

private:
	// 이름 없이 객체 생성 방지를 위해 추가
	USplitterDebugWidget() = delete;

	const SSplitter* RootSplitter = nullptr;
	const SSplitter* LeftSplitter = nullptr;
	const SSplitter* RightSplitter = nullptr;
	bool bSplittersVisible = true;  // 스플리터 표시 여부
};
