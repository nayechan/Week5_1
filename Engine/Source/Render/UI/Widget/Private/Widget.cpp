#include "pch.h"
#include "Render/UI/Widget/Public/Widget.h"

IMPLEMENT_CLASS(UWidget, UObject);

UWidget::UWidget(const FString& InName)
	: UObject(InName)
{
}
