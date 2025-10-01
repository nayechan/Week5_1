#include "pch.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(UTextRenderComponent, UPrimitiveComponent)

UTextRenderComponent::UTextRenderComponent()
{
	Type = EPrimitiveType::TextRender;
}

UTextRenderComponent::~UTextRenderComponent()
{}
