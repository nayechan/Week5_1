#include "pch.h"
#include "Component/Public/BillboardComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Texture/Public/Texture.h"
#include "Editor/Public/Camera.h"

IMPLEMENT_CLASS(UBillboardComponent, UPrimitiveComponent)

UBillboardComponent::UBillboardComponent()
{
	Type = EPrimitiveType::Billboard;
}

UBillboardComponent::~UBillboardComponent()
{
}

void UBillboardComponent::UpdateFacingCamera(const UCamera* InCamera, bool bYawOnly)
{
	if (!InCamera) return;
	if (!GetOwner()) return;

	// World position of this component (owner location + component relative location).
	// For attached components you may want GetWorldTransformMatrix() instead.
	const FVector WorldPos = GetOwner()->GetActorLocation() + GetRelativeLocation();

	// Direction from billboard to camera in world space
	FVector ToCamera = InCamera->GetLocation() - WorldPos;
	const float DistSq = ToCamera.LengthSquared();
	if (DistSq < 1e-8f)
	{
		// Camera is on the billboard — nothing to do
		return;
	}
	ToCamera.Normalize();

	// Coordinate system: Z-up, X-forward, Y-right
	// Compute yaw around Z so forward (X) faces camera projection on XY plane.
	const float yawRad = atan2f(ToCamera.Y, ToCamera.X);
	const float yawDeg = yawRad * (180.0f / 3.14159265358979323846f);

	if (bYawOnly)
	{
		// Engine maps FVector -> (Roll, Pitch, Yaw) => (X, Y, Z)
		// Put yaw into Z to rotate around world Z.
		SetRelativeRotation(FVector(0.0f, 0.0f, yawDeg));
	}
	else
	{
		// Full spherical facing (yaw + pitch).
		const float horizLen = sqrtf(ToCamera.X * ToCamera.X + ToCamera.Y * ToCamera.Y);
		const float pitchRad = atan2f(-ToCamera.Z, horizLen); // sign may be tuned
		const float pitchDeg = pitchRad * (180.0f / 3.14159265358979323846f);

		// Store as (Roll, Pitch, Yaw) -> (X, Y, Z)
		SetRelativeRotation(FVector(0.0f, pitchDeg, yawDeg));
	}

	// Ensure world transform is recomputed
	MarkAsDirty();
}

void UBillboardComponent::SetSprite(const FName& InFilePath)
{
	// Clear sprite when no path provided
	if (InFilePath == FName::GetNone())
	{
		Sprite = nullptr;
		return;
	}

	// If already set to the same file, do nothing
	if (Sprite)
	{
		// Compare using file path string to avoid depending on UTexture internals
		// GetFilePath returns a string representation; guard against nullptr.
		try
		{
			if (Sprite->GetFilePath() == InFilePath.ToString())
			{
				return;
			}
		}
		catch (...)
		{
			// fall through and attempt to set new sprite
		}
	}

	UAssetManager& AssetManager = UAssetManager::GetInstance();

	// CreateTexture will load or create a UTexture wrapper and associated SRV.
	UTexture* NewTex = AssetManager.CreateTexture(InFilePath);

	if (!NewTex)
	{
		UE_LOG("UBillboardComponent::SetSprite: Failed to create or load texture '%s'", InFilePath.ToString().c_str());
		Sprite = nullptr;
		return;
	}

	Sprite = NewTex;
	UE_LOG("UBillboardComponent::SetSprite: Assigned texture '%s' to billboard", InFilePath.ToString().c_str());
}
