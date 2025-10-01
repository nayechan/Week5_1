#include "pch.h"
#include "Component/Public/TextRenderComponent.h"

IMPLEMENT_CLASS(UTextRenderComponent, UPrimitiveComponent)

UTextRenderComponent::UTextRenderComponent()
{
	Type = EPrimitiveType::TextRender;
}

UTextRenderComponent::~UTextRenderComponent()
{}

void UTextRenderComponent::UpdateRotationMatrix(const FVector& InCameraLocation)
{
	const FVector& OwnerActorLocation = GetOwner()->GetActorLocation();

	FVector ToCamera = InCameraLocation - OwnerActorLocation;
	ToCamera.Normalize();

	const FVector4 worldUp4 = FVector4(0, 0, 1, 1);
	const FVector worldUp = { worldUp4.X, worldUp4.Y, worldUp4.Z };
	FVector Right = worldUp.Cross(ToCamera);
	Right.Normalize();
	FVector Up = ToCamera.Cross(Right);
	Up.Normalize();

	RTMatrix = FMatrix(FVector4(0, 1, 0, 1), worldUp4, FVector4(1, 0, 0, 1));
	RTMatrix = FMatrix(ToCamera, Right, Up);

	const FVector Translation = OwnerActorLocation + GetRelativeLocation();
	RTMatrix *= FMatrix::TranslationMatrix(Translation);
}
