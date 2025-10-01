#pragma once
#include "Widget.h"

class UPIEControlWidget : public UWidget
{
public:
	UPIEControlWidget() : UWidget("PIE Control Widget"){}
	virtual void Initialize();
	virtual void Update();
	virtual void RenderWidget();
};

