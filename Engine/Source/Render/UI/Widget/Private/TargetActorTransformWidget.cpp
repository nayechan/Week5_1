#include "pch.h"
#include "Render/UI/Widget/Public/TargetActorTransformWidget.h"

#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"

UTargetActorTransformWidget::UTargetActorTransformWidget()
	: UWidget("Target Actor Tranform Widget")
{
}

UTargetActorTransformWidget::~UTargetActorTransformWidget() = default;

void UTargetActorTransformWidget::Initialize()
{
	// Do Nothing Here
}

void UTargetActorTransformWidget::Update()
{
	// 매 프레임 Level의 선택된 Actor를 확인해서 정보 반영
	// TODO(KHJ): 적절한 위치를 찾을 것
	ULevelManager& LevelManager = ULevelManager::GetInstance();
	ULevel* CurrentLevel = LevelManager.GetCurrentLevel();

	LevelMemoryByte = CurrentLevel->GetAllocatedBytes();
	LevelObjectCount = CurrentLevel->GetAllocatedCount();

	if (CurrentLevel)
	{
		AActor* CurrentSelectedActor = CurrentLevel->GetSelectedActor();

		// Update Current Selected Actor
		if (SelectedActor != CurrentSelectedActor)
		{
			SelectedActor = CurrentSelectedActor;
		}

		// Get Current Selected Actor Information
		if (SelectedActor)
		{
			UpdateTransformFromActor();
		}
		else if (CurrentSelectedActor)
		{
			SelectedActor = nullptr;
		}
	}
}

void UTargetActorTransformWidget::RenderWidget()
{
	// Memory Information
	// ImGui::Text("레벨 메모리 정보");
	// ImGui::Text("Level Object Count: %u", LevelObjectCount);
	// ImGui::Text("Level Memory: %.3f KB", static_cast<float>(LevelMemoryByte) / KILO);
	// ImGui::Separator();

	if (SelectedActor)
	{
		ImGui::Separator();
		ImGui::Text("Transform");

		ImGui::Spacing();

		bPositionChanged |= ImGui::DragFloat3("Location", &EditLocation.X, 0.1f);
		bRotationChanged |= ImGui::DragFloat3("Rotation", &EditRotation.X, 0.1f);

		// Uniform Scale 옵션
		bool bUniformScale = SelectedActor->IsUniformScale();
		if (bUniformScale)
		{
			float UniformScale = EditScale.X;

			if (ImGui::DragFloat("Scale", &UniformScale, 0.01f, 0.01f, 10.0f))
			{
				EditScale = FVector(UniformScale, UniformScale, UniformScale);
				bScaleChanged = true;
			}
		}
		else
		{
			bScaleChanged |= ImGui::DragFloat3("Scale", &EditScale.X, 0.1f);
		}

		ImGui::Checkbox("Uniform Scale", &bUniformScale);

		SelectedActor->SetUniformScale(bUniformScale);
	}

	ImGui::Separator();
}

/**
 * @brief Render에서 체크된 내용으로 인해 이후 변경되어야 할 내용이 있다면 Change 처리
 */
void UTargetActorTransformWidget::PostProcess()
{
	if (bPositionChanged || bRotationChanged || bScaleChanged)
	{
		ApplyTransformToActor();
	}
}

void UTargetActorTransformWidget::UpdateTransformFromActor()
{
	if (SelectedActor)
	{
		EditLocation = SelectedActor->GetActorLocation();
		EditRotation = SelectedActor->GetActorRotation();
		EditScale = SelectedActor->GetActorScale3D();
	}
}

void UTargetActorTransformWidget::ApplyTransformToActor() const
{
	if (SelectedActor)
	{
		SelectedActor->SetActorLocation(EditLocation);
		SelectedActor->SetActorRotation(EditRotation);
		SelectedActor->SetActorScale3D(EditScale);
	}
}
