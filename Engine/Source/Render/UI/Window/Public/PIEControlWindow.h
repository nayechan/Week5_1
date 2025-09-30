#pragma once
#include "UIWindow.h"
#include "Render/UI/Widget/Public/PIEControlWidget.h"
class UPIEControlWindow :
    public UUIWindow
{
public:
	UPIEControlWindow();
	virtual ~UPIEControlWindow() override {}

	virtual void Cleanup();
	virtual void OnFocusGained();
	virtual void OnFocusLost();

	void Initialize() override;

private:
	TObjectPtr<UPIEControlWidget> PIEControlWidget = nullptr;
};

