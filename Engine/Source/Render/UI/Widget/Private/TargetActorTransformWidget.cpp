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
		if (LastSelectedComponent != TargetComponent)
		{
			LastSelectedComponent = TargetComponent;
			EditLocation = TargetComponent->GetRelativeLocation();
			EditRotation = TargetComponent->GetRelativeRotation();
			EditScale = TargetComponent->GetRelativeScale3D();
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
			TargetComponent->SetRelativeLocation(EditLocation);
		}
		if (bRotationChanged)
		{
			TargetComponent->SetRelativeRotation(EditRotation);
		}
		if (bScaleChanged)
		{
			TargetComponent->SetRelativeScale3D(EditScale);
		}
	}
	else
	{
		LastSelectedComponent = nullptr;
	}
	ImGui::Separator();
}
