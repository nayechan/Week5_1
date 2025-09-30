#include "pch.h"
#include "Component/Public/BillBoardComponent.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Editor/Public/Editor.h"
#include "Actor/Public/Actor.h"

/**
 * @brief Level에서 각 Actor마다 가지고 있는 UUID를 출력해주기 위한 빌보드 클래스
 * Actor has a UBillBoardComponent
 */
UBillBoardComponent::UBillBoardComponent(AActor* InOwnerActor, float InYOffset)
	: POwnerActor(InOwnerActor)
	, ZOffset(InYOffset)
{
	Type = EPrimitiveType::BillBoard;
}

UBillBoardComponent::~UBillBoardComponent()
{
	POwnerActor = nullptr;
}

void UBillBoardComponent::UpdateRotationMatrix(const FVector& InCameraLocation)
{
	const FVector& OwnerActorLocation = POwnerActor->GetActorLocation();

	FVector ToCamera = InCameraLocation - OwnerActorLocation;
	ToCamera.Normalize();

	const FVector4 worldUp4 = FVector4(0, 0, 1, 1);
	const FVector worldUp = { worldUp4.X, worldUp4.Y, worldUp4.Z };
	FVector Right = worldUp.Cross(ToCamera);
	Right.Normalize();
	FVector Up = ToCamera.Cross(Right);
	Up.Normalize();

	RTMatrix = FMatrix(FVector4(0, 1, 0, 1), worldUp4, FVector4(1,0,0,1));
	RTMatrix = FMatrix(ToCamera, Right, Up);
	//RTMatrix = FMatrix::Identity();
	//UE_LOG("%.2f, %.2f, %.2f", ToCamera.X, ToCamera.Y, ToCamera.Z);

	const FVector Translation = OwnerActorLocation + FVector(0.0f, 0.0f, ZOffset);
	//UE_LOG("%.2f, %.2f, %.2f", Translation.X, Translation.Y, Translation.Z);
	RTMatrix *= FMatrix::TranslationMatrix(Translation);
}
