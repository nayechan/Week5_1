#include "pch.h"
#include "Render/UI/Widget/Public/TargetActorTransformWidget.h"
#include "Manager/UI/Public/UIManager.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"

UTargetActorTransformWidget::UTargetActorTransformWidget()
	: UWidget("Target Actor Tranform Widget")
{
}

UTargetActorTransformWidget::~UTargetActorTransformWidget() = default;

void UTargetActorTransformWidget::Initialize()
{
}

void UTargetActorTransformWidget::Update()
{
}

void UTargetActorTransformWidget::RenderWidget()
{
	TObjectPtr<UObject> SelectedObject = UUIManager::GetInstance().GetSelectedObject();
	USceneComponent* TargetComponent = nullptr;

	if (SelectedObject)
	{
		if (AActor* SelectedActor = Cast<AActor>(SelectedObject.Get()))
		{
			TargetComponent = SelectedActor->GetRootComponent();
		}
		else if (UActorComponent* SelectedComponent = Cast<UActorComponent>(SelectedObject.Get()))
		{
			TargetComponent = Cast<USceneComponent>(SelectedComponent);
		}
	}

	if (TargetComponent)
	{
		bool bIsChildComponent = (TargetComponent->GetAttachParent() != nullptr);

		if (LastSelectedComponent != TargetComponent)
		{
			LastSelectedComponent = TargetComponent;

			if (bIsChildComponent)
			{
				// Child components store RelativeLocation in parent's DX space
				// Convert DX -> UE for display: (X_dx, Y_dx, Z_dx) -> (Z_dx, X_dx, Y_dx)
				FVector DxLoc = TargetComponent->GetRelativeLocation();
				EditLocation = FVector(DxLoc.Z, DxLoc.X, DxLoc.Y);

				FVector DxRot = TargetComponent->GetRelativeRotation();
				EditRotation = FVector(DxRot.Z, DxRot.X, DxRot.Y);

				FVector DxScale = TargetComponent->GetRelativeScale3D();
				EditScale = FVector(DxScale.Z, DxScale.X, DxScale.Y);
			}
			else
			{
				// Root components already use UE space
				EditLocation = TargetComponent->GetRelativeLocation();
				EditRotation = TargetComponent->GetRelativeRotation();
				EditScale = TargetComponent->GetRelativeScale3D();
			}
			bPositionChanged = bRotationChanged = bScaleChanged = false;
		}

		ImGui::Separator();
		ImGui::Text("Transform");
		ImGui::Spacing();

		bPositionChanged |= ImGui::DragFloat3("Location", &EditLocation.X, 0.1f);
		bRotationChanged |= ImGui::DragFloat3("Rotation", &EditRotation.X, 0.1f);
		bScaleChanged |= ImGui::DragFloat3("Scale", &EditScale.X, 0.01f);

		if (bPositionChanged)
		{
			if (bIsChildComponent)
			{
				// Convert UE -> DX before storing: (X_ue, Y_ue, Z_ue) -> (Y_ue, Z_ue, X_ue)
				FVector DxLoc(EditLocation.Y, EditLocation.Z, EditLocation.X);
				TargetComponent->SetRelativeLocation(DxLoc);
			}
			else
			{
				TargetComponent->SetRelativeLocation(EditLocation);
			}
		}
		if (bRotationChanged)
		{
			if (bIsChildComponent)
			{
				// Convert UE -> DX before storing
				FVector DxRot(EditRotation.Y, EditRotation.Z, EditRotation.X);
				TargetComponent->SetRelativeRotation(DxRot);
			}
			else
			{
				TargetComponent->SetRelativeRotation(EditRotation);
			}
		}
		if (bScaleChanged)
		{
			if (bIsChildComponent)
			{
				// Convert UE -> DX before storing
				FVector DxScale(EditScale.Y, EditScale.Z, EditScale.X);
				TargetComponent->SetRelativeScale3D(DxScale);
			}
			else
			{
				TargetComponent->SetRelativeScale3D(EditScale);
			}
		}
	}
	else
	{
		LastSelectedComponent = nullptr;
	}
	ImGui::Separator();
}
